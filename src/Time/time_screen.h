#ifndef TIME_SCREEN_H
#define TIME_SCREEN_H

#include "lvgl.h"

// Initialize the time screen module
void time_screen_init(void);

// Update the time shown on the time screen
void time_screen_update(const char *time_str);

// Event callback for the Time button
void time_btn_event_cb(lv_event_t *e);

void time_screen_back_to_menu(void);

#endif // TIME_SCREEN_H
