#include "time_screen.h"
#include "lvgl.h"

static lv_obj_t *time_scr = NULL;
static lv_obj_t *time_label = NULL;

static lv_obj_t *main_menu_scr = NULL; // Store menu screen to go back

// Forward declaration for Back button event
static void back_btn_event_cb(lv_event_t *e);

void time_screen_init(void) {
    // Create the time screen
    time_scr = lv_obj_create(NULL);
    lv_obj_set_size(time_scr, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(time_scr, lv_color_black(), 0);

    // Create the centered time label
    time_label = lv_label_create(time_scr);
    lv_label_set_text(time_label, "--:--:--");
    lv_obj_center(time_label);

    // Back button
    lv_obj_t *back_btn = lv_btn_create(time_scr);
    lv_obj_set_size(back_btn, 80, 40);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_t *lbl = lv_label_create(back_btn);
    lv_label_set_text(lbl, "Back");
    lv_obj_center(lbl);
    lv_obj_add_event_cb(back_btn, back_btn_event_cb, LV_EVENT_CLICKED, NULL);
}

// Update the time
void time_screen_update(const char *time_str) {
    if(time_label) {
        lv_label_set_text(time_label, time_str);
        lv_obj_center(time_label);
    }
}

// Event callback for the Time button in menu
void time_btn_event_cb(lv_event_t *e) {
    LV_UNUSED(e);
    // Store current screen as main menu
    main_menu_scr = lv_scr_act();
    // Load time screen
    lv_scr_load(time_scr);
}

// Back button callback
static void back_btn_event_cb(lv_event_t *e) {
    LV_UNUSED(e);
    if(main_menu_scr) {
        lv_scr_load(main_menu_scr);
    }
}
