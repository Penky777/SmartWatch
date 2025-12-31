#include "simple_button.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

static const char *TAG = "BUTTON";

#define DEBOUNCE_MS         50
#define LONG_PRESS_MS       2000
#define POLL_INTERVAL_MS    20

typedef struct {
    gpio_num_t gpio;
    uint8_t active_level;
    button_callback_t callback;
    bool pressed;
    uint32_t press_time;
    bool long_press_triggered;
} button_state_t;

static button_state_t btn_state = {0};

// Read button state (debounced)
static bool button_is_pressed(void) {
    return (gpio_get_level(btn_state.gpio) == btn_state.active_level);
}

// Button monitoring task
static void button_task(void *arg) {
    ESP_LOGI(TAG, "Button task started on GPIO %d", btn_state.gpio);
    
    bool last_state = false;
    uint32_t stable_time = 0;
    
    while (1) {
        bool current = button_is_pressed();
        uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
        
        // Debouncing
        if (current != last_state) {
            stable_time = now;
            last_state = current;
        }
        
        uint32_t stable_duration = now - stable_time;
        
        if (stable_duration >= DEBOUNCE_MS) {
            // Button pressed
            if (current && !btn_state.pressed) {
                btn_state.pressed = true;
                btn_state.press_time = now;
                btn_state.long_press_triggered = false;
                ESP_LOGD(TAG, "Button pressed");
            }
            // Button still held
            else if (current && btn_state.pressed) {
                uint32_t hold_time = now - btn_state.press_time;
                
                if (hold_time >= LONG_PRESS_MS && !btn_state.long_press_triggered) {
                    btn_state.long_press_triggered = true;
                    ESP_LOGI(TAG, "Long press detected");
                    
                    if (btn_state.callback) {
                        btn_state.callback(BUTTON_EVENT_LONG_PRESS);
                    }
                }
            }
            // Button released
            else if (!current && btn_state.pressed) {
                uint32_t press_duration = now - btn_state.press_time;
                btn_state.pressed = false;
                
                
                if (!btn_state.long_press_triggered && press_duration < LONG_PRESS_MS) {
                    ESP_LOGI(TAG, "Single click detected");
                    
                    if (btn_state.callback) {
                        btn_state.callback(BUTTON_EVENT_SINGLE_CLICK);
                    }
                }
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(POLL_INTERVAL_MS));
    }
}

// Initialize button
void button_init(gpio_num_t gpio_num, uint8_t active_level, button_callback_t callback) {
    btn_state.gpio = gpio_num;
    btn_state.active_level = active_level;
    btn_state.callback = callback;
    btn_state.pressed = false;
    btn_state.press_time = 0;
    btn_state.long_press_triggered = false;
    
    // Configure GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << gpio_num),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = (active_level == 0) ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
        .pull_down_en = (active_level == 1) ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    
    ESP_LOGI(TAG, "Button initialized on GPIO %d (active %s)", 
             gpio_num, active_level ? "HIGH" : "LOW");
    
    // Create monitoring task
    xTaskCreate(button_task, "button", 2048, NULL, 5, NULL);
}
