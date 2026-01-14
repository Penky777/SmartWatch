#ifndef ALERTS_SCREEN_H
#define ALERTS_SCREEN_H

#include "lvgl.h"

lv_obj_t *build_alerts_screen(void);
void alerts_add_notification(const char *title, const char *message);
void alerts_clear_all(void);

#endif
