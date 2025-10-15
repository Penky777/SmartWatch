#include "lvgl.h"
#include "menu_screen.h"

// ---------- Forward declarations ----------
static void build_brightness_screen(void);
static void brightness_slider_event_cb(lv_event_t *e);
static void settings_swipe_event_cb(lv_event_t *e);

void build_settings_screen(void);

// ---------- Helper: Create a row ----------
static lv_obj_t* create_row(lv_obj_t *parent, const char *title, lv_event_cb_t cb) {
    lv_obj_t *row = lv_btn_create(parent);
    lv_obj_set_size(row, lv_pct(100), 55);
    lv_obj_set_style_bg_color(row, lv_color_hex(0x3a3a3a), 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(row, 20, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_all(row, 10, 0);

    lv_obj_t *lbl = lv_label_create(row);
    lv_label_set_text(lbl, title);
    lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
    lv_obj_set_flex_grow(lbl, 1);

    if (cb != NULL)
        lv_obj_add_event_cb(row, cb, LV_EVENT_CLICKED, NULL);

    return row;
}

// ---------- Brightness slider callback ----------
static void brightness_slider_event_cb(lv_event_t *e) {
    lv_obj_t *slider = lv_event_get_target(e);
    int value = lv_slider_get_value(slider);

    // Example: Update display brightness
    // If your hardware driver supports it:
    // lv_display_set_brightness(NULL, value);

    LV_LOG_USER("Brightness set to: %d", value);
}

// ---------- Brightness screen ----------
static void build_brightness_screen(void) {
    lv_obj_clean(lv_scr_act());
    lv_obj_t *scr = lv_scr_act();

    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    // Title
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "Adjust Brightness");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

    // Slider
    lv_obj_t *slider = lv_slider_create(scr);
    lv_obj_set_size(slider, lv_pct(80), 20);
    lv_obj_align(slider, LV_ALIGN_CENTER, 0, 0);
    lv_slider_set_range(slider, 0, 100);
    lv_slider_set_value(slider, 80, LV_ANIM_OFF); // default brightness
    lv_obj_add_event_cb(slider, brightness_slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
}

// ---------- Swipe back to menu ----------
static void settings_swipe_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_GESTURE) {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
        if (dir == LV_DIR_LEFT || dir == LV_DIR_RIGHT) {
            build_menu_screen();
        }
    }
}

// ---------- Settings screen ----------
void build_settings_screen(void) {
    lv_obj_clean(lv_scr_act());
    lv_obj_t *scr = lv_scr_act();

    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_add_event_cb(scr, settings_swipe_event_cb, LV_EVENT_GESTURE, NULL);

    lv_obj_t *container = lv_obj_create(scr);
    lv_obj_set_width(container, LV_PCT(100));
    lv_obj_set_height(container, LV_SIZE_CONTENT);
    lv_obj_set_scroll_dir(container, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(container, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_snap_y(container, LV_SCROLL_SNAP_NONE);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(container, 10, 0);
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_style_bg_color(container, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(container, LV_OPA_COVER, 0);

    // Title
    lv_obj_t *menu_lbl = lv_label_create(container);
    lv_label_set_text(menu_lbl, "Settings Menu");
    lv_obj_set_style_text_color(menu_lbl, lv_color_white(), 0);
    lv_obj_set_style_text_align(menu_lbl, LV_TEXT_ALIGN_CENTER, 0);

    // Create rows with callbacks
    create_row(container, "Bluetooth", NULL);
    create_row(container, "Brightness", (lv_event_cb_t)build_brightness_screen);
    create_row(container, "Factory reset", NULL);
}
