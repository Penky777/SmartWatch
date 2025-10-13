#include "lvgl.h"
#include "icons/Calendar.h"
#include "icons/Activity.h"
#include "icons/Settings.h"
#include "icons/Calculator.h"

// Forward declaration for build_settings_screen
void build_settings_screen(void);

// Button event handler for settings button
static void settings_btn_event_cb(lv_event_t *e) {
    // Change to settings screen
    build_settings_screen();
}

void build_menu_screen(void) {
    lv_obj_t *scr = lv_scr_act();

    // Set black background
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    lv_obj_t *container = lv_obj_create(scr);
    lv_obj_set_width(container, LV_PCT(100));
    lv_obj_set_height(container, LV_SIZE_CONTENT);
    lv_obj_set_scroll_dir(container, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(container, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_snap_y(container, LV_SCROLL_SNAP_NONE);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(container, 10, 0);
    lv_obj_set_style_pad_column(container, 0, 0);
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_style_bg_color(container, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(container, LV_OPA_COVER, 0);

    // Menu label
    lv_obj_t *menu_lbl = lv_label_create(container);
    lv_label_set_text(menu_lbl, "Menu");
    lv_obj_set_style_text_color(menu_lbl, lv_color_white(), 0);
    lv_obj_set_style_text_align(menu_lbl, LV_TEXT_ALIGN_CENTER, 0);

    // Calendar icon
    lv_obj_t *calendar_img = lv_img_create(container);
    lv_img_set_src(calendar_img, &Calendar);
    lv_obj_set_size(calendar_img, 90, 90);
    lv_obj_center(calendar_img);

    lv_obj_t *calendar_lbl = lv_label_create(container);
    lv_label_set_text(calendar_lbl, "Calendar");
    lv_obj_set_style_text_color(calendar_lbl, lv_color_white(), 0);
    lv_obj_set_style_text_align(calendar_lbl, LV_TEXT_ALIGN_CENTER, 0);

    // Activity icon
    lv_obj_t *activity = lv_img_create(container);
    lv_img_set_src(activity, &Activity);
    lv_obj_set_size(activity, 90, 90);
    lv_obj_center(activity);

    lv_obj_t *activity_lbl = lv_label_create(container);
    lv_label_set_text(activity_lbl, "Activity");
    lv_obj_set_style_text_color(activity_lbl, lv_color_white(), 0);
    lv_obj_set_style_text_align(activity_lbl, LV_TEXT_ALIGN_CENTER, 0);

    // Settings icon
    lv_obj_t *settings = lv_img_create(container);
    lv_img_set_src(settings, &Settings);
    lv_obj_set_size(settings, 90, 90);
    lv_obj_center(settings);

    lv_obj_t *settings_lbl = lv_label_create(container);
    lv_label_set_text(settings_lbl, "Settings");
    lv_obj_set_style_text_color(settings_lbl, lv_color_white(), 0);
    lv_obj_set_style_text_align(settings_lbl, LV_TEXT_ALIGN_CENTER, 0);

    // Invisible button behind settings icon
    lv_obj_t *settings_btn = lv_btn_create(container);
    lv_obj_set_size(settings_btn, 90, 90);
    lv_obj_set_style_bg_opa(settings_btn, LV_OPA_TRANSP, 0); // Make button invisible
    // Button event handler
    lv_obj_add_event_cb(settings_btn, settings_btn_event_cb, LV_EVENT_CLICKED, NULL);

    // Calculator icon
    lv_obj_t *calculator = lv_img_create(container);
    lv_img_set_src(calculator, &Calculator);
    lv_obj_set_size(calculator, 90, 90);
    lv_obj_center(calculator);

    lv_obj_t *calculator_lbl = lv_label_create(container);
    lv_label_set_text(calculator_lbl, "Calculator");
    lv_obj_set_style_text_color(calculator_lbl, lv_color_white(), 0);
    lv_obj_set_style_text_align(calculator_lbl, LV_TEXT_ALIGN_CENTER, 0);
}
