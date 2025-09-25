#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/ledc.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_commands.h"
#include "esp_heap_caps.h"

static const char *TAG = "ST7789";

// Define pins from BSP header
#define PIN_NUM_MOSI 2
#define PIN_NUM_CLK  1
#define PIN_NUM_CS   5
#define PIN_NUM_DC   3
#define PIN_NUM_RST  4
#define PIN_NUM_BL   6

#define LCD_WIDTH  240
#define LCD_HEIGHT 280

// LEDC PWM config for backlight
#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO          PIN_NUM_BL
#define LEDC_CHANNEL            LEDC_CHANNEL_0
#define LEDC_DUTY_RES           LEDC_TIMER_10_BIT
#define LEDC_FREQUENCY          5000 // 5 kHz

static void backlight_init(void) {
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_MODE,
        .timer_num = LEDC_TIMER,
        .duty_resolution = LEDC_DUTY_RES,
        .freq_hz = LEDC_FREQUENCY,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ledc_channel_config_t ledc_channel = {
        .gpio_num = LEDC_OUTPUT_IO,
        .speed_mode = LEDC_MODE,
        .channel = LEDC_CHANNEL,
        .timer_sel = LEDC_TIMER,
        .duty = 0, // initially off
        .hpoint = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}

static void backlight_set_brightness(uint8_t brightness) {
    // brightness 0-100%
    uint32_t duty = (8191 * brightness) / 100; // 2^13 -1 max for 13-bit, but using 10bit timer so max 1023
    duty = (1023 * brightness) / 100; // For 10-bit resolution max is 1023
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));
}

void app_main(void)
{
    // Initialize SPI bus
    ESP_LOGI(TAG, "Initialize SPI bus");
    spi_bus_config_t buscfg = {
        .sclk_io_num = PIN_NUM_CLK,
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_WIDTH * LCD_HEIGHT * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));

    // Attach ST7789 panel IO to SPI bus
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = PIN_NUM_DC,
        .cs_gpio_num = PIN_NUM_CS,
        .pclk_hz = 40 * 1000 * 1000,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST, &io_config, &io_handle));

    // Create ST7789 panel instance
    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = PIN_NUM_RST,
        .color_space = ESP_LCD_COLOR_SPACE_RGB,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));

    // Reset and initialize the panel
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));

    // Set gaps / offsets (from BSP)
    ESP_ERROR_CHECK(esp_lcd_panel_set_gap(panel_handle, 0, 20));

    // Invert colors if needed (depends on your panel)
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));

    // Turn display on
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    // Initialize backlight PWM, set brightness to 100%
    backlight_init();
    backlight_set_brightness(100);

    // Allocate framebuffer (DMA capable)
    size_t buf_size = LCD_WIDTH * LCD_HEIGHT * sizeof(uint16_t);
    uint16_t *framebuffer = heap_caps_malloc(buf_size, MALLOC_CAP_DMA);
    if (!framebuffer) {
        ESP_LOGE(TAG, "Failed to allocate framebuffer");
        return;
    }

    // Fill initial color (blue)
    for (int i = 0; i < LCD_WIDTH * LCD_HEIGHT; i++) {
        framebuffer[i] = 0x001F;
    }
    ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, LCD_WIDTH, LCD_HEIGHT, framebuffer));
    ESP_LOGI(TAG, "Display initialized");

    while (1) {
        // Red fill
        for (int i = 0; i < LCD_WIDTH * LCD_HEIGHT; i++) framebuffer[i] = 0xF800;
        ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, LCD_WIDTH, LCD_HEIGHT, framebuffer));
        vTaskDelay(pdMS_TO_TICKS(500));

        // Green fill
        for (int i = 0; i < LCD_WIDTH * LCD_HEIGHT; i++) framebuffer[i] = 0x07E0;
        ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, LCD_WIDTH, LCD_HEIGHT, framebuffer));
        vTaskDelay(pdMS_TO_TICKS(500));

        // Blue fill
        for (int i = 0; i < LCD_WIDTH * LCD_HEIGHT; i++) framebuffer[i] = 0x001F;
        ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, LCD_WIDTH, LCD_HEIGHT, framebuffer));
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
