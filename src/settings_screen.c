#include "lvgl.h"
#include "ui_manager.h"
#include "brightness_screen.h"
#include "bluetooth.h"

// ---------- Static screen handle ----------
static lv_obj_t *settings_scr = NULL;

// ---------- Forward declarations ----------
static void bluetooth_toggle_event_cb(lv_event_t *e);
static void brightness_row_event_cb(lv_event_t *e);
static void settings_swipe_event_cb(lv_event_t *e);


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
        LV_FLEX_ALIGN_START,   //  (left to right)
        LV_FLEX_ALIGN_CENTER,  //  (vertical center)
        LV_FLEX_ALIGN_CENTER);

    lv_obj_set_style_pad_left(row, 18, 0);
    lv_obj_set_style_pad_right(row, 18, 0);
    lv_obj_set_style_pad_column(row, 20, 0);

    // Label on the left
    lv_obj_t *lbl = lv_label_create(row);
    lv_label_set_text(lbl, title);
    lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
    lv_obj_set_flex_grow(lbl, 1);   // takes all free space, pushes switch to the right

    
    if (right_widget) {
        lv_obj_set_parent(right_widget, row);
    }

    // Whole row clickable
    if (cb)
        lv_obj_add_event_cb(row, cb, LV_EVENT_CLICKED, NULL);

    return row;
}


// Bluetooth toggle callback

static void bluetooth_toggle_event_cb(lv_event_t *e)
{
    lv_obj_t *sw = lv_event_get_target(e);

    if (lv_obj_has_state(sw, LV_STATE_CHECKED))
        bluetooth_enable();
    else
        bluetooth_disable();
}


// Brightness row callback

static void brightness_row_event_cb(lv_event_t *e)
{
    (void)e;
    ui_show_brightness();
}


// Swipe: Back to menu

static void settings_swipe_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_GESTURE) {
        if (lv_indev_get_gesture_dir(lv_indev_get_act()) == LV_DIR_RIGHT)
            ui_show_menu();
    }
}


// Build Settings screen

lv_obj_t *build_settings_screen(void)
{
    if (settings_scr) return settings_scr;   // reuse existing

    settings_scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(settings_scr, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(settings_scr, LV_OPA_COVER, 0);
    lv_obj_add_event_cb(settings_scr, settings_swipe_event_cb, LV_EVENT_GESTURE, NULL);

    // Vertical list of rows
    lv_obj_set_flex_flow(settings_scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(settings_scr,
        LV_FLEX_ALIGN_START,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(settings_scr, 14, 0);

    // Title 
    lv_obj_t *title = lv_label_create(settings_scr);
    lv_label_set_text(title, "Settings");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);

    // Bluetooth Row with Switch 
    lv_obj_t *sw = lv_switch_create(settings_scr);

    // Default: OFF at startup
    lv_obj_clear_state(sw, LV_STATE_CHECKED);

    lv_obj_add_event_cb(sw, bluetooth_toggle_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    create_row(settings_scr, "Bluetooth", sw, NULL);

    //  Brightness Row 
    create_row(settings_scr, "Brightness", NULL, brightness_row_event_cb);

    //  Factory Reset Row 
    create_row(settings_scr, "Factory Reset", NULL, NULL);

    return settings_scr;
}
