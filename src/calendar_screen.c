// calendar_screen.c - Calendar with swipe navigation
// Complete rewrite with working gesture detection

#include "lvgl.h"
#include "ui_manager.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <stdlib.h>
#include "bsp_pcf85063.h"
#include "esp_log.h"

static const char *TAG = "CALENDAR";

// ==================== SWIPE DETECTION STATE ====================

typedef struct {
    lv_point_t press_point;
    lv_point_t release_point;
    bool pressed;
    uint32_t press_time;
} swipe_state_t;

static swipe_state_t calendar_swipe = {0};
static swipe_state_t detail_swipe = {0};

// ==================== DETAIL SCREEN STATE ====================

static lv_obj_t *detail_screen = NULL;
static char selected_date[32] = {0};

// ==================== FORWARD DECLARATIONS ====================

static void add_days_to_date(struct tm *date, int days);
static void format_date_string(char *buffer, size_t size, struct tm *date);
static void format_month_year(char *buffer, size_t size, struct tm *date);
static lv_obj_t* create_date_row(lv_obj_t *parent, const char *date, 
                                  const char *event_text, lv_event_cb_t cb);
static void date_row_event_cb(lv_event_t *e);
static void calendar_event_cb(lv_event_t *e);
static void detail_event_cb(lv_event_t *e);
static void detail_back_btn_cb(lv_event_t *e);
static void build_event_detail_screen(lv_obj_t *scr, const char *date);
static bool detect_swipe_right(swipe_state_t *state);

// ==================== SWIPE DETECTION ====================

