#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_heap_caps.h"

static const char *TAG = "ST7789_TOUCH";

// ---------- LCD pins ----------
#define PIN_NUM_MOSI 2
#define PIN_NUM_CLK  1
#define PIN_NUM_CS   5
#define PIN_NUM_DC   3
#define PIN_NUM_RST  4
#define PIN_NUM_BL   6

#define LCD_WIDTH  240
#define LCD_HEIGHT 280

// ---------- Backlight ----------
#define LEDC_TIMER    LEDC_TIMER_0
#define LEDC_MODE     LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL  LEDC_CHANNEL_0
#define LEDC_DUTY_RES LEDC_TIMER_10_BIT
#define LEDC_FREQ     5000

static void backlight_init(void) {
    ledc_timer_config_t timer = {
        .speed_mode = LEDC_MODE,
        .timer_num = LEDC_TIMER,
        .duty_resolution = LEDC_DUTY_RES,
        .freq_hz = LEDC_FREQ,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer));

    ledc_channel_config_t channel = {
        .gpio_num = PIN_NUM_BL,
        .speed_mode = LEDC_MODE,
        .channel = LEDC_CHANNEL,
        .timer_sel = LEDC_TIMER,
        .duty = 0,
        .hpoint = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&channel));
}

static void backlight_set_brightness(uint8_t brightness) {
    uint32_t duty = (1023 * brightness) / 100; // 10-bit
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));
}

// ---------- Touch pins (XPT2046) ----------
#define TOUCH_MISO 19
#define TOUCH_CLK  18
#define TOUCH_CS   21
#define TOUCH_IRQ  22

spi_device_handle_t touch_dev;

void touch_init() {
    spi_bus_config_t buscfg = {
        .mosi_io_num = -1,
        .miso_io_num = TOUCH_MISO,
        .sclk_io_num = TOUCH_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 2 * 1000 * 1000,
        .mode = 0,
        .spics_io_num = TOUCH_CS,
        .queue_size = 1
    };
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &devcfg, &touch_dev));

    gpio_set_direction(TOUCH_IRQ, GPIO_MODE_INPUT);
}

bool touch_pressed() {
    return gpio_get_level(TOUCH_IRQ) == 0; // low when pressed
}

// ---------- Main ----------
void app_main(void)
{
    ESP_LOGI(TAG, "Init SPI + LCD + Touch");

    // --- SPI for LCD ---
    spi_bus_config_t buscfg = {
        .sclk_io_num = PIN_NUM_CLK,
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_WIDTH * LCD_HEIGHT * sizeof(uint16_t)
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));

    // --- Framebuffer (fill RED first) ---
    size_t buf_size = LCD_WIDTH * LCD_HEIGHT * sizeof(uint16_t);
    uint16_t *framebuffer = heap_caps_malloc(buf_size, MALLOC_CAP_DMA);
    if (!framebuffer) {
        ESP_LOGE(TAG, "Failed to allocate framebuffer");
        return;
    }
    for (int i = 0; i < LCD_WIDTH * LCD_HEIGHT; i++) framebuffer[i] = 0xF800; // red

    // --- LCD ---
    esp_lcd_panel_io_handle_t io_handle;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = PIN_NUM_DC,
        .cs_gpio_num = PIN_NUM_CS,
        .pclk_hz = 40 * 1000 * 1000,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST, &io_config, &io_handle));

    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = PIN_NUM_RST,
        .color_space = ESP_LCD_COLOR_SPACE_RGB,
        .bits_per_pixel = 16
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));

    // --- Reset + Init ---
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_set_gap(panel_handle, 0, 0)); // set gap 0,0 for no offset
    // remove invert_color entirely
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    // draw initial red frame
    ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, LCD_WIDTH, LCD_HEIGHT, framebuffer));

    // --- Backlight ---
    backlight_init();
    backlight_set_brightness(100);

    // --- Touch ---
    touch_init();

    // --- Color toggle: RED <-> GREEN ---
    uint16_t colors[2] = {0xF800, 0x07E0};
    int current = 0;

    while (1) {
        if (touch_pressed()) {
            current = (current + 1) % 2;
            for (int i = 0; i < LCD_WIDTH * LCD_HEIGHT; i++)
                framebuffer[i] = colors[current];
            ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, LCD_WIDTH, LCD_HEIGHT, framebuffer));

            // wait until release to debounce
            while (touch_pressed()) vTaskDelay(pdMS_TO_TICKS(10));
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
