#include "lvgl.h"

lv_obj_t *build_calculator_screen(void) {
    static lv_obj_t *calc_scr = NULL;
    if (calc_scr) return calc_scr;

    calc_scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(calc_scr, lv_color_black(), 0);

    lv_obj_t *lbl = lv_label_create(calc_scr);
    lv_label_set_text(lbl, "Calculator (coming soon)");
    lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
    lv_obj_center(lbl);

    return calc_scr;
}
