#ifndef SIMPLE_BUTTON_H
#define SIMPLE_BUTTON_H

#include <stdbool.h>
#include "driver/gpio.h"

typedef enum {
    BUTTON_EVENT_NONE = 0,
    BUTTON_EVENT_SINGLE_CLICK,
    BUTTON_EVENT_LONG_PRESS
} button_event_t;

typedef void (*button_callback_t)(button_event_t event);

/**
 * @brief 
 * @param gpio_num 
 * @param active_level 
 * @param callback 
 */
void button_init(gpio_num_t gpio_num, uint8_t active_level, button_callback_t callback);

#endif 
