#include "lvgl.h"
#include "../Ui_manager/ui_manager.h"
#include "../backlight.h"

// Forward declarations
static void brightness_slider_event_cb(lv_event_t *e);
static void swipe_back_event_cb(lv_event_t *e);

lv_obj_t *build_brightness_screen(void) {
    lv_obj_t *scr = lv_obj_create(NULL);

    // --- Style cleanup ---
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(scr, 0, 0);
    lv_obj_set_style_outline_width(scr, 0, 0);
    lv_obj_set_style_pad_all(scr, 0, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    // --- Title ---
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "Adjust Brightness");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 25);

    // --- Slider ---
    lv_obj_t *slider = lv_slider_create(scr);
    lv_obj_set_size(slider, 200, 20);
    lv_obj_align(slider, LV_ALIGN_CENTER, 0, 0);
    lv_slider_set_range(slider, 5, 100);
    lv_slider_set_value(slider, 100, LV_ANIM_OFF);
    lv_obj_add_event_cb(slider, brightness_slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // --- Swipe gesture (to go back) ---
    lv_obj_add_event_cb(scr, swipe_back_event_cb, LV_EVENT_GESTURE, NULL);

    return scr;
}

// Handle brightness change
static void brightness_slider_event_cb(lv_event_t *e) {
    lv_obj_t *slider = lv_event_get_target(e);
    int val = lv_slider_get_value(slider);
    backlight_set((uint8_t)val);
}

// Handle right-swipe back to settings
static void swipe_back_event_cb(lv_event_t *e) {
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
    if (dir == LV_DIR_RIGHT) {
        ui_show_settings(); // go back smoothly
    }
}
