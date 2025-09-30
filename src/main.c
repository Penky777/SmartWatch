
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
#include "bsp_pcf85063.h" 
#include "cst816t.h"
#include "bsp_qmi8658.h"
#include "bsp_i2c.h"

#include "bsp_pcf85063.h"
#include "lvgl.h"  
#include "bsp_qmi8658.h"

static const char *TAG = "RTC_DISPLAY";

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
#define LCD_Y_GAP   0

// ===== Backlight =====
#define LEDC_TIMER      LEDC_TIMER_0
#define LEDC_MODE       LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO  PIN_NUM_BL
#define LEDC_CHANNEL    LEDC_CHANNEL_0
#define LEDC_DUTY_RES   LEDC_TIMER_10_BIT
#define LEDC_FREQUENCY  5000

// ===== Colors (RGB565) =====
#define C_BLACK   0x0000
#define C_WHITE   0xFFFF
#define C_BLUE    0x001F
#define C_GREEN   0x07E0
#define C_RED     0xF800

// ===== Backlight helpers =====
static void backlight_init(void){
    ledc_timer_config_t t = {
        .speed_mode=LEDC_MODE,.timer_num=LEDC_TIMER,
        .duty_resolution=LEDC_DUTY_RES,.freq_hz=LEDC_FREQUENCY,.clk_cfg=LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&t));
    ledc_channel_config_t c = {
        .gpio_num=LEDC_OUTPUT_IO,.speed_mode=LEDC_MODE,.channel=LEDC_CHANNEL,
        .timer_sel=LEDC_TIMER,.duty=0,.hpoint=0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&c));
}
static void backlight_set(uint8_t p){
    if(p>100) p=100;
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE,LEDC_CHANNEL,(1023*p)/100));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE,LEDC_CHANNEL));
}

// ===== Simple framebuffer helpers =====
static void fb_fill(uint16_t *fb, uint16_t color){
    for (int i=0;i<LCD_WIDTH*LCD_HEIGHT;i++) fb[i]=color;
}
static void fb_rect(uint16_t *fb,int x,int y,int w,int h,uint16_t c){
    for(int yy=y; yy<y+h; yy++){
        if(yy<0||yy>=LCD_HEIGHT) continue;
        uint16_t *p=&fb[yy*LCD_WIDTH + x];
        for(int xx=0; xx<w; xx++){
            if(x+xx>=0 && x+xx<LCD_WIDTH) *p++=c;
        }
    }
}

// ===== Very basic text drawing (5x7 font) =====
static const uint8_t font5x7[] = {
    // only digits '0'..'9' and ':' stored here
    // each char is 5 bytes (columns)
    // '0'
    0x3E,0x51,0x49,0x45,0x3E,
    // '1'
    0x00,0x42,0x7F,0x40,0x00,
    // '2'
    0x42,0x61,0x51,0x49,0x46,
    // '3'
    0x21,0x41,0x45,0x4B,0x31,
    // '4'
    0x18,0x14,0x12,0x7F,0x10,
    // '5'
    0x27,0x45,0x45,0x45,0x39,
    // '6'
    0x3C,0x4A,0x49,0x49,0x30,
    // '7'
    0x01,0x71,0x09,0x05,0x03,
    // '8'
    0x36,0x49,0x49,0x49,0x36,
    // '9'
    0x06,0x49,0x49,0x29,0x1E,
    // ':'
    0x00,0x36,0x36,0x00,0x00
};
static void draw_char(uint16_t *fb,int x,int y,char ch,uint16_t color){
    int idx=-1;
    if(ch>='0' && ch<='9') idx=(ch-'0');
    else if(ch==':') idx=10;
    if(idx<0) return;
    const uint8_t *glyph=&font5x7[idx*5];
    for(int col=0; col<5; col++){
        for(int row=0; row<7; row++){
            if(glyph[col] & (1<<row)){
                int xx=x+col, yy=y+row;
                if(xx>=0&&xx<LCD_WIDTH&&yy>=0&&yy<LCD_HEIGHT){
                    fb[yy*LCD_WIDTH+xx]=color;
                }
            }
        }
    }
}
static void draw_string(uint16_t *fb,int x,int y,const char *s,uint16_t color){
    while(*s){
        draw_char(fb,x,y,*s,color);
        x+=6; // 5px + 1 space
        s++;
    }
}

void app_main(void)
{
    // --- SPI bus for ST7789 ---
    spi_bus_config_t buscfg = {
        .sclk_io_num=PIN_NUM_CLK, .mosi_io_num=PIN_NUM_MOSI, .miso_io_num=-1,
        .quadwp_io_num=-1, .quadhd_io_num=-1, .max_transfer_sz=LCD_WIDTH*LCD_HEIGHT*2
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST,&buscfg,SPI_DMA_CH_AUTO));

    esp_lcd_panel_io_handle_t io=NULL;
    esp_lcd_panel_io_spi_config_t iocfg = {
        .dc_gpio_num=PIN_NUM_DC, 
        .cs_gpio_num=PIN_NUM_CS, 
        .pclk_hz=80*1000*1000,
        .lcd_cmd_bits=8, 
        .lcd_param_bits=8, 
        .spi_mode=0, 
        .trans_queue_depth=10
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST,&iocfg,&io));

    esp_lcd_panel_handle_t panel=NULL;
    esp_lcd_panel_dev_config_t pcfg = {
        .reset_gpio_num=PIN_NUM_RST, .color_space=ESP_LCD_COLOR_SPACE_RGB, .bits_per_pixel=16
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io,&pcfg,&panel));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel));
    ESP_ERROR_CHECK(esp_lcd_panel_set_gap(panel,LCD_X_GAP,LCD_Y_GAP));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel,true));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel,true));

    // Backlight
    backlight_init();
    backlight_set(100);

    // --- I2C bus for RTC ---
    i2c_master_bus_handle_t bus=NULL;
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = GPIO_NUM_8,
        .scl_io_num = GPIO_NUM_7,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags = {.enable_internal_pullup=true},
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg,&bus));

    // --- RTC init ---
    bsp_pcf85063_init(bus);

    // Framebuffer
    size_t sz = LCD_WIDTH*LCD_HEIGHT*2;
    uint16_t *fb = heap_caps_malloc(sz, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if(!fb){ ESP_LOGE(TAG,"FB alloc failed"); return; }

    // Loop: read RTC and show
    while(1){
        struct tm now;
        if(bsp_pcf85063_get_time(&now)){
            char buf[32];
            snprintf(buf,sizeof(buf),"%02d:%02d:%02d",
                     now.tm_hour,now.tm_min,now.tm_sec);

            fb_fill(fb,C_BLACK);
            draw_string(fb,60,120,buf,C_GREEN);

            ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel,0,0,LCD_WIDTH,LCD_HEIGHT,fb));
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}