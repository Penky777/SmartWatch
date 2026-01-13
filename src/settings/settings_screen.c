#include "lvgl.h"
#include "../Ui_manager/ui_manager.h"
#include "../Brightness/brightness_screen.h"
#include "../Bluetooth/bluetooth.h"

// Forward declarations
static void bluetooth_toggle_event_cb(lv_event_t *e);
static void brightness_row_event_cb(lv_event_t *e);
static void settings_swipe_event_cb(lv_event_t *e);
static void reset_btn_event_cb(lv_event_t *e);

// Helper to create a settings row
static lv_obj_t* create_row(lv_obj_t *parent, const char *title,
                            lv_obj_t *right_widget, lv_event_cb_t cb)
{
    lv_obj_t *row = lv_btn_create(parent);
    lv_obj_set_size(row, lv_pct(100), 55);

    lv_obj_set_style_bg_color(row, lv_color_hex(0x3a3a3a), 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(row, 20, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_outline_width(row, 0, 0);
    lv_obj_set_style_shadow_width(row, 0, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row,
        LV_FLEX_ALIGN_START,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER);

    lv_obj_set_style_pad_left(row, 18, 0);
    lv_obj_set_style_pad_right(row, 18, 0);
    lv_obj_set_style_pad_column(row, 20, 0);

    // Label on the left
    lv_obj_t *lbl = lv_label_create(row);
    lv_label_set_text(lbl, title);
    lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
    lv_obj_set_flex_grow(lbl, 1);

    // Add right widget if provided
    if (right_widget) {
        lv_obj_set_parent(right_widget, row);
    }

    // Make row clickable if callback provided
    if (cb) {
        lv_obj_add_event_cb(row, cb, LV_EVENT_CLICKED, NULL);
    }

    return row;
}

// Bluetooth toggle callback
static void bluetooth_toggle_event_cb(lv_event_t *e)
{
    lv_obj_t *sw = lv_event_get_target(e);

    if (lv_obj_has_state(sw, LV_STATE_CHECKED)) {
        bluetooth_enable();
    } else {
        bluetooth_disable();
    }
}

// Brightness row callback
static void brightness_row_event_cb(lv_event_t *e)
{
    (void)e;
    ui_show_brightness();
}

// Swipe right to go back to menu
static void settings_swipe_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_GESTURE) {
        if (lv_indev_get_gesture_dir(lv_indev_get_act()) == LV_DIR_RIGHT) {
            ui_show_menu();
        }
    }
}

// Reset button callback
static void reset_btn_event_cb(lv_event_t *e)
{
    (void)e;
    ui_show_reset();
}

// Build Settings screen (called once by UI manager)
lv_obj_t *build_settings_screen(void)
{
    // REMOVED: if (settings_scr) return settings_scr;
    // UI manager handles caching now
    
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_add_event_cb(screen, settings_swipe_event_cb, LV_EVENT_GESTURE, NULL);

    // Vertical list layout
    lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(screen,
        LV_FLEX_ALIGN_START,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(screen, 14, 0);
    lv_obj_set_style_pad_top(screen, 20, 0);

    // Title
    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "Settings");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);

    // Bluetooth Row with Switch
    // Create switch INSIDE create_row by passing it as right_widget
    lv_obj_t *bt_switch = lv_switch_create(screen);
    lv_obj_clear_state(bt_switch, LV_STATE_CHECKED);  // Default OFF
    lv_obj_add_event_cb(bt_switch, bluetooth_toggle_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    
    create_row(screen, "Bluetooth", bt_switch, NULL);

    // Brightness Row (no widget, clickable)
    create_row(screen, "Brightness", NULL, brightness_row_event_cb);

    // Factory Reset Row (no widget, clickable)
    create_row(screen, "Factory Reset", NULL, reset_btn_event_cb);

    return screen;
}
