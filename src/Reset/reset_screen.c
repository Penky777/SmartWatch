#include "lvgl.h"
#include "../Ui_manager/ui_manager.h"
#include <stdio.h>

// Forward declarations
static void confirm_btn_event_cb(lv_event_t *e);
static void cancel_btn_event_cb(lv_event_t *e);
static void swipe_back_event_cb(lv_event_t *e);

lv_obj_t *build_reset_screen(void) {
    lv_obj_t *scr = lv_obj_create(NULL);
    
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    
    lv_obj_add_event_cb(scr, swipe_back_event_cb, LV_EVENT_GESTURE, NULL);

    // Title
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "Factory Reset");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFF5555), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

    // Warning message
    lv_obj_t *warning = lv_label_create(scr);
    lv_label_set_text(warning, "This will erase all data\nand settings!");
    lv_obj_set_style_text_color(warning, lv_color_white(), 0);
    lv_obj_set_style_text_font(warning, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_align(warning, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(warning, LV_ALIGN_CENTER, 0, -30);
    lv_obj_set_width(warning, LV_PCT(90));

    // Confirm button
    lv_obj_t *confirm_btn = lv_btn_create(scr);
    lv_obj_set_size(confirm_btn, 180, 50);
    lv_obj_align(confirm_btn, LV_ALIGN_CENTER, 0, 50);
    lv_obj_set_style_bg_color(confirm_btn, lv_color_hex(0xFF3333), 0);
    lv_obj_set_style_radius(confirm_btn, 25, 0);
    lv_obj_add_event_cb(confirm_btn, confirm_btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *confirm_lbl = lv_label_create(confirm_btn);
    lv_label_set_text(confirm_lbl, "RESET");
    lv_obj_set_style_text_color(confirm_lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(confirm_lbl, &lv_font_montserrat_20, 0);
    lv_obj_center(confirm_lbl);

    // Cancel button
    lv_obj_t *cancel_btn = lv_btn_create(scr);
    lv_obj_set_size(cancel_btn, 180, 50);
    lv_obj_align(cancel_btn, LV_ALIGN_CENTER, 0, 110);
    lv_obj_set_style_bg_color(cancel_btn, lv_color_hex(0x555555), 0);
    lv_obj_set_style_radius(cancel_btn, 25, 0);
    lv_obj_add_event_cb(cancel_btn, cancel_btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *cancel_lbl = lv_label_create(cancel_btn);
    lv_label_set_text(cancel_lbl, "Cancel");
    lv_obj_set_style_text_color(cancel_lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(cancel_lbl, &lv_font_montserrat_20, 0);
    lv_obj_center(cancel_lbl);

    // Hint
    lv_obj_t *hint = lv_label_create(scr);
    lv_label_set_text(hint, "← Swipe to go back");
    lv_obj_set_style_text_color(hint, lv_color_hex(0x666666), 0);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_14, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -10);

    return scr;
}

static void confirm_btn_event_cb(lv_event_t *e) {
    (void)e;
    // TODO: Implement actual factory reset logic here
    // For now, just show a confirmation message
    
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    
    lv_obj_t *msg = lv_label_create(scr);
    lv_label_set_text(msg, "Resetting...\nPlease wait");
    lv_obj_set_style_text_color(msg, lv_color_white(), 0);
    lv_obj_set_style_text_font(msg, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_align(msg, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(msg);
    
    lv_scr_load(scr);
    
    // Add your reset logic here:
    // - Clear NVS storage
    // - Reset RTC settings
    // - Clear BLE pairing data
    // - Reboot device
}

static void cancel_btn_event_cb(lv_event_t *e) {
    (void)e;
    ui_show_settings();
}

static void swipe_back_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_GESTURE) {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
        if (dir == LV_DIR_RIGHT) {
            ui_show_settings();
        }
    }
}
