#include <stdlib.h>       // for abs()
#include "time_screen.h"
#include "lvgl.h"

static lv_obj_t *time_scr = NULL;
static lv_obj_t *time_label = NULL;
static lv_obj_t *main_menu_scr = NULL; // store the menu screen

// Swipe tracking
static lv_point_t start_point;
static lv_point_t end_point;

// Forward declarations
static void back_btn_event_cb(lv_event_t *e);
static void swipe_event_cb(lv_event_t *e);

void time_screen_init(void) {
    // Create the time screen
    time_scr = lv_obj_create(NULL);
    lv_obj_set_size(time_scr, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(time_scr, lv_color_black(), 0);

    // Centered time label
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

    // Attach swipe events
    lv_obj_add_event_cb(time_scr, swipe_event_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(time_scr, swipe_event_cb, LV_EVENT_RELEASED, NULL);
}

// Update the time on the screen
void time_screen_update(const char *time_str) {
    if(time_label) {
        lv_label_set_text(time_label, time_str);
        lv_obj_center(time_label);
    }
}

// Time button in menu pressed
void time_btn_event_cb(lv_event_t *e) {
    LV_UNUSED(e);
    main_menu_scr = lv_scr_act();  
    lv_scr_load(time_scr);         
}

// Swipe detection (right) to go back
static void swipe_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_indev_t *indev = lv_event_get_indev(e);
    lv_point_t p;

    if(!indev) return;
    lv_indev_get_point(indev, &p);

    if(code == LV_EVENT_PRESSED) {
        start_point = p;
    } else if(code == LV_EVENT_RELEASED) {
        end_point = p;
        int dx = end_point.x - start_point.x;
        int dy = end_point.y - start_point.y;

        // Horizontal swipe threshold
        if(abs(dx) > abs(dy) && abs(dx) > 30) {
            if(main_menu_scr) {
                lv_scr_load(main_menu_scr);
            }
        }
    }
}
