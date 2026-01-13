#include "lvgl.h"
#include "../Ui_manager/ui_manager.h"
#include <stdio.h>
#include "../icons/steps_80.h"
#include "../icons/heart_19.h"
#include "../icons/blood_drip_21.h"
#include "../Max30102/max30102.h"
#include "../Bsp_qmi/bsp_qmi8658.h"
#include "esp_log.h"

static const char *TAG = "ACTIVITY_SCREEN";

// UI element references (persist across screen loads)
static lv_obj_t *steps_label = NULL;
static lv_obj_t *bpm_label = NULL;
static lv_obj_t *spo2_label = NULL;

// Timer for periodic updates
static lv_timer_t *activity_update_timer = NULL;

// Track if screen is active
static bool screen_active = false;

// Forward declarations
static void swipe_back_event_cb(lv_event_t *e);
static void activity_update_timer_cb(lv_timer_t *timer);

lv_obj_t *build_activity_screen(void) {
    lv_obj_t *scr = lv_obj_create(NULL);
    
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    // Title
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "Activity");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 15);

    // Layout constants
    int16_t row1_y = 60;
    int16_t row2_y = 140;
    int16_t row3_y = 220;
    int16_t icon_x = 0;
    int16_t label_x = 120;

    // ========== Steps Section ==========
    lv_obj_t *steps_icon = lv_image_create(scr);
    lv_image_set_src(steps_icon, &steps_80);
    lv_obj_align(steps_icon, LV_ALIGN_TOP_LEFT, icon_x, row1_y);

    steps_label = lv_label_create(scr);
    lv_label_set_text(steps_label, "---");
    lv_obj_set_style_text_color(steps_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(steps_label, &lv_font_montserrat_28, 0);
    lv_obj_align(steps_label, LV_ALIGN_TOP_LEFT, label_x, row1_y + 25);
    
    lv_obj_t *steps_unit = lv_label_create(scr);
    lv_label_set_text(steps_unit, "steps");
    lv_obj_set_style_text_color(steps_unit, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(steps_unit, &lv_font_montserrat_14, 0);
    lv_obj_align(steps_unit, LV_ALIGN_TOP_LEFT, label_x, row1_y + 55);

    // ========== Heart Rate Section ==========
    lv_obj_t *heart_icon = lv_image_create(scr);
    lv_image_set_src(heart_icon, &heart_19);
    lv_obj_align(heart_icon, LV_ALIGN_TOP_LEFT, icon_x, row2_y + 5);

    bpm_label = lv_label_create(scr);
    lv_label_set_text(bpm_label, "---");
    lv_obj_set_style_text_color(bpm_label, lv_color_hex(0xFF5555), 0);
    lv_obj_set_style_text_font(bpm_label, &lv_font_montserrat_28, 0);
    lv_obj_align(bpm_label, LV_ALIGN_TOP_LEFT, label_x, row2_y + 25);
    
    lv_obj_t *bpm_unit = lv_label_create(scr);
    lv_label_set_text(bpm_unit, "BPM");
    lv_obj_set_style_text_color(bpm_unit, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(bpm_unit, &lv_font_montserrat_14, 0);
    lv_obj_align(bpm_unit, LV_ALIGN_TOP_LEFT, label_x, row2_y + 55);

    // ========== SpO2 Section ==========
    lv_obj_t *blood_icon = lv_image_create(scr);
    lv_image_set_src(blood_icon, &blood_drip_21);
    lv_obj_align(blood_icon, LV_ALIGN_TOP_LEFT, icon_x, row3_y + 5);

    spo2_label = lv_label_create(scr);
    lv_label_set_text(spo2_label, "---%");
    lv_obj_set_style_text_color(spo2_label, lv_color_hex(0x55AAFF), 0);
    lv_obj_set_style_text_font(spo2_label, &lv_font_montserrat_28, 0);
    lv_obj_align(spo2_label, LV_ALIGN_TOP_LEFT, label_x, row3_y + 25);
    
    lv_obj_t *spo2_unit = lv_label_create(scr);
    lv_label_set_text(spo2_unit, "SpO2");
    lv_obj_set_style_text_color(spo2_unit, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(spo2_unit, &lv_font_montserrat_14, 0);
    lv_obj_align(spo2_unit, LV_ALIGN_TOP_LEFT, label_x, row3_y + 55);

    // Swipe gesture
    lv_obj_add_event_cb(scr, swipe_back_event_cb, LV_EVENT_GESTURE, NULL);

    // Mark screen as active
    screen_active = true;
    
    ESP_LOGI(TAG, "Activity screen opened");
    
    // Start MAX30102 measurements
    max_start();

    // Create or resume timer (only once)
    if (activity_update_timer == NULL) {
        activity_update_timer = lv_timer_create(activity_update_timer_cb, 1000, NULL);
        ESP_LOGI(TAG, "Created activity update timer");
    } else {
        lv_timer_resume(activity_update_timer);
        ESP_LOGI(TAG, "Resumed activity update timer");
    }

    // Force immediate update
    activity_update_timer_cb(NULL);

    return scr;
}

// Update UI with sensor data
void activity_screen_update(uint16_t steps, uint8_t bpm, uint8_t spo2) {
    // Only update if screen is active and labels exist
    if (!screen_active || !steps_label || !bpm_label || !spo2_label) {
        return;
    }
    
    char buf[32];
    
    ESP_LOGI(TAG, "Update: Steps=%u, BPM=%u, SpO2=%u", steps, bpm, spo2);
    
    // Update steps
    snprintf(buf, sizeof(buf), "%u", steps);
    lv_label_set_text(steps_label, buf);

    // Update heart rate
    if (bpm > 0 && bpm < 220) {
        snprintf(buf, sizeof(buf), "%u", bpm);
        lv_label_set_text(bpm_label, buf);
        lv_obj_set_style_text_color(bpm_label, lv_color_hex(0xFF5555), 0);
    } else {
        lv_label_set_text(bpm_label, "---");
        lv_obj_set_style_text_color(bpm_label, lv_color_hex(0x666666), 0);
    }

    // Update SpO2
    if (spo2 >= 70 && spo2 <= 100) {
        snprintf(buf, sizeof(buf), "%u%%", spo2);
        lv_label_set_text(spo2_label, buf);
        
        // Color based on value
        if (spo2 >= 95) {
            lv_obj_set_style_text_color(spo2_label, lv_color_hex(0x55FF55), 0);  // Green
        } else if (spo2 >= 90) {
            lv_obj_set_style_text_color(spo2_label, lv_color_hex(0xFFAA00), 0);  // Orange
        } else {
            lv_obj_set_style_text_color(spo2_label, lv_color_hex(0xFF5555), 0);  // Red
        }
    } else {
        lv_label_set_text(spo2_label, "--%%");
        lv_obj_set_style_text_color(spo2_label, lv_color_hex(0x666666), 0);
    }
}

// Timer callback - reads sensors and updates UI
static void activity_update_timer_cb(lv_timer_t *timer) {
    (void)timer;
    
    if (!screen_active) {
        ESP_LOGD(TAG, "Timer callback but screen inactive");
        return;
    }
    
    // Read step count from IMU
    uint32_t steps = bsp_qmi8658_get_software_steps();
    
    // Read heart rate and SpO2 from MAX30102
    uint8_t bpm = 0, spo2 = 0;
    esp_err_t ret = max_read(&spo2, &bpm);
    
    if (ret == ESP_OK) {
        ESP_LOGD(TAG, "MAX30102 read: BPM=%u, SpO2=%u", bpm, spo2);
    } else {
        ESP_LOGW(TAG, "MAX30102 read failed");
        bpm = 0;
        spo2 = 0;
    }
    
    // Update display
    activity_screen_update(steps, bpm, spo2);
}

// Swipe right to go back
static void swipe_back_event_cb(lv_event_t *e) {
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
    if (dir == LV_DIR_RIGHT) {
        ESP_LOGI(TAG, "Swipe back detected");
        
        // Mark screen as inactive
        screen_active = false;
        
        // Pause timer (don't delete - screen is cached)
        if (activity_update_timer) {
            lv_timer_pause(activity_update_timer);
            ESP_LOGI(TAG, "Paused activity update timer");
        }
        
        // Stop MAX30102 to save power
        max_stop();
        ESP_LOGI(TAG, "Stopped MAX30102");
        
        // Navigate back
        ui_show_menu();
    }
}
