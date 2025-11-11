#include "lvgl.h"
#include "ui_manager.h"
#include "icons/Calendar.h"
#include "icons/Activity.h"
#include "icons/Settings.h"
#include "icons/Calculator.h"

// ---------- Forward declarations ----------
lv_obj_t *build_menu_screen(void);

// ---------- Event callbacks (NEW) ----------
static void calendar_btn_event_cb(lv_event_t *e) { (void)e; ui_show_calendar(); }
static void activity_btn_event_cb(lv_event_t *e) { (void)e; ui_show_activity(); }
static void settings_btn_event_cb(lv_event_t *e) { (void)e; ui_show_settings(); }
static void calculator_btn_event_cb(lv_event_t *e) { (void)e; ui_show_calculator(); }

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
    static lv_obj_t *menu_scr = NULL;
    if (menu_scr) return menu_scr;

    menu_scr = lv_obj_create(NULL);
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

    create_menu_button(menu_scr, &Calendar, "Calendar", calendar_btn_event_cb);
    create_menu_button(menu_scr, &Activity, "Activity", activity_btn_event_cb);
    create_menu_button(menu_scr, &Settings, "Settings", settings_btn_event_cb);
    create_menu_button(menu_scr, &Calculator, "Calculator", calculator_btn_event_cb);

    lv_obj_scroll_to_y(menu_scr, 0, LV_ANIM_OFF);
    return menu_scr;
}
