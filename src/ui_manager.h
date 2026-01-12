#pragma once
#include "lvgl.h"

void ui_manager_init(void);

void ui_show_watchface(void);
void ui_show_menu(void);
void ui_show_menu_no_anim(void);
void ui_show_calendar(void);
void ui_show_activity(void);
void ui_show_settings(void);
void ui_show_calculator(void);
void ui_show_brightness(void);
void ui_show_reset(void);
lv_obj_t *build_pairing_screen(int pin);
void ui_show_pairing(int pin);
void ui_hide_pairing(void);
void ui_show_flashlight(void);


void ui_go_back(void);
bool ui_can_go_back(void);

