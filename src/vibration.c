#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define VIBE_GPIO 16  // choose a free GPIO

void vibe_init() {
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << VIBE_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    gpio_set_level(VIBE_GPIO, 0);  // Changed to 0 (LOW = OFF)
}

void vibe_on() {
    gpio_set_level(VIBE_GPIO, 1);  // Changed to 1 (HIGH = ON)
}

void vibe_off() {
    gpio_set_level(VIBE_GPIO, 0);  // Changed to 0 (LOW = OFF)
}

// Example: pulse vibration 200 ms
void vibe_pulse() {
    vibe_on();
    vTaskDelay(pdMS_TO_TICKS(200));
    vibe_off();
}
