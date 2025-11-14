#include "lvgl.h"
#include "ui_manager.h"
#include <stdio.h> 

static lv_obj_t *steps_label;
static lv_obj_t *bpm_label;
static lv_obj_t *spo2_label;

lv_obj_t *build_activity_screen(void) {
    lv_obj_t *activity_scr = lv_obj_create(NULL);
    
    lv_obj_set_style_bg_color(activity_scr, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(activity_scr, LV_OPA_COVER, 0);

    lv_obj_t *title = lv_label_create(activity_scr);
    lv_label_set_text(title, "Activity");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    steps_label = lv_label_create(activity_scr);
    lv_obj_align(steps_label, LV_ALIGN_CENTER, 0, -40);
    lv_label_set_text(steps_label, "Steps: 0");
    lv_obj_set_style_text_color(steps_label, lv_color_white(), 0);

    bpm_label = lv_label_create(activity_scr);
    lv_obj_align(bpm_label, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(bpm_label, "BPM: 0");
    lv_obj_set_style_text_color(bpm_label, lv_color_white(), 0);

    spo2_label = lv_label_create(activity_scr);
    lv_obj_align(spo2_label, LV_ALIGN_CENTER, 0, 40);
    lv_label_set_text(spo2_label, "SpO2: 0");
    lv_obj_set_style_text_color(spo2_label, lv_color_white(), 0);

    return activity_scr;
}

void activity_screen_update(uint16_t steps, uint8_t bpm, uint8_t spo2) {
    if (!steps_label) return;
    char buf[32];
    snprintf(buf, sizeof(buf), "Steps: %u", steps);
    lv_label_set_text(steps_label, buf);

    snprintf(buf, sizeof(buf), "BPM: %u", bpm);
    lv_label_set_text(bpm_label, buf);

    snprintf(buf, sizeof(buf), "SpO2: %u%%", spo2);
    lv_label_set_text(spo2_label, buf);
}
