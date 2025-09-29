#include <stdio.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/ledc.h"
#include "driver/i2c_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_heap_caps.h"

#include "cst816t.h"  // <— our minimal CST816T driver

static const char *TAG = "PAINT_DEMO_CST816T";

// ===== Display pins =====
#define PIN_NUM_MOSI 2
#define PIN_NUM_CLK  1
#define PIN_NUM_CS   5
#define PIN_NUM_DC   3
#define PIN_NUM_RST  4
#define PIN_NUM_BL   6

#define LCD_WIDTH   240
#define LCD_HEIGHT  280
#define LCD_X_GAP   0
#define LCD_Y_GAP   20   // top gap used in ST7789 init

// ===== Backlight (LEDC) =====
#define LEDC_TIMER      LEDC_TIMER_0
#define LEDC_MODE       LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO  PIN_NUM_BL
#define LEDC_CHANNEL    LEDC_CHANNEL_0
#define LEDC_DUTY_RES   LEDC_TIMER_10_BIT
#define LEDC_FREQUENCY  5000

// ===== Colors (RGB565) =====
#define C_BLACK   0x0000
#define C_WHITE   0xFFFF
#define C_RED     0xF800
#define C_GREEN   0x07E0
#define C_BLUE    0x001F
#define C_DARK    0x4208

// ===== Paint UI =====
#define PALETTE_H          26
#define PALETTE_Y0         0
#define SWATCH_W           40
#define SWATCH_H           (PALETTE_H - 6)
#define SWATCH_SPACING     6
#define SWATCH_Y           (PALETTE_Y0 + 3)
#define FIRST_SWATCH_X     6

// CLEAR button
#define CLEAR_W            60
#define CLEAR_H            (PALETTE_H - 6)
#define CLEAR_X            (LCD_WIDTH - CLEAR_W - 6)
#define CLEAR_Y            (PALETTE_Y0 + 3)

// Brush
#define BRUSH_RADIUS       4

// ===== Backlight helpers =====
static void backlight_init(void){
    ledc_timer_config_t t = {
        .speed_mode=LEDC_MODE, .timer_num=LEDC_TIMER,
        .duty_resolution=LEDC_DUTY_RES, .freq_hz=LEDC_FREQUENCY, .clk_cfg=LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&t));
    ledc_channel_config_t c = {
        .gpio_num=LEDC_OUTPUT_IO, .speed_mode=LEDC_MODE, .channel=LEDC_CHANNEL,
        .timer_sel=LEDC_TIMER, .duty=0, .hpoint=0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&c));
}
static void backlight_set(uint8_t p){
    if(p>100) p=100;
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE,LEDC_CHANNEL,(1023*p)/100));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE,LEDC_CHANNEL));
}

