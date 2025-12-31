#ifndef ACTIVITY_SCREEN_H
#define ACTIVITY_SCREEN_H

#include "lvgl.h"
#include <stdint.h>

// Build the activity screen
lv_obj_t *build_activity_screen(void);

// Update activity display with new values
void activity_screen_update(uint16_t steps, uint8_t bpm, uint8_t spo2);

#endif // ACTIVITY_SCREEN_H