static bool detect_swipe_right(swipe_state_t *state) {
    int dx = state->release_point.x - state->press_point.x;
    int dy = state->release_point.y - state->press_point.y;
    
    // Must be: rightward (dx > 0), at least 60px, more horizontal than vertical
    bool is_right_swipe = (dx > 60) && (abs(dx) > abs(dy) * 1.5);
    
    if (is_right_swipe) {
        ESP_LOGI(TAG, "✓ Swipe detected: dx=%d dy=%d", dx, dy);
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

// ==================== CALENDAR SCREEN ====================

lv_obj_t *build_calendar_screen(void) {
    lv_obj_t *scr = lv_obj_create(NULL);
    
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    
    // Enable scrolling and gestures
    lv_obj_set_scroll_dir(scr, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(scr, LV_SCROLLBAR_MODE_OFF);
    
    // Register unified event handler
    lv_obj_add_event_cb(scr, calendar_event_cb, LV_EVENT_ALL, NULL);

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

    // Date rows
    static char date_strings[15][16];
    for (int offset = -7; offset <= 7; offset++) {
        struct tm date = current_time;
        add_days_to_date(&date, offset);
        
        int idx = offset + 7;
        format_date_string(date_strings[idx], 16, &date);
        
        const char *event_text = (offset == 0) ? "TODAY" : "no events";
        lv_obj_t *row = create_date_row(scr, date_strings[idx], 
                                        event_text, date_row_event_cb);
        
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

// Calendar event handler - handles all events in one place
static void calendar_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);
    
    if (code == LV_EVENT_PRESSED) {
        lv_indev_t *indev = lv_indev_get_act();
        if (indev) {
            lv_indev_get_point(indev, &calendar_swipe.press_point);
            calendar_swipe.pressed = true;
            calendar_swipe.press_time = lv_tick_get();
        }
    }
    else if (code == LV_EVENT_RELEASED) {
        lv_indev_t *indev = lv_indev_get_act();
        if (indev && calendar_swipe.pressed) {
            lv_indev_get_point(indev, &calendar_swipe.release_point);
            calendar_swipe.pressed = false;
            
            if (detect_swipe_right(&calendar_swipe)) {
                ESP_LOGI(TAG, "Swipe → Menu");
                
                if (detail_screen) {
                    lv_obj_del(detail_screen);
                    detail_screen = NULL;
                }
                
                ui_show_menu();
            }
        }
    }
}

// Date row clicked - show detail
static void date_row_event_cb(lv_event_t *e) {
    const char *date = (const char*)lv_event_get_user_data(e);
    
    snprintf(selected_date, sizeof(selected_date), "%s", date);
    ESP_LOGI(TAG, "Date selected: %s", selected_date);
    
    if (detail_screen) {
        lv_obj_del(detail_screen);
    }
    
    detail_screen = lv_obj_create(NULL);
    build_event_detail_screen(detail_screen, selected_date);
    
    lv_scr_load_anim(detail_screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, false);
}

// ==================== DETAIL SCREEN ====================

static void build_event_detail_screen(lv_obj_t *scr, const char *date) {
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    
    // Register unified event handler
    lv_obj_add_event_cb(scr, detail_event_cb, LV_EVENT_ALL, NULL);

    // Title
    lv_obj_t *title = lv_label_create(scr);
    char title_text[64];
    snprintf(title_text, sizeof(title_text), "Events: %s", date);
    lv_label_set_text(title, title_text);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 25);

    // Message
    lv_obj_t *msg = lv_label_create(scr);
    lv_label_set_text(msg, "No events scheduled");
    lv_obj_set_style_text_color(msg, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(msg, &lv_font_montserrat_18, 0);
    lv_obj_align(msg, LV_ALIGN_CENTER, 0, -10);

    // Back button
    lv_obj_t *back_btn = lv_btn_create(scr);
    lv_obj_set_size(back_btn, 140, 50);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_MID, 0, -40);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0x2196F3), 0);
    lv_obj_set_style_radius(back_btn, 25, 0);
    
    // Stop button events from bubbling to parent
    lv_obj_add_flag(back_btn, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_event_cb(back_btn, detail_back_btn_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *btn_lbl = lv_label_create(back_btn);
    lv_label_set_text(btn_lbl, LV_SYMBOL_LEFT " Back");
    lv_obj_set_style_text_color(btn_lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(btn_lbl, &lv_font_montserrat_18, 0);
    lv_obj_center(btn_lbl);
    
    // Make label non-clickable so button gets the event
    lv_obj_add_flag(btn_lbl, LV_OBJ_FLAG_EVENT_BUBBLE);

    // Hint
    lv_obj_t *hint = lv_label_create(scr);
    lv_label_set_text(hint, "Swipe right or tap Back →");
    lv_obj_set_style_text_color(hint, lv_color_hex(0x666666), 0);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_14, 0);
    lv_obj_align(hint, LV_ALIGN_TOP_MID, 0, 60);
    
    // Reset swipe state
    memset(&detail_swipe, 0, sizeof(swipe_state_t));
    
    ESP_LOGI(TAG, "Detail screen built");
}

// Detail screen event handler
static void detail_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);
    
    if (code == LV_EVENT_PRESSED) {
        lv_indev_t *indev = lv_indev_get_act();
        if (indev) {
            lv_indev_get_point(indev, &detail_swipe.press_point);
            detail_swipe.pressed = true;
            detail_swipe.press_time = lv_tick_get();
            ESP_LOGD(TAG, "Touch pressed at X=%d Y=%d", 
                     detail_swipe.press_point.x, detail_swipe.press_point.y);
        }
    }
    else if (code == LV_EVENT_PRESSING) {
        // Optional: visual feedback during drag
    }
    else if (code == LV_EVENT_RELEASED) {
        lv_indev_t *indev = lv_indev_get_act();
        if (indev && detail_swipe.pressed) {
            lv_indev_get_point(indev, &detail_swipe.release_point);
            detail_swipe.pressed = false;
            
            ESP_LOGD(TAG, "Touch released at X=%d Y=%d", 
                     detail_swipe.release_point.x, detail_swipe.release_point.y);
            
            if (detect_swipe_right(&detail_swipe)) {
                ESP_LOGI(TAG, "Swipe → Calendar");
                
                if (detail_screen) {
                    lv_obj_del(detail_screen);
                    detail_screen = NULL;
                }
                
                ui_show_calendar();
            }
        }
    }
}

// Back button clicked
static void detail_back_btn_cb(lv_event_t *e) {
    ESP_LOGI(TAG, "Back button → Calendar");
    
    if (detail_screen) {
        lv_obj_del(detail_screen);
        detail_screen = NULL;
    }
    
    ui_show_calendar();
}
