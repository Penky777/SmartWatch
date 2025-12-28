#include <stdio.h>
#include <stdlib.h>
#include "lvgl.h"
#include "ui_manager.h"

// Static label reference for updating time (persists across screen loads)
static lv_obj_t *time_label = NULL;

// Forward declarations
static void time_swipe_event_cb(lv_event_t *e);

// Build time screen
lv_obj_t *build_time_screen(void) {
    lv_obj_t *scr = lv_obj_create(NULL);
    
    // Style
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    
    // Swipe gesture to go back to menu
    lv_obj_add_event_cb(scr, time_swipe_event_cb, LV_EVENT_GESTURE, NULL);
    
    // Title
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "Current Time");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    
    // Large time display (centered)
    time_label = lv_label_create(scr);
    lv_label_set_text(time_label, "--:--:--");
    lv_obj_set_style_text_color(time_label, lv_color_white(), 0);
    //lv_obj_set_style_text_font(time_label, &LV_FONT_MONTSERRAT_16, 0);
    lv_obj_center(time_label);
    
    // Hint text
    lv_obj_t *hint = lv_label_create(scr);
    lv_label_set_text(hint, "← Swipe to go back");
    lv_obj_set_style_text_color(hint, lv_color_hex(0x666666), 0);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_14, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -20);
    
    return scr;
}

// Update time display (called from clock task)
void time_screen_update(const char *time_str) {
    if (time_label && time_str) {
        lv_label_set_text(time_label, time_str);
        lv_obj_center(time_label);  // Re-center after text change
    }
}

// Handle swipe back to menu
static void time_swipe_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_GESTURE) {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
        if (dir == LV_DIR_RIGHT) {
            ui_show_menu();
        }
    }
}
