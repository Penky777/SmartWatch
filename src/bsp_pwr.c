#include "bsp_pwr.h"
#include "simple_button.h" 
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lvgl.h"

static const char *TAG = "PWR";


extern void backlight_set(uint8_t percent);
extern uint8_t g_backlight_level;
extern bool g_backlight_on;
extern uint32_t g_last_activity_ms;

// State
static uint8_t brightness_before_sleep = 0;
static bool screen_sleeping = false;

// ==================== PUBLIC API ====================

bool bsp_pwr_is_screen_sleeping(void) {
    return screen_sleeping;
}

void bsp_pwr_wake_screen(void) {
    if (screen_sleeping && brightness_before_sleep > 0) {
        ESP_LOGI(TAG, "Wake → %d%%", brightness_before_sleep);
        backlight_set(brightness_before_sleep);
        g_backlight_on = true;
        g_backlight_level = brightness_before_sleep;
        g_last_activity_ms = lv_tick_get();
        screen_sleeping = false;
    }
}

void bsp_pwr_sleep_screen(void) {
    if (!screen_sleeping) {
        brightness_before_sleep = g_backlight_level;
        ESP_LOGI(TAG, "Sleep (was %d%%)", brightness_before_sleep);
        backlight_set(0);
        g_backlight_on = false;
        screen_sleeping = true;
    }
}

// ==================== BUTTON CALLBACK ====================

static void power_button_callback(button_event_t event) {
    switch (event) {
        case BUTTON_EVENT_SINGLE_CLICK:
            ESP_LOGI(TAG, "Button: Single click");
            if (screen_sleeping) {
                bsp_pwr_wake_screen();
            } else {
                bsp_pwr_sleep_screen();
            }
            break;

        case BUTTON_EVENT_LONG_PRESS:
            ESP_LOGW(TAG, "Button: Long press → Shutdown");
            backlight_set(0);
            vTaskDelay(pdMS_TO_TICKS(200));
            gpio_set_level(BAT_EN_PIN, 0);  // Cut power
            break;

        default:
            break;
    }
}

// ==================== INITIALIZATION ====================

void bsp_pwr_init(void) {
    ESP_LOGI(TAG, "Initializing power management...");

    // Enable battery power
    gpio_reset_pin(BAT_EN_PIN);
    gpio_set_direction(BAT_EN_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(BAT_EN_PIN, 1);
    ESP_LOGI(TAG, "✓ Battery enabled (GPIO %d)", BAT_EN_PIN);


    vTaskDelay(pdMS_TO_TICKS(50));

    // Initialize button (active LOW = 0)
    button_init(PWR_KEY_PIN, 0, power_button_callback);
    ESP_LOGI(TAG, "✓ Power button ready (GPIO %d)", PWR_KEY_PIN);
}
