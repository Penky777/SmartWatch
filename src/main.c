#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_commands.h"

static const char *TAG = "ST7789";

// ==== Pin config (adjust to your board!) ====
#define PIN_NUM_MOSI 2
#define PIN_NUM_CLK  1
#define PIN_NUM_CS    5
#define PIN_NUM_DC   3
#define PIN_NUM_RST  4
#define PIN_NUM_BL    6   // backlight enable

// Display resolution
#define LCD_WIDTH  240
#define LCD_HEIGHT 280

void app_main(void)
{
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

    ESP_LOGI(TAG, "Attach ST7789 to SPI bus");
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

    ESP_LOGI(TAG, "Initialize ST7789 panel");
    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = PIN_NUM_RST,
        .color_space = ESP_LCD_COLOR_SPACE_RGB,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));

    // Reset and init panel
    esp_lcd_panel_reset(panel_handle);
    esp_lcd_panel_init(panel_handle);

    // Turn on backlight
    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << PIN_NUM_BL
    };
    gpio_config(&bk_gpio_config);
    gpio_set_level(PIN_NUM_BL, 1);

    // Set rotation (0–3)
    esp_lcd_panel_mirror(panel_handle, false, false);

    // Fill screen
    uint16_t color = 0x001F; // blue
    esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, LCD_WIDTH, LCD_HEIGHT, &color);

    ESP_LOGI(TAG, "LCD init complete");

    // Example: simple color loop
    while (1) {
        uint16_t red = 0xF800;
        uint16_t green = 0x07E0;
        uint16_t blue = 0x001F;

        esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, LCD_WIDTH, LCD_HEIGHT, &red);
        vTaskDelay(pdMS_TO_TICKS(500));

        esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, LCD_WIDTH, LCD_HEIGHT, &green);
        vTaskDelay(pdMS_TO_TICKS(500));

        esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, LCD_WIDTH, LCD_HEIGHT, &blue);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
