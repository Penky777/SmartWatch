#include "lvgl.h"
#include "../Ui_manager/ui_manager.h"
#include "../icons/Calendar.h"
#include "../icons/Activity.h"
#include "../icons/Settings.h"
#include "../icons/sun.h"
#include "../icons/Bell.h"

// ---------- Forward declarations ----------
lv_obj_t *build_menu_screen(void);
static void menu_gesture_event_cb(lv_event_t *e);

// ---------- Event callbacks  ----------
static void calendar_btn_event_cb(lv_event_t *e) { (void)e; ui_show_calendar(); }
static void activity_btn_event_cb(lv_event_t *e) { (void)e; ui_show_activity(); }
static void settings_btn_event_cb(lv_event_t *e) { (void)e; ui_show_settings(); }
static void flashlight_btn_event_cb(lv_event_t *e) {(void)e; ui_show_flashlight();}
static void notifications_btn_event_cb(lv_event_t *e){(void)e; ui_show_alerts();}

// ---------- Helper: Create a menu button ----------
static lv_obj_t* create_menu_button(lv_obj_t *parent, const void *icon_src, const char *label_text, lv_event_cb_t event_cb) {
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 120, LV_SIZE_CONTENT);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_set_style_pad_all(btn, 0, 0);

    lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(btn,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER
    );
    lv_obj_set_style_pad_row(btn, 6, 0);

    if (event_cb)
        lv_obj_add_event_cb(btn, event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *icon = lv_img_create(btn);
    lv_img_set_src(icon, icon_src);
    lv_obj_clear_flag(icon, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_center(icon);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, label_text);
    lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(lbl, LV_PCT(100));
    lv_obj_center(lbl);

    return btn;
}

// ---------- Menu screen ----------
lv_obj_t *build_menu_screen(void) {
    lv_obj_t *menu_scr = lv_obj_create(NULL);
    
    lv_obj_set_scroll_dir(menu_scr, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(menu_scr, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(menu_scr, lv_color_black(), 0);

    lv_obj_set_flex_flow(menu_scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(menu_scr,
        LV_FLEX_ALIGN_START,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER
    );

    lv_obj_t *menu_lbl = lv_label_create(menu_scr);
    lv_label_set_text(menu_lbl, "Menu");
    lv_obj_set_style_text_color(menu_lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(menu_lbl, &lv_font_montserrat_28, 0);

    // Swipe left-to-right to return to watchface
    lv_obj_add_event(menu_scr, menu_gesture_event_cb, LV_EVENT_GESTURE, NULL);

    create_menu_button(menu_scr, &Calendar, "Calendar", calendar_btn_event_cb);
    create_menu_button(menu_scr, &Activity, "Activity", activity_btn_event_cb);
    create_menu_button(menu_scr, &Settings, "Settings", settings_btn_event_cb);
    create_menu_button(menu_scr, &Sun, "Flashlight",flashlight_btn_event_cb);
    create_menu_button(menu_scr, &Bell, "Notifications", notifications_btn_event_cb);

    lv_obj_scroll_to_y(menu_scr, 0, LV_ANIM_OFF);
    return menu_scr;
}

// Gesture handler for the menu screen: left-to-right goes back to watchface
static void menu_gesture_event_cb(lv_event_t *e) {
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
    if (dir == LV_DIR_RIGHT) {
        lv_event_stop_processing(e);  // Stop event from propagating to children
        lv_indev_reset(lv_indev_get_act(), NULL);  // Clear input state to prevent phantom clicks
        ui_show_watchface();
    }
}
