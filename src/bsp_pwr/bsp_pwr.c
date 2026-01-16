#include "bsp_pwr.h"
#include "../Simple_btn/simple_button.h" 
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"  
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_timer.h"
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

// RTC memory 
RTC_DATA_ATTR static uint32_t deep_sleep_count = 0;
RTC_DATA_ATTR static uint64_t total_sleep_seconds = 0;

// FORWARD DECLARATION
static void power_button_callback(button_event_t event);

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
        xQueueReceive(pwr_event_queue, &event, 0);
    }
    return event;
}

// ==================== DEEP SLEEP FUNCTIONS ====================

bool bsp_pwr_is_deep_sleep_wake(void) {
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    return (cause != ESP_SLEEP_WAKEUP_UNDEFINED);
}

void bsp_pwr_log_wakeup_reason(void) {
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    
    switch (cause) {
        case ESP_SLEEP_WAKEUP_EXT1:
            ESP_LOGI(TAG, " Woke from GPIO (touch/button)");
            
            uint64_t wakeup_mask = esp_sleep_get_ext1_wakeup_status();
            if (wakeup_mask != 0) {
                int gpio = __builtin_ffsll(wakeup_mask) - 1;
                ESP_LOGI(TAG, "   → GPIO %d triggered wake", gpio);
            }
            break;
            
        case ESP_SLEEP_WAKEUP_TIMER:
            ESP_LOGI(TAG, " Woke from timer");
            break;
            
        case ESP_SLEEP_WAKEUP_UNDEFINED:
        default:
            ESP_LOGI(TAG, " Normal power-on (not deep sleep)");
            deep_sleep_count = 0;
            total_sleep_seconds = 0;
            break;
    }
    
    if (deep_sleep_count > 0) {
        ESP_LOGI(TAG, "Deep sleep cycles: %lu", deep_sleep_count);
        ESP_LOGI(TAG, "Total sleep time: %llu min", total_sleep_seconds / 60);
    }
}

void bsp_pwr_handle_deep_sleep_wake(void) {
    if (bsp_pwr_is_deep_sleep_wake()) {
        
        rtc_gpio_deinit(GPIO_NUM_11);  // Touch INT
        rtc_gpio_deinit(GPIO_NUM_18);  // Power button
        
        ESP_LOGI(TAG, "✓ Deep sleep wake handled");
    }
}

void bsp_pwr_deep_sleep(uint32_t sleep_time_sec) {
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔════════════════════════════════════╗");
    ESP_LOGI(TAG, "║  ENTERING DEEP SLEEP               ║");
    ESP_LOGI(TAG, "╚════════════════════════════════════╝");
    ESP_LOGI(TAG, "");
    
    // Update counters
    deep_sleep_count++;
    total_sleep_seconds += sleep_time_sec;
    
    // ========== DISABLE PERIPHERALS ==========
    
    ESP_LOGI(TAG, "Shutting down peripherals...");
    backlight_set(0);
    vTaskDelay(pdMS_TO_TICKS(50));
    
    // ========== CONFIGURE WAKE SOURCES ==========
    
    ESP_LOGI(TAG, "Configuring wake sources...");
    
    gpio_num_t touch_gpio = GPIO_NUM_11;   // TP_INT_GPIO
    gpio_num_t button_gpio = GPIO_NUM_18;  // PWR_KEY_PIN
    
    uint64_t wake_mask = (1ULL << touch_gpio) | (1ULL << button_gpio);
    
    // Configure as RTC GPIOs
    rtc_gpio_init(touch_gpio);
    rtc_gpio_set_direction(touch_gpio, RTC_GPIO_MODE_INPUT_ONLY);
    rtc_gpio_pullup_en(touch_gpio);
    rtc_gpio_pulldown_dis(touch_gpio);
    
    rtc_gpio_init(button_gpio);
    rtc_gpio_set_direction(button_gpio, RTC_GPIO_MODE_INPUT_ONLY);
    rtc_gpio_pullup_en(button_gpio);
    rtc_gpio_pulldown_dis(button_gpio);
    
    // Wake when either GPIO goes LOW
    esp_sleep_enable_ext1_wakeup_io(wake_mask, ESP_EXT1_WAKEUP_ANY_LOW);
    
    ESP_LOGI(TAG, " Wake on GPIO %d (touch) or GPIO %d (button)", 
             touch_gpio, button_gpio);
    
    // Timer wake 
    if (sleep_time_sec > 0) {
        uint64_t sleep_us = sleep_time_sec * 1000000ULL;
        esp_sleep_enable_timer_wakeup(sleep_us);
        ESP_LOGI(TAG, " Timer wake after %lu sec", sleep_time_sec);
    } else {
        ESP_LOGI(TAG, " No timer - sleep until GPIO");
    }
    
    // ========== ISOLATE UNUSED GPIOS ==========
    
    ESP_LOGI(TAG, "Isolating unused GPIOs...");
    
    for (int gpio = 0; gpio < 22; gpio++) {
        // Skip wake-up GPIOs
        if (gpio == touch_gpio || gpio == button_gpio) continue;
        
        // Skip flash pins 
        if (gpio >= 12 && gpio <= 17) continue;
        
        // Skip battery enable
        if (gpio == BAT_EN_PIN) continue;
        
        if (rtc_gpio_is_valid_gpio(gpio)) {
            rtc_gpio_isolate(gpio);
        }
    }
    
    ESP_LOGI(TAG, " GPIOs isolated");
    
    // ========== ENTER DEEP SLEEP ==========
    
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Deep sleep #%lu", deep_sleep_count);
    ESP_LOGI(TAG, "Estimated current: ~30 μA");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, " Entering deep sleep NOW...");

    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Go to sleep
    esp_deep_sleep_start();
}

// ==================== BUTTON CALLBACK ====================

static void power_button_callback(button_event_t event) {
    pwr_event_t pwr_event = PWR_EVENT_NONE;
    
    switch (event) {
        case BUTTON_EVENT_SINGLE_CLICK:
            ESP_LOGI(TAG, "Button: Single click");
            
            if (screen_sleeping) {
                pwr_event = PWR_EVENT_WAKE;
            } else {
                pwr_event = PWR_EVENT_GO_BACK;
            }
            break;

        case BUTTON_EVENT_LONG_PRESS:
            ESP_LOGW(TAG, "Button: Long press → Deep sleep");
            pwr_event = PWR_EVENT_DEEP_SLEEP;
            break;

        default:
            break;
    }
    
    if (pwr_event != PWR_EVENT_NONE && pwr_event_queue) {
        xQueueSend(pwr_event_queue, &pwr_event, 0);
    }
}

// ==================== INITIALIZATION (ONLY ONE!) ====================

void bsp_pwr_init(void) {
    ESP_LOGI(TAG, "Initializing power management...");

    // Create event queue
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

    // Initialize button with callback
    button_init(PWR_KEY_PIN, 0, power_button_callback);
    ESP_LOGI(TAG, "✓ Power button ready (GPIO %d)", PWR_KEY_PIN);
}