// ===== Framebuffer helpers =====
static void fb_fill(uint16_t *fb, uint16_t color){
    for (int i=0;i<LCD_WIDTH*LCD_HEIGHT;i++) fb[i]=color;
}
static void fb_hline(uint16_t *fb,int x0,int x1,int y,uint16_t c){
    if(y<0||y>=LCD_HEIGHT) return;
    if(x0>x1){int t=x0;x0=x1;x1=t;}
    if(x1<0||x0>=LCD_WIDTH) return;
    if(x0<0) x0=0;
    if(x1>=LCD_WIDTH) x1=LCD_WIDTH-1;
    uint16_t *p=&fb[y*LCD_WIDTH + x0];
    for(int x=x0;x<=x1;x++) *p++=c;
}
static void fb_rect(uint16_t *fb,int x,int y,int w,int h,uint16_t c){
    int x1=x+w-1, y1=y+h-1;
    if(x>=LCD_WIDTH||y>=LCD_HEIGHT||x1<0||y1<0) return;
    if(x<0){w += x; x=0;}
    if(y<0){h += y; y=0;}
    if(x+w>LCD_WIDTH) w=LCD_WIDTH-x;
    if(y+h>LCD_HEIGHT) h=LCD_HEIGHT-y;
    for(int yy=y; yy<y+h; yy++){
        uint16_t *p=&fb[yy*LCD_WIDTH + x];
        for(int xx=0; xx<w; xx++) *p++=c;
    }
}
static void fb_rect_outline(uint16_t *fb,int x,int y,int w,int h,uint16_t c){
    fb_hline(fb,x,x+w-1,y,c);
    fb_hline(fb,x,x+w-1,y+h-1,c);
    for(int yy=y; yy<y+h; yy++){
        if(yy<0||yy>=LCD_HEIGHT) continue;
        if(x>=0&&x<LCD_WIDTH) fb[yy*LCD_WIDTH + x]=c;
        if(x+w-1>=0&&x+w-1<LCD_WIDTH) fb[yy*LCD_WIDTH + (x+w-1)]=c;
    }
}
static void fb_circle_fill(uint16_t *fb,int cx,int cy,int r,uint16_t c){
    int r2=r*r;
    for(int y=-r;y<=r;y++){
        int yy=cy+y;
        if(yy<0||yy>=LCD_HEIGHT) continue;
        int dx=(int)sqrtf((float)(r2 - y*y));
        int x0=cx - dx, x1=cx + dx;
        if(x1<0||x0>=LCD_WIDTH) continue;
        if(x0<0) x0=0;
        if(x1>=LCD_WIDTH) x1=LCD_WIDTH-1;
        uint16_t *p=&fb[yy*LCD_WIDTH + x0];
        for(int x=x0; x<=x1; x++) *p++=c;
    }
}

// ===== Palette helpers =====
typedef enum { SW_WHITE=0, SW_RED, SW_GREEN, SW_BLUE, SWATCH_COUNT } swatch_id_t;
static uint16_t swatch_color(swatch_id_t id){
    switch(id){
        case SW_WHITE: return C_WHITE;
        case SW_RED:   return C_RED;
        case SW_GREEN: return C_GREEN;
        case SW_BLUE:  return C_BLUE;
        default:       return C_WHITE;
    }
}
static void draw_palette(uint16_t *fb, swatch_id_t active){
    fb_rect(fb, 0, PALETTE_Y0, LCD_WIDTH, PALETTE_H, 0x18E3);
    for(int i=0;i<SWATCH_COUNT;i++){
        int x = FIRST_SWATCH_X + i*(SWATCH_W+SWATCH_SPACING);
        fb_rect(fb, x, SWATCH_Y, SWATCH_W, SWATCH_H, swatch_color(i));
        fb_rect_outline(fb, x, SWATCH_Y, SWATCH_W, SWATCH_H, C_DARK);
        if (i == active) {
            fb_rect_outline(fb, x-2, SWATCH_Y-2, SWATCH_W+4, SWATCH_H+4, C_WHITE);
        }
    }
    fb_rect(fb, CLEAR_X, CLEAR_Y, CLEAR_W, CLEAR_H, 0x39C7);
    fb_rect_outline(fb, CLEAR_X, CLEAR_Y, CLEAR_W, CLEAR_H, C_DARK);
}
static bool hit_swatch(int x,int y, swatch_id_t *out_id){
    for(int i=0;i<SWATCH_COUNT;i++){
        int sx = FIRST_SWATCH_X + i*(SWATCH_W+SWATCH_SPACING);
        if (x>=sx && x<sx+SWATCH_W && y>=SWATCH_Y && y<SWATCH_Y+SWATCH_H){
            if(out_id) *out_id = (swatch_id_t)i;
            return true;
        }
    }
    return false;
}
static bool hit_clear(int x,int y){
    return (x>=CLEAR_X && x<CLEAR_X+CLEAR_W && y>=CLEAR_Y && y<CLEAR_Y+CLEAR_H);
}

