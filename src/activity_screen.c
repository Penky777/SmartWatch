#include "lvgl.h"
#include "ui_manager.h"
#include <stdio.h>
#include "icons/steps_80.h"
#include "icons/heart_19.h"
#include "icons/blood_drip_21.h"
#include "max30102.h"
#include "bsp_qmi8658.h"

static lv_obj_t *steps_label;
static lv_obj_t *bpm_label;
static lv_obj_t *spo2_label;

static lv_timer_t *activity_update_timer;

static void swipe_back_event_cb(lv_event_t *e);
static void activity_update_timer_cb(lv_timer_t *timer);

lv_obj_t *build_activity_screen(void) {
    //max_start();
    
    lv_obj_t *activity_scr = lv_obj_create(NULL);
    
    lv_obj_set_style_bg_color(activity_scr, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(activity_scr, LV_OPA_COVER, 0);

    // Title
    lv_obj_t *title = lv_label_create(activity_scr);
    lv_label_set_text(title, "Activity");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    // Layout constants - all icons are ~80px wide
    int16_t row1_y = 50;   // First row (steps)
    int16_t row2_y = 140;  // Second row (heart/BPM)
    int16_t row3_y = 230;  // Third row (blood/SpO2)
    int16_t icon_x = 20;   // Icon left margin
    int16_t label_x = 130; // Label position (80px icon + 10px gap + 20px margin)

    // Steps section (80×81)
    lv_obj_t *steps_icon = lv_image_create(activity_scr);
    lv_image_set_src(steps_icon, &steps_80);
    lv_obj_align(steps_icon, LV_ALIGN_TOP_LEFT, icon_x, row1_y);

    steps_label = lv_label_create(activity_scr);
    lv_label_set_text(steps_label, "0");
    lv_obj_set_style_text_color(steps_label, lv_color_white(), 0);
    lv_obj_align(steps_label, LV_ALIGN_TOP_LEFT, label_x, row1_y + 30);
    lv_obj_set_style_text_font(steps_label, &lv_font_montserrat_28, 0);

    // BPM section (79×79)
    lv_obj_t *heart_icon = lv_image_create(activity_scr);
    lv_image_set_src(heart_icon, &heart_19);
    lv_obj_align(heart_icon, LV_ALIGN_TOP_LEFT, icon_x, row2_y);

    bpm_label = lv_label_create(activity_scr);
    lv_label_set_text(bpm_label, "0");
    lv_obj_set_style_text_color(bpm_label, lv_color_white(), 0);
    lv_obj_align(bpm_label, LV_ALIGN_TOP_LEFT, label_x, row2_y + 30);
    lv_obj_set_style_text_font(bpm_label, &lv_font_montserrat_28, 0);

    // SpO2 section (80×82)
    lv_obj_t *blood_icon = lv_image_create(activity_scr);
    lv_image_set_src(blood_icon, &blood_drip_21);
    lv_obj_align(blood_icon, LV_ALIGN_TOP_LEFT, icon_x, row3_y);

    spo2_label = lv_label_create(activity_scr);
    lv_label_set_text(spo2_label, "0%");
    lv_obj_set_style_text_color(spo2_label, lv_color_white(), 0);
    lv_obj_align(spo2_label, LV_ALIGN_TOP_LEFT, label_x, row3_y + 30);
    lv_obj_set_style_text_font(spo2_label, &lv_font_montserrat_28, 0);
    
    lv_obj_add_event_cb(activity_scr, swipe_back_event_cb, LV_EVENT_GESTURE, NULL);

    // Start timer to update activity data every second
    activity_update_timer = lv_timer_create(activity_update_timer_cb, 1000, NULL);

    return activity_scr;
    
}

void activity_screen_update(uint16_t steps, uint8_t bpm, uint8_t spo2) {
    if (!steps_label) return;
    
    char buf[32];
    
    snprintf(buf, sizeof(buf), "%u", steps);
    lv_label_set_text(steps_label, buf);

    snprintf(buf, sizeof(buf), "%u", bpm);
    lv_label_set_text(bpm_label, buf);

    snprintf(buf, sizeof(buf), "%u%%", spo2);
    lv_label_set_text(spo2_label, buf);
}

static void activity_update_timer_cb(lv_timer_t *timer) {
    uint32_t steps = bsp_qmi8658_get_software_steps();
    activity_screen_update(steps, 0, 0); // Update steps, keep bpm and spo2 as 0 for now
}

static void swipe_back_event_cb(lv_event_t *e) {
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
    if (dir == LV_DIR_RIGHT) {
        ui_show_menu(); 
    }
}
