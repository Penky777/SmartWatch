#include "lvgl.h"
#include "../Ui_manager/ui_manager.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <stdlib.h>
#include "../../lib/PCF85063/bsp_pcf85063.h"
#include "esp_log.h"

static const char *TAG = "CALENDAR";

// ==================== SWIPE DETECTION ====================

typedef struct {
    lv_point_t press_point;
    lv_point_t release_point;
    bool pressed;
} swipe_state_t;

static swipe_state_t calendar_swipe = {0};

// ==================== FORWARD DECLARATIONS ====================

static void add_days_to_date(struct tm *date, int days);
static void format_date_string(char *buffer, size_t size, struct tm *date);
static void format_month_year(char *buffer, size_t size, struct tm *date);
static lv_obj_t* create_date_row(lv_obj_t *parent, const char *date, 
                                  const char *event_text, lv_event_cb_t cb);
static void date_row_event_cb(lv_event_t *e);
static void calendar_event_cb(lv_event_t *e);
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

// ==================== HELPER FUNCTIONS ====================

static void add_days_to_date(struct tm *date, int days) {
    date->tm_mday += days;
    mktime(date);
}

static void format_date_string(char *buffer, size_t size, struct tm *date) {
    strftime(buffer, size, "%d.%m", date);
}

static void format_month_year(char *buffer, size_t size, struct tm *date) {
    strftime(buffer, size, "%B %Y", date);
}

// ==================== UI COMPONENTS ====================

static lv_obj_t* create_date_row(lv_obj_t *parent, const char *date, 
                                  const char *event_text, lv_event_cb_t cb) {
    lv_obj_t *row = lv_btn_create(parent);
    lv_obj_set_size(row, lv_pct(90), 60);
    lv_obj_set_style_bg_color(row, lv_color_hex(0x3a3a3a), 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(row, 15, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_left(row, 20, 0);
    lv_obj_set_style_pad_right(row, 20, 0);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, 
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *date_lbl = lv_label_create(row);
    lv_label_set_text(date_lbl, date);
    lv_obj_set_style_text_color(date_lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(date_lbl, &lv_font_montserrat_20, 0);

    lv_obj_t *event_lbl = lv_label_create(row);
    lv_label_set_text(event_lbl, event_text);
    lv_obj_set_style_text_color(event_lbl, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(event_lbl, &lv_font_montserrat_18, 0);

    if (cb) {
        lv_obj_add_event_cb(row, cb, LV_EVENT_CLICKED, (void*)date);
    }

    return row;
}

// ==================== BUILD CALENDAR SCREEN ====================

lv_obj_t *build_calendar_screen(void) {
    ESP_LOGI(TAG, "Building calendar screen...");
    
    lv_obj_t *scr = lv_obj_create(NULL);
    
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    
    // Enable scrolling
    lv_obj_set_scroll_dir(scr, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(scr, LV_SCROLLBAR_MODE_OFF);
    
    // Register swipe events
    lv_obj_add_event_cb(scr, calendar_event_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(scr, calendar_event_cb, LV_EVENT_RELEASED, NULL);

    // Layout
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, 
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(scr, 12, 0);
    lv_obj_set_style_pad_top(scr, 20, 0);
    lv_obj_set_style_pad_bottom(scr, 20, 0);

    // Get current date
    struct tm current_time;
    if (!bsp_pcf85063_get_time(&current_time)) {
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        if (tm_info) {
            current_time = *tm_info;
        } else {
            memset(&current_time, 0, sizeof(struct tm));
            current_time.tm_year = 125;
            current_time.tm_mon = 0;
            current_time.tm_mday = 1;
        }
    }

    // Month header
    lv_obj_t *month_lbl = lv_label_create(scr);
    char month_str[32];
    format_month_year(month_str, sizeof(month_str), &current_time);
    lv_label_set_text(month_lbl, month_str);
    lv_obj_set_style_text_color(month_lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(month_lbl, &lv_font_montserrat_18, 0);
    lv_obj_set_style_pad_bottom(month_lbl, 10, 0);

    // Date rows (±7 days from today)
    static char date_strings[15][16];
    for (int offset = -7; offset <= 7; offset++) {
        struct tm date = current_time;
        add_days_to_date(&date, offset);
        
        int idx = offset + 7;
        format_date_string(date_strings[idx], 16, &date);
        
        const char *event_text = (offset == 0) ? "TODAY" : "no events";
        
        // Create clickable row
        lv_obj_t *row = create_date_row(scr, date_strings[idx], 
                                        event_text, date_row_event_cb);
        
        // Highlight today
        if (offset == 0) {
            lv_obj_set_style_bg_color(row, lv_color_hex(0x2196F3), 0);
            lv_obj_t *event_lbl = lv_obj_get_child(row, 1);
            if (event_lbl) {
                lv_obj_set_style_text_color(event_lbl, lv_color_white(), 0);
            }
        }
    }

    lv_obj_update_layout(scr);
    lv_obj_scroll_to_y(scr, 7 * 72, LV_ANIM_OFF);
    
    // Reset swipe state
    memset(&calendar_swipe, 0, sizeof(swipe_state_t));
    
    ESP_LOGI(TAG, "Calendar screen built");
    return scr;
}

// ==================== EVENT HANDLERS ====================

static void calendar_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    
    if (code == LV_EVENT_PRESSED) {
        lv_indev_t *indev = lv_indev_get_act();
        if (indev) {
            lv_indev_get_point(indev, &calendar_swipe.press_point);
            calendar_swipe.pressed = true;
        }
    }
    else if (code == LV_EVENT_RELEASED) {
        lv_indev_t *indev = lv_indev_get_act();
        if (indev && calendar_swipe.pressed) {
            lv_indev_get_point(indev, &calendar_swipe.release_point);
            calendar_swipe.pressed = false;
            
            if (detect_swipe_right(&calendar_swipe)) {
                ESP_LOGI(TAG, "Swipe → Going back");
                
                if (ui_can_go_back()) {
                    ui_go_back();
                } else {
                    ui_show_menu();
                }
            }
        }
    }
}

static void date_row_event_cb(lv_event_t *e) {
    const char *date = (const char*)lv_event_get_user_data(e);
    ESP_LOGI(TAG, "Date clicked: %s", date);
    ui_show_detail(date);
}
