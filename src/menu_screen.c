#include "lvgl.h"
#include "icons/Calendar.h"
#include "icons/Activity.h"
#include "icons/Settings.h"
#include "icons/Calculator.h"

// Forward declarations
void build_settings_screen(lv_obj_t *scr);
void build_menu_screen(lv_obj_t *scr);

// ---------- Event callbacks ----------

// Settings button
static void settings_btn_event_cb(lv_event_t *e) {
    // Create a *new* LVGL screen for Settings
    lv_obj_t *settings_scr = lv_obj_create(NULL);
    build_settings_screen(settings_scr);
    lv_scr_load(settings_scr);   // Switch to the new screen
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

// ---------- Helper: Create a menu button ----------
static lv_obj_t* create_menu_button(lv_obj_t *parent, const void *icon_src, const char *label_text, lv_event_cb_t event_cb) {
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 120, LV_SIZE_CONTENT); // height grows automatically
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

    // Completely remove visual styles
    lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_outline_width(btn, 0, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_set_style_pad_all(btn, 0, 0);
    lv_obj_set_style_clip_corner(btn, false, 0); // prevent clipping

    // Flex layout to stack icon + text
    lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(btn,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER
    );
    lv_obj_set_style_pad_row(btn, 6, 0); // small gap between icon and text

    // Event handler
    if (event_cb)
        lv_obj_add_event_cb(btn, event_cb, LV_EVENT_CLICKED, NULL);

    // Icon — use natural image size
    lv_obj_t *icon = lv_img_create(btn);
    lv_img_set_src(icon, icon_src);
    lv_obj_clear_flag(icon, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_pad_all(icon, 0, 0);
    lv_obj_center(icon);

    // Label
    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, label_text);
    lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(lbl, LV_PCT(100));
    lv_obj_center(lbl);

    return btn;
}

// ---------- Main menu screen ----------
void build_menu_screen(lv_obj_t *scr) {
    // Make the *screen* scrollable
    lv_obj_set_scroll_dir(scr, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(scr, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);

    // Flex layout on the screen
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr,
        LV_FLEX_ALIGN_START,   // top align vertically 
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER
    );

    // Menu title
    lv_obj_t *menu_lbl = lv_label_create(scr);
    lv_label_set_text(menu_lbl, "Menu");
    lv_obj_set_style_text_color(menu_lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(menu_lbl, &lv_font_montserrat_28, 0);

    // Buttons
    create_menu_button(scr, &Calendar, "Calendar", calendar_btn_event_cb);
    create_menu_button(scr, &Activity, "Activity", activity_btn_event_cb);
    create_menu_button(scr, &Settings, "Settings", settings_btn_event_cb);
    create_menu_button(scr, &Calculator, "Calculator", calculator_btn_event_cb);

    // Make sure scroll starts at top
    lv_obj_scroll_to_y(scr, 0, LV_ANIM_OFF);
}