void app_main(void)
{
    // --- SPI bus for ST7789 ---
    spi_bus_config_t buscfg = {
        .sclk_io_num=PIN_NUM_CLK, .mosi_io_num=PIN_NUM_MOSI, .miso_io_num=-1,
        .quadwp_io_num=-1, .quadhd_io_num=-1, .max_transfer_sz=LCD_WIDTH*LCD_HEIGHT*2
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST,&buscfg,SPI_DMA_CH_AUTO));

    // Panel IO + ST7789
    esp_lcd_panel_io_handle_t io=NULL;
    esp_lcd_panel_io_spi_config_t iocfg = {
        .dc_gpio_num=PIN_NUM_DC, .cs_gpio_num=PIN_NUM_CS, .pclk_hz=40*1000*1000,
        .lcd_cmd_bits=8, .lcd_param_bits=8, .spi_mode=0, .trans_queue_depth=10
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST,&iocfg,&io));

    esp_lcd_panel_handle_t panel=NULL;
    esp_lcd_panel_dev_config_t pcfg = {
        .reset_gpio_num=PIN_NUM_RST, .color_space=ESP_LCD_COLOR_SPACE_RGB, .bits_per_pixel=16
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io,&pcfg,&panel));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel));
    ESP_ERROR_CHECK(esp_lcd_panel_set_gap(panel, LCD_X_GAP, LCD_Y_GAP));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel, true));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel, true));

    // Backlight
    backlight_init();
    backlight_set(100);

    // --- I2C bus for CST816T ---
    i2c_master_bus_handle_t i2c_bus = NULL;
    i2c_master_bus_config_t i2c_cfg = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = GPIO_NUM_8,   // SDA from your table
        .scl_io_num = GPIO_NUM_7,   // SCL from your table
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags = { .enable_internal_pullup = true },
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_cfg, &i2c_bus));

    cst816t_t tp = {0};
    ESP_ERROR_CHECK(cst816t_init(&tp, i2c_bus, GPIO_NUM_11)); // INT on GPIO11 (or GPIO_NUM_NC)

    // Framebuffer
    size_t sz = LCD_WIDTH*LCD_HEIGHT*2;
    uint16_t *fb = heap_caps_malloc(sz, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if(!fb){
        ESP_LOGE(TAG,"FB alloc failed");
        return;
    }

    // Initial canvas
    fb_fill(fb, C_BLACK);
    fb_rect(fb, 0, PALETTE_Y0+PALETTE_H, LCD_WIDTH, LCD_HEIGHT-(PALETTE_Y0+PALETTE_H), 0x0008);
    swatch_id_t active = SW_WHITE;
    draw_palette(fb, active);
    ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel, 0, 0, LCD_WIDTH, LCD_HEIGHT, fb));

    // Paint loop
    while(1){
        bool touched = false;
        uint16_t rx = 0, ry = 0;
        esp_err_t tr = cst816t_read_point(&tp, &touched, &rx, &ry);
        if (tr != ESP_OK) {
            ESP_LOGE(TAG, "CST816T read error: %s", esp_err_to_name(tr));
            vTaskDelay(pdMS_TO_TICKS(16));
            continue;
        }

        if (touched){
            int tx = (int)rx;
            int ty = (int)ry - LCD_Y_GAP;

            if (tx < 0) tx = 0;
            if (tx >= LCD_WIDTH) tx = LCD_WIDTH - 1;
            if (ty < 0) ty = 0;
            if (ty >= LCD_HEIGHT) ty = LCD_HEIGHT - 1;

            if (ty >= PALETTE_Y0 && ty < PALETTE_Y0 + PALETTE_H){
                swatch_id_t id;
                if (hit_swatch(tx, ty, &id)){
                    active = id;
                    draw_palette(fb, active);
                } else if (hit_clear(tx, ty)){
                    fb_rect(fb, 0, PALETTE_Y0+PALETTE_H,
                            LCD_WIDTH, LCD_HEIGHT-(PALETTE_Y0+PALETTE_H), 0x0008);
                }
            } else {
                fb_circle_fill(fb, tx, ty, BRUSH_RADIUS, swatch_color(active));
            }

            ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel, 0, 0, LCD_WIDTH, LCD_HEIGHT, fb));
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
