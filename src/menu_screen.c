#include "lvgl.h"
#include "icons/Calendar.h"
#include "icons/Activity.h"
#include "icons/Settings.h"
#include "icons/Calculator.h"

// Forward declaration
void build_settings_screen(void);

// ---------- Event callbacks ----------

// Settings button
static void settings_btn_event_cb(lv_event_t *e) {
    // Clear current screen first
    lv_obj_clean(lv_scr_act());
    build_settings_screen();
}

// Placeholder callbacks for other icons
static void calendar_btn_event_cb(lv_event_t *e) {
    LV_LOG_USER("Calendar pressed");
}

static void activity_btn_event_cb(lv_event_t *e) {
    LV_LOG_USER("Activity pressed");
}

static void calculator_btn_event_cb(lv_event_t *e) {
    LV_LOG_USER("Calculator pressed");
}

// ---------- Helper: Create a menu button with icon + label ----------
static lv_obj_t* create_menu_button(lv_obj_t *parent, const void *icon_src, const char *label_text, lv_event_cb_t event_cb) {
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 100, 120);
    lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(btn, 0, 0);
    lv_obj_set_style_pad_row(btn, 0, 0);

    lv_obj_add_event_cb(btn, event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *icon = lv_img_create(btn);
    lv_img_set_src(icon, icon_src);
    lv_obj_set_size(icon, 90, 90);
    lv_obj_center(icon);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, label_text);
    lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);

    return btn;
}

// ---------- Main menu screen ----------
void build_menu_screen(void) {
    // Make sure screen is clean before building
    lv_obj_clean(lv_scr_act());

    lv_obj_t *scr = lv_scr_act();

    // Background
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    // Container
    lv_obj_t *container = lv_obj_create(scr);
    lv_obj_set_size(container, LV_PCT(100), LV_PCT(100));
    lv_obj_set_scroll_dir(container, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(container, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(container, 10, 0);
    lv_obj_set_style_bg_color(container, lv_color_black(), 0);
    lv_obj_set_style_border_width(container, 0, 0);

    // Menu title
    lv_obj_t *menu_lbl = lv_label_create(container);
    lv_label_set_text(menu_lbl, "Menu");
    lv_obj_set_style_text_color(menu_lbl, lv_color_white(), 0);
    lv_obj_set_style_text_align(menu_lbl, LV_TEXT_ALIGN_CENTER, 0);

    // Buttons
    create_menu_button(container, &Calendar, "Calendar", calendar_btn_event_cb);
    create_menu_button(container, &Activity, "Activity", activity_btn_event_cb);
    create_menu_button(container, &Settings, "Settings", settings_btn_event_cb);
    create_menu_button(container, &Calculator, "Calculator", calculator_btn_event_cb);
}
