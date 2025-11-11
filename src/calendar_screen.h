#ifndef MENU_SCREEN_H
#define MENU_SCREEN_H

#include "lvgl.h"

void build_event_detail_screen(lv_obj_t *scr, const char *date);
void calendar_swipe_event_cb(lv_event_t *e);
void date_row_event_cb(lv_event_t *e);

void build_calendar_screen(lv_obj_t *scr);

#endif