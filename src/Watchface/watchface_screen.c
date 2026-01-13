#include "watchface_screen.h"
#include "../Ui_manager/ui_manager.h"
#include "../../lib/PCF85063/bsp_pcf85063.h"
#include "../Bsp_bat/bsp_battery.h"
#include "../icons/watchface_bg.h"
#include <stdio.h>
#include <time.h>
#include "esp_log.h"

static const char *TAG = "WATCHFACE";

// UI elements
static lv_obj_t *time_label = NULL;
static lv_obj_t *overlay_panel = NULL;
static lv_obj_t *date_label = NULL;  

// Swipe tracking
static int16_t drag_start_y = 0;
static bool is_dragging = false;

// Forward declarations
static void screen_tap_event_cb(lv_event_t *e);

// ==================== BUILD WATCHFACE ====================

lv_obj_t *build_watchface_screen(void)
{
    ESP_LOGI(TAG, "Building watchface screen...");
    
    lv_obj_t *scr = lv_obj_create(NULL);
    
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(scr, 0, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    
    // Background image
    lv_obj_t *bg_img = lv_img_create(scr);
    lv_img_set_src(bg_img, &image_kasv);
    lv_obj_align(bg_img, LV_ALIGN_CENTER, 0, 20);
    lv_obj_clear_flag(bg_img, LV_OBJ_FLAG_CLICKABLE);

    // ========== DATE LABEL ==========
    
    // Digital clock
    time_label = lv_label_create(scr);
    lv_label_set_text(time_label, "12:34");
    lv_obj_set_style_text_color(time_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_48, 0);
    lv_obj_set_style_shadow_width(time_label, 10, 0);
    lv_obj_set_style_shadow_color(time_label, lv_color_black(), 0);
    lv_obj_set_style_shadow_opa(time_label, LV_OPA_80, 0);
    lv_obj_align(time_label, LV_ALIGN_OUT_TOP_MID, 60, 10);

    date_label = lv_label_create(scr);
    lv_label_set_text(date_label, "Mon, Jan 13");  // Initial placeholder
    lv_obj_set_style_text_color(date_label, lv_color_hex(0xAAAAAA), 0);  // Light gray
    lv_obj_set_style_text_font(date_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_shadow_width(date_label, 8, 0);
    lv_obj_set_style_shadow_color(date_label, lv_color_black(), 0);
    lv_obj_set_style_shadow_opa(date_label, LV_OPA_60, 0);
    lv_obj_align(date_label, LV_ALIGN_OUT_TOP_MID,60,60);
    

    // Tap to menu
    lv_obj_add_event_cb(scr, screen_tap_event_cb, LV_EVENT_CLICKED, NULL);
    
    ESP_LOGI(TAG, "Watchface screen built");
    return scr;
}




void watchface_update_time(const char *time_str)
{
    if (time_label && time_str) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%.5s", time_str);
        lv_label_set_text(time_label, buf);
    }
}

static void screen_tap_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        if (overlay_panel) return;
        ui_show_menu();
    }
}
void watchface_update_date(const char *date_str)
{
    if (date_label && date_str) {
        lv_label_set_text(date_label, date_str);
    }
}
