#include "lvgl.h"
#include "./Ui_manager/ui_manager.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "DETAIL";

static char current_date[32] = {0};

// Forward declarations
static void swipe_to_calendar_event_cb(lv_event_t *e);

// ==================== SWIPE DETECTION ===================

static void swipe_to_calendar_event_cb(lv_event_t *e) {
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
    if (dir == LV_DIR_RIGHT) {
        ui_show_calendar();
    }
}

// ==================== BUILD SCREEN ====================

lv_obj_t *build_detail_screen(const char *date) {
    ESP_LOGI(TAG, "Building detail screen for: %s", date);
    
    // Save date
    snprintf(current_date, sizeof(current_date), "%s", date);
    
    // Create screen
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
        
    // ========== TITLE ==========
    lv_obj_t *title = lv_label_create(scr);
    char title_text[64];
    snprintf(title_text, sizeof(title_text), "Events: %s", date);
    lv_label_set_text(title, title_text);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    
    // ========== SWIPE HINT ==========
    lv_obj_t *hint = lv_label_create(scr);
    lv_label_set_text(hint, LV_SYMBOL_RIGHT " Swipe right to go back");
    lv_obj_set_style_text_color(hint, lv_color_hex(0x666666), 0);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_14, 0);
    lv_obj_align(hint, LV_ALIGN_TOP_MID, 0, 50);
    
    // ========== EVENT CONTAINER ==========
    lv_obj_t *event_container = lv_obj_create(scr);
    lv_obj_set_size(event_container, lv_pct(90), 180);
    lv_obj_align(event_container, LV_ALIGN_CENTER, 0, -10);
    lv_obj_set_style_bg_color(event_container, lv_color_hex(0x1a1a1a), 0);
    lv_obj_set_style_radius(event_container, 15, 0);
    lv_obj_set_style_border_width(event_container, 0, 0);
    
    // Empty state message
    lv_obj_t *msg = lv_label_create(event_container);
    lv_label_set_text(msg, "No events scheduled\nfor this date");
    lv_obj_set_style_text_color(msg, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(msg, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_align(msg, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(msg);

    lv_obj_add_event(scr, swipe_to_calendar_event_cb,LV_EVENT_GESTURE,NULL);

    ESP_LOGI(TAG, "Detail screen built");
    return scr;
}

// ==================== EVENT HANDLERS ====================


