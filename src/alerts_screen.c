#include "lvgl.h"
#include "Ui_manager/ui_manager.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "ALERTS";
// Forward declarations
//static void alerts_event_cb(lv_event_t *e);
static lv_obj_t* create_notification_card(lv_obj_t *parent, const char *title, const char *message);
void alerts_event_cb_to_menu(lv_event_t *e);

// ==================== NOTIFICATION CARD ====================

static lv_obj_t* create_notification_card(lv_obj_t *parent, const char *title, const char *message) {
    // Card container
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, lv_pct(90), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(card, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(card, 25, 0);
    lv_obj_set_style_border_width(card, 0, 0);
    lv_obj_set_style_pad_all(card, 20, 0);
    lv_obj_set_style_pad_row(card, 8, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    
    // Layout
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    
    // Title (if provided)
    if (title && strlen(title) > 0) {
        lv_obj_t *title_lbl = lv_label_create(card);
        lv_label_set_text(title_lbl, title);
        lv_obj_set_style_text_color(title_lbl, lv_color_black(), 0);
        lv_obj_set_style_text_font(title_lbl, &lv_font_montserrat_18, 0);
    }
    
    // Message
    if (message && strlen(message) > 0) {
        lv_obj_t *msg_lbl = lv_label_create(card);
        lv_label_set_text(msg_lbl, message);
        lv_label_set_long_mode(msg_lbl, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(msg_lbl, lv_pct(100));
        lv_obj_set_style_text_color(msg_lbl, lv_color_hex(0x555555), 0);
        lv_obj_set_style_text_font(msg_lbl, &lv_font_montserrat_14, 0);
    }
    
    return card;
}

// ==================== BUILD ALERTS SCREEN ====================

lv_obj_t *build_alerts_screen(void) {
    ESP_LOGI(TAG, "Building alerts screen...");
    
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x2a2a2a), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    
    // Enable scrolling
    lv_obj_set_scroll_dir(scr, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(scr, LV_SCROLLBAR_MODE_OFF);
    
    // ========== TITLE ==========
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "Alerts");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);  
    
    // ========== SCROLLABLE CONTAINER FOR CARDS ==========
    lv_obj_t *card_container = lv_obj_create(scr);
    lv_obj_set_size(card_container, lv_pct(100), lv_pct(85));  
    lv_obj_align(card_container, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_opa(card_container, LV_OPA_TRANSP, 0);  
    lv_obj_set_style_border_width(card_container, 0, 0);
    lv_obj_set_scroll_dir(card_container, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(card_container, LV_SCROLLBAR_MODE_OFF);
    
    lv_obj_set_flex_flow(card_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(card_container, 15, 0);
    lv_obj_set_style_pad_top(card_container, 10, 0);
    lv_obj_set_style_pad_bottom(card_container, 20, 0);
    
    // ========== NOTIFICATION CARDS ==========
    create_notification_card(card_container, "", "");  
    create_notification_card(card_container, "", "");
    create_notification_card(card_container, "", "");
        
    lv_obj_add_event_cb(scr, alerts_event_cb_to_menu, LV_EVENT_GESTURE, NULL);
    
    ESP_LOGI(TAG, "Alerts screen built");
    return scr;
}


// // ==================== EVENT HANDLER ====================

// static void alerts_event_cb(lv_event_t *e) {
//     lv_event_code_t code = lv_event_get_code(e);
    
//     if (code == LV_EVENT_PRESSED) {
//         lv_indev_t *indev = lv_indev_get_act();
//         if (indev) {
//             lv_indev_get_point(indev, &alerts_swipe.press_point);
//             alerts_swipe.pressed = true;
//         }
//     }
//     else if (code == LV_EVENT_RELEASED) {
//         lv_indev_t *indev = lv_indev_get_act();
//         if (indev && alerts_swipe.pressed) {
//             lv_indev_get_point(indev, &alerts_swipe.release_point);
//             alerts_swipe.pressed = false;
            
//             if (detect_swipe_left(&alerts_swipe)) {
//                 ESP_LOGI(TAG, "Swipe left → Going back to watchface");
                
//                 if (ui_can_go_back()) {
//                     ui_go_back();
//                 } else {
//                     ui_show_watchface();
//                 }
//             }
//         }
//     }
// }

// ==================== PUBLIC API ====================

void alerts_add_notification(const char *title, const char *message) {
    ESP_LOGI(TAG, "New notification: %s - %s", title, message);
}

void alerts_clear_all(void) {
    ESP_LOGI(TAG, "Clearing all notifications");
}
void alerts_event_cb_to_menu(lv_event_t *e){
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
    if (dir == LV_DIR_RIGHT) {
        ui_show_watchface();

    }
}
