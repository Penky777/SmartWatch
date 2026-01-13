#include "bsp_pwr.h"
#include "../Simple_btn/simple_button.h" 
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"  
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

// Event queue
static QueueHandle_t pwr_event_queue = NULL;

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

pwr_event_t bsp_pwr_get_event(void) {
    pwr_event_t event = PWR_EVENT_NONE;
    if (pwr_event_queue) {
        xQueueReceive(pwr_event_queue, &event, 0);  // Non-blocking
    }
    return event;
}

// ==================== BUTTON CALLBACK ====================

static void power_button_callback(button_event_t event) {
    pwr_event_t pwr_event = PWR_EVENT_NONE;
    
    switch (event) {
        case BUTTON_EVENT_SINGLE_CLICK:
            ESP_LOGI(TAG, "Button: Single click");
            
            if (screen_sleeping) {
                // Wake immediately
                pwr_event = PWR_EVENT_WAKE;
            } else {
                // Queue the back event 
                pwr_event = PWR_EVENT_GO_BACK;
            }
            break;

        case BUTTON_EVENT_LONG_PRESS:
            ESP_LOGW(TAG, "Button: Long press → Shutdown");
            pwr_event = PWR_EVENT_SHUTDOWN;
            break;

        default:
            break;
    }
    
    // Send event to queue 
    if (pwr_event != PWR_EVENT_NONE && pwr_event_queue) {
        xQueueSend(pwr_event_queue, &pwr_event, 0);
    }
}

// ==================== INITIALIZATION ====================

void bsp_pwr_init(void) {
    ESP_LOGI(TAG, "Initializing power management...");

    // CREATE THE QUEUE!
    pwr_event_queue = xQueueCreate(5, sizeof(pwr_event_t));
    if (!pwr_event_queue) {
        ESP_LOGE(TAG, "Failed to create event queue!");
        return;
    }

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
