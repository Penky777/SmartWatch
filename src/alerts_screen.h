#ifndef ALERTS_SCREEN_H
#define ALERTS_SCREEN_H

#include "lvgl.h"
#include <stdbool.h>

lv_obj_t *build_alerts_screen(void);
void alerts_add_notification(const char *title, const char *message, const char *app);
void alerts_clear_all(void);
int alerts_get_count(void);
void alerts_refresh_if_active(void);
bool alerts_has_pending_refresh(void);

#endif
