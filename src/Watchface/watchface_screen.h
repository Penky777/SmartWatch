#ifndef WATCHFACE_SCREEN_H
#define WATCHFACE_SCREEN_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Build watchface screen with background image
 */
lv_obj_t *build_watchface_screen(void);

/**
 * @brief Update time display on watchface
 * @param time_str Time string "HH:MM:SS"
 */
void watchface_update_time(const char *time_str);

/**
 * @brief Update battery percentage display
 * @param percent Battery level 0-100
 */
void watchface_update_battery(uint8_t percent);

/**
 * @brief Update step count display
 * @param steps Current step count
 */
void watchface_update_steps(uint32_t steps);
void watchface_update_date(const char *date_str);

#ifdef __cplusplus
}
#endif

#endif 