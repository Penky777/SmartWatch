#include "lvgl.h"
#include "ui_manager.h"
#include "brightness_screen.h"

// ---------- Static screen handles ----------
static lv_obj_t *settings_scr = NULL;

// ---------- Forward declarations ----------
static void brightness_row_event_cb(lv_event_t *e);
static void settings_swipe_event_cb(lv_event_t *e);

// ---------- Helper: Create a row ----------
static lv_obj_t* create_row(lv_obj_t *parent, const char *title, lv_event_cb_t cb) {
    lv_obj_t *row = lv_btn_create(parent);
    lv_obj_set_size(row, lv_pct(100), 55);
    lv_obj_set_style_bg_color(row, lv_color_hex(0x3a3a3a), 0);
    lv_obj_set_style_radius(row, 20, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl = lv_label_create(row);
    lv_label_set_text(lbl, title);
    lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
    lv_obj_center(lbl);

    if (cb)
        lv_obj_add_event_cb(row, cb, LV_EVENT_CLICKED, NULL);

    return row;
}

// ---------- Swipe: Back to menu ----------
static void settings_swipe_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_GESTURE) {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
        if (dir == LV_DIR_RIGHT) {
            ui_show_menu();
        }
    }
}


// ---------- Brightness row ----------
static void brightness_row_event_cb(lv_event_t *e) {
    (void)e;
    ui_show_brightness();  // show brightness screen via manager
}

// ---------- Build Settings screen ----------
lv_obj_t *build_settings_screen(void) {
    if (settings_scr) return settings_scr; // reuse

    settings_scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(settings_scr, lv_color_black(), 0);
    lv_obj_add_event_cb(settings_scr, settings_swipe_event_cb, LV_EVENT_GESTURE, NULL);

    lv_obj_set_flex_flow(settings_scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(settings_scr,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(settings_scr, 12, 0);

    lv_obj_t *lbl = lv_label_create(settings_scr);
    lv_label_set_text(lbl, "Settings");
    lv_obj_set_style_text_color(lbl, lv_color_white(), 0);

    create_row(settings_scr, "Bluetooth", NULL);
    create_row(settings_scr, "Brightness", brightness_row_event_cb);
    create_row(settings_scr, "Factory Reset", NULL);

    return settings_scr;
}
