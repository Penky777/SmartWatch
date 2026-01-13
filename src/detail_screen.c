#include "lvgl.h"
#include "ui_manager.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "DETAIL";

// Swipe detection
typedef struct {
    lv_point_t press_point;
    lv_point_t release_point;
    bool pressed;
} swipe_state_t;

static swipe_state_t detail_swipe = {0};
static char current_date[32] = {0};

// Forward declarations
static void detail_event_cb(lv_event_t *e);
static void back_btn_cb(lv_event_t *e);
static bool detect_swipe_right(swipe_state_t *state);

// ==================== SWIPE DETECTION ====================

static bool detect_swipe_right(swipe_state_t *state) {
    int dx = state->release_point.x - state->press_point.x;
    int dy = state->release_point.y - state->press_point.y;
    
    bool is_right_swipe = (dx > 60) && (abs(dx) > abs(dy) * 1.5);
    
    if (is_right_swipe) {
        ESP_LOGI(TAG, "✓ Swipe right: dx=%d dy=%d", dx, dy);
    }
    
    return is_right_swipe;
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
    
    // Register swipe events
    lv_obj_add_event_cb(scr, detail_event_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(scr, detail_event_cb, LV_EVENT_RELEASED, NULL);
    
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
    
    // ========== BACK BUTTON ==========
    lv_obj_t *back_btn = lv_btn_create(scr);
    lv_obj_set_size(back_btn, 160, 50);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_MID, 0, -25);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0x2196F3), 0);
    lv_obj_set_style_radius(back_btn, 25, 0);
    lv_obj_set_style_border_width(back_btn, 0, 0);
    lv_obj_add_event_cb(back_btn, back_btn_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *btn_lbl = lv_label_create(back_btn);
    lv_label_set_text(btn_lbl, LV_SYMBOL_LEFT " Back");
    lv_obj_set_style_text_color(btn_lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(btn_lbl, &lv_font_montserrat_18, 0);
    lv_obj_center(btn_lbl);
    
    // Reset swipe state
    memset(&detail_swipe, 0, sizeof(swipe_state_t));
    
    ESP_LOGI(TAG, "Detail screen built");
    return scr;
}

// ==================== EVENT HANDLERS ====================

static void detail_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    
    if (code == LV_EVENT_PRESSED) {
        lv_indev_t *indev = lv_indev_get_act();
        if (indev) {
            lv_indev_get_point(indev, &detail_swipe.press_point);
            detail_swipe.pressed = true;
        }
    }
    else if (code == LV_EVENT_RELEASED) {
        lv_indev_t *indev = lv_indev_get_act();
        if (indev && detail_swipe.pressed) {
            lv_indev_get_point(indev, &detail_swipe.release_point);
            detail_swipe.pressed = false;
            
            if (detect_swipe_right(&detail_swipe)) {
                ESP_LOGI(TAG, "Swipe → Going back");
                
                if (ui_can_go_back()) {
                    ui_go_back();
                } else {
                    ui_show_calendar();
                }
            }
        }
    }
}

static void back_btn_cb(lv_event_t *e) {
    ESP_LOGI(TAG, "Back button clicked");
    
    if (ui_can_go_back()) {
        ui_go_back();
    } else {
        ui_show_calendar();
    }
}
