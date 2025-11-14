#include "lvgl.h"
#include "ui_manager.h"
#include "menu_screen.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include "bsp_pcf85063.h"

// ---------- Forward declarations ----------
static void build_event_detail_screen(lv_obj_t *scr, const char *date);
static void calendar_swipe_event_cb(lv_event_t *e);
static void detail_swipe_event_cb(lv_event_t *e);
static void date_row_event_cb(lv_event_t *e);
static void add_days_to_date(struct tm *date, int days);
static void format_date_string(char *buffer, size_t size, struct tm *date);

lv_obj_t *build_calendar_screen(void);

// ---------- Helper: Add days to a date ----------
static void add_days_to_date(struct tm *date, int days) {
    date->tm_mday += days;
    mktime(date); // Normalize the date
}

// ---------- Helper: Format date as "DD.MM" ----------
static void format_date_string(char *buffer, size_t size, struct tm *date) {
    strftime(buffer, size, "%d.%m", date);
}

// ---------- Helper: Get month name ----------
static void format_month_year(char *buffer, size_t size, struct tm *date) {
    strftime(buffer, size, "%B, %Y", date);
}

// ---------- Helper: Create a date row ----------
static lv_obj_t* create_date_row(lv_obj_t *parent, const char *date, const char *event_text, lv_event_cb_t cb) {
    lv_obj_t *row = lv_btn_create(parent);
    lv_obj_set_size(row, lv_pct(90), 60);
    lv_obj_set_style_bg_color(row, lv_color_hex(0x4a4a4a), 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(row, 30, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_left(row, 25, 0);
    lv_obj_set_style_pad_right(row, 25, 0);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *date_lbl = lv_label_create(row);
    lv_label_set_text(date_lbl, date);
    lv_obj_set_style_text_color(date_lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(date_lbl, &lv_font_montserrat_20, 0);

    lv_obj_t *event_lbl = lv_label_create(row);
    lv_label_set_text(event_lbl, event_text);
    lv_obj_set_style_text_color(event_lbl, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(event_lbl, &lv_font_montserrat_18, 0);

    if (cb != NULL) {
        lv_obj_add_event_cb(row, cb, LV_EVENT_CLICKED, (void*)date);
    }

    return row;
}

// ---------- Date row callback ----------
static void date_row_event_cb(lv_event_t *e) {
    const char *date = (const char*)lv_event_get_user_data(e);
    
    // Get current screen before creating new one
    lv_obj_t *old_scr = lv_scr_act();
    
    // Create and load new screen
    lv_obj_t *detail_scr = lv_obj_create(NULL);
    build_event_detail_screen(detail_scr, date);
    lv_scr_load(detail_scr);
    
    // Delete old screen to free memory
    lv_obj_del(old_scr);
}

// ---------- Calendar swipe: RIGHT = back to menu ----------
static void calendar_swipe_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_GESTURE) {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
        if (dir == LV_DIR_RIGHT) {
            ui_show_menu();
        }
    }
}

// ---------- Detail swipe: RIGHT = back to calendar ----------
static void detail_swipe_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_GESTURE) {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
        if (dir == LV_DIR_RIGHT) {
            ui_show_calendar();
        }
    }
}

// ---------- Event detail screen ----------
static void build_event_detail_screen(lv_obj_t *scr, const char *date) {
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    
    lv_obj_add_event_cb(scr, detail_swipe_event_cb, LV_EVENT_GESTURE, NULL);

    lv_obj_t *title = lv_label_create(scr);
    char title_text[32];
    snprintf(title_text, sizeof(title_text), "Events for %s", date);
    lv_label_set_text(title, title_text);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

    lv_obj_t *msg = lv_label_create(scr);
    lv_label_set_text(msg, "No events scheduled");
    lv_obj_set_style_text_color(msg, lv_color_hex(0x888888), 0);
    lv_obj_align(msg, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *hint = lv_label_create(scr);
    lv_label_set_text(hint, "← Swipe to go back");
    lv_obj_set_style_text_color(hint, lv_color_hex(0x666666), 0);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_14, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -20);
}

// ---------- Calendar screen ----------
lv_obj_t *build_calendar_screen(void) {
    lv_obj_t *scr = lv_obj_create(NULL);
    
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    
    lv_obj_add_event_cb(scr, calendar_swipe_event_cb, LV_EVENT_GESTURE, NULL);

    lv_obj_set_scroll_dir(scr, LV_DIR_VER);
    lv_obj_add_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(scr, LV_SCROLLBAR_MODE_OFF);

    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(scr, 15, 0);
    lv_obj_set_style_pad_top(scr, 20, 0);

    // Get current date from RTC or system time
    struct tm current_time;
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    if (tm_info != NULL) {
        current_time = *tm_info;
    } else {
        // Fallback to a default date if localtime fails
        memset(&current_time, 0, sizeof(struct tm));
        current_time.tm_year = 125; // 2025
        current_time.tm_mon = 0;    // January (0-based)
        current_time.tm_mday = 1;
    }

    // Display current month and year
    lv_obj_t *month_lbl = lv_label_create(scr);
    char month_str[32];
    format_month_year(month_str, sizeof(month_str), &current_time);
    lv_label_set_text(month_lbl, month_str);
    lv_obj_set_style_text_color(month_lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(month_lbl, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_align(month_lbl, LV_TEXT_ALIGN_CENTER, 0);

    // Static storage for date strings (15 dates: -7 to +7)
    static char date_strings[15][16];
    int str_index = 0;

    // Create date rows for -7 to +7 days
    for (int offset = -7; offset <= 7; offset++) {
        struct tm date = current_time;
        add_days_to_date(&date, offset);
        
        format_date_string(date_strings[str_index], 16, &date);
        
        // Highlight today
        const char *event_text = (offset == 0) ? "TODAY" : "no events";
        
        lv_obj_t *row = create_date_row(scr, date_strings[str_index], event_text, date_row_event_cb);
        
        // Style today's row differently
        if (offset == 0) {
            lv_obj_set_style_bg_color(row, lv_color_hex(0x2196F3), 0);
            lv_obj_t *event_lbl = lv_obj_get_child(row, 1);
            if (event_lbl) {
                lv_obj_set_style_text_color(event_lbl, lv_color_white(), 0);
            }
        }
        
        str_index++;
    }

    // Scroll to today (middle of the list)
    lv_obj_update_layout(scr);
    lv_obj_scroll_to_y(scr, 7 * 75, LV_ANIM_OFF); // Position of today
    
    return scr;
}

