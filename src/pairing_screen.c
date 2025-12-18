#include "lvgl.h"
#include "ui_manager.h"
#include <stdio.h> 

static lv_obj_t *pair_scr = NULL;
static lv_obj_t *pin_label = NULL;

lv_obj_t *build_pairing_screen(int pin)
{
    pair_scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(pair_scr, lv_color_black(), 0);

    lv_obj_t *title = lv_label_create(pair_scr);
    lv_label_set_text(title, "Bluetooth Pairing");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 15);

    lv_obj_t *info = lv_label_create(pair_scr);
    lv_label_set_text(info, "Pairing Code:\nEnter on phone");
    lv_obj_set_style_text_color(info, lv_color_white(), 0);
    lv_obj_align(info, LV_ALIGN_CENTER, 0, -30);

    pin_label = lv_label_create(pair_scr);
    static char pin_buf[8];
    snprintf(pin_buf, sizeof(pin_buf), "%06d", pin);
    lv_label_set_text(pin_label, pin_buf);
    lv_obj_set_style_text_font(pin_label, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(pin_label, lv_color_white(), 0);
    lv_obj_align(pin_label, LV_ALIGN_CENTER, 0, 20);

    return pair_scr;
}
