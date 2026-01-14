#include "lvgl.h"
#include "Ui_manager/ui_manager.h"
#include "esp_log.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "ALERTS";

// Notification storage
#define MAX_NOTIFICATIONS 10
#define MAX_TITLE_LEN 64
#define MAX_TEXT_LEN 128
#define MAX_APP_LEN 32

typedef struct {
    char title[MAX_TITLE_LEN];
    char text[MAX_TEXT_LEN];
    char app[MAX_APP_LEN];
    bool active;
} notification_t;

static notification_t notifications[MAX_NOTIFICATIONS] = {0};
static int notification_count = 0;
static lv_obj_t *card_container = NULL;
static volatile bool notification_refresh_pending = false;
static volatile bool is_refreshing = false;  // Prevent concurrent refresh

// Forward declarations
static void modal_close_cb(lv_event_t *e);
static void card_text_event_cb(lv_event_t *e);
static lv_obj_t* create_notification_card(lv_obj_t *parent, const char *title, const char *message, const char *app);
void alerts_event_cb_to_menu(lv_event_t *e);
static void refresh_alerts_screen(void);
static void alerts_screen_show_cb(lv_event_t *e);
void alerts_refresh_if_active(void);
bool alerts_has_pending_refresh(void);

// ==================== NOTIFICATION CARD ====================

typedef struct {
    const char *full_text;
} card_data_t;

static void modal_close_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        lv_obj_del(lv_event_get_target(e));
    }
}

static void card_text_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        lv_obj_t *card = lv_event_get_target(e);
        card_data_t *data = (card_data_t *)lv_obj_get_user_data(card);
        
        if (!data || !data->full_text) return;
        
        // Create modal popup with full text
        lv_obj_t *modal = lv_obj_create(lv_scr_act());
        lv_obj_set_size(modal, lv_pct(95), lv_pct(80));
        lv_obj_center(modal);
        lv_obj_set_style_bg_color(modal, lv_color_black(), 0);
        lv_obj_set_style_bg_opa(modal, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(modal, lv_color_white(), 0);
        lv_obj_set_style_border_width(modal, 2, 0);
        lv_obj_set_style_radius(modal, 15, 0);
        lv_obj_set_style_pad_all(modal, 15, 0);
        
        // Add scrollable text
        lv_obj_t *text_label = lv_label_create(modal);
        lv_label_set_text(text_label, data->full_text);
        lv_label_set_long_mode(text_label, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(text_label, lv_pct(100));
        lv_obj_set_style_text_color(text_label, lv_color_white(), 0);
        lv_obj_set_style_text_font(text_label, &lv_font_montserrat_14, 0);
        
        // Close on click
        lv_obj_add_event_cb(modal, modal_close_cb, LV_EVENT_CLICKED, NULL);
    }
}

static lv_obj_t* create_notification_card(lv_obj_t *parent, const char *title, const char *message, const char *app) {
    // Card container (clickable)
    lv_obj_t *card = lv_btn_create(parent);
    lv_obj_set_size(card, lv_pct(90), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(card, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(card, 25, 0);
    lv_obj_set_style_border_width(card, 0, 0);
    lv_obj_set_style_pad_all(card, 15, 0);
    lv_obj_set_style_pad_row(card, 6, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    
    // Store full text for modal expand
    card_data_t *card_data = malloc(sizeof(card_data_t));
    if (card_data) {
        card_data->full_text = message;
        lv_obj_set_user_data(card, card_data);
    }
    
    // Layout
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    
    // App label (top, small)
    if (app && strlen(app) > 0) {
        lv_obj_t *app_lbl = lv_label_create(card);
        lv_label_set_text(app_lbl, app);
        lv_obj_set_style_text_color(app_lbl, lv_color_hex(0x888888), 0);
        lv_obj_set_style_text_font(app_lbl, &lv_font_montserrat_14, 0);
    }
    
    // Title
    if (title && strlen(title) > 0) {
        lv_obj_t *title_lbl = lv_label_create(card);
        lv_label_set_text(title_lbl, title);
        lv_obj_set_style_text_color(title_lbl, lv_color_black(), 0);
        lv_obj_set_style_text_font(title_lbl, &lv_font_montserrat_18, 0);
        lv_label_set_long_mode(title_lbl, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(title_lbl, lv_pct(100));
    }
    
    // Message (truncated with ellipsis)
    if (message && strlen(message) > 0) {
        lv_obj_t *msg_lbl = lv_label_create(card);
        lv_label_set_long_mode(msg_lbl, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_label_set_text(msg_lbl, message);
        lv_obj_set_width(msg_lbl, lv_pct(100));
        lv_obj_set_height(msg_lbl, 50);  // Limit to ~2 lines
        lv_obj_set_style_text_color(msg_lbl, lv_color_hex(0x333333), 0);
        lv_obj_set_style_text_font(msg_lbl, &lv_font_montserrat_14, 0);
    }
    
    // Add click handler to show full text in modal
    lv_obj_add_event_cb(card, card_text_event_cb, LV_EVENT_CLICKED, NULL);
    
    return card;
}

// ==================== BUILD ALERTS SCREEN ====================

lv_obj_t *build_alerts_screen(void) {
    ESP_LOGI(TAG, "Building alerts screen...");
    
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
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
    card_container = lv_obj_create(scr);
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
    
    // Populate with stored notifications
    refresh_alerts_screen();
        
    lv_obj_add_event_cb(scr, alerts_event_cb_to_menu, LV_EVENT_GESTURE, NULL);
    lv_obj_add_event_cb(scr, alerts_screen_show_cb, LV_EVENT_SCREEN_LOADED, NULL);
    
    ESP_LOGI(TAG, "Alerts screen built with %d notifications", notification_count);
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

// ==================== REFRESH SCREEN ====================

static void refresh_alerts_screen(void) {
    // Guard against concurrent refresh or invalid container
    if (!card_container || is_refreshing) {
        ESP_LOGW(TAG, "Refresh skipped: container=%p, is_refreshing=%d", card_container, is_refreshing);
        return;
    }
    
    is_refreshing = true;
    
    // Validate container is still valid LVGL object
    if (!lv_obj_is_valid(card_container)) {
        ESP_LOGE(TAG, "Card container is invalid!");
        card_container = NULL;
        is_refreshing = false;
        return;
    }
    
    // Clear existing cards
    lv_obj_clean(card_container);
    
    // Add cards from stored notifications (most recent first)
    if (notification_count == 0) {
        // Show "No notifications" message
        lv_obj_t *empty_label = lv_label_create(card_container);
        lv_label_set_text(empty_label, "No notifications");
        lv_obj_set_style_text_color(empty_label, lv_color_hex(0x888888), 0);
        lv_obj_set_style_text_font(empty_label, &lv_font_montserrat_14, 0);
    } else {
        for (int i = notification_count - 1; i >= 0; i--) {
            if (notifications[i].active) {
                create_notification_card(card_container, 
                                        notifications[i].title, 
                                        notifications[i].text,
                                        notifications[i].app);
            }
        }
    }
    
    is_refreshing = false;
}

// ==================== SCREEN SHOW CALLBACK ====================

static void alerts_screen_show_cb(lv_event_t *e) {
    // Refresh notifications whenever screen is shown
    ESP_LOGI(TAG, "Alerts screen shown, refreshing notifications");
    alerts_refresh_if_active();
}

// ==================== PUBLIC API ====================

void alerts_add_notification(const char *title, const char *message, const char *app) {
    ESP_LOGI(TAG, "New notification: %s - %s (app: %s)", title, message, app);
    
    // If buffer is full, shift notifications down (oldest gets removed)
    if (notification_count >= MAX_NOTIFICATIONS) {
        for (int i = 0; i < MAX_NOTIFICATIONS - 1; i++) {
            notifications[i] = notifications[i + 1];
        }
        notification_count = MAX_NOTIFICATIONS - 1;
    }
    
    // Add new notification at the end
    notification_t *notif = &notifications[notification_count];
    notif->active = true;
    
    // Copy title safely
    if (title) {
        strncpy(notif->title, title, MAX_TITLE_LEN - 1);
        notif->title[MAX_TITLE_LEN - 1] = '\0';
    } else {
        notif->title[0] = '\0';
    }
    
    // Copy text safely
    if (message) {
        strncpy(notif->text, message, MAX_TEXT_LEN - 1);
        notif->text[MAX_TEXT_LEN - 1] = '\0';
    } else {
        notif->text[0] = '\0';
    }
    
    // Copy app safely
    if (app) {
        strncpy(notif->app, app, MAX_APP_LEN - 1);
        notif->app[MAX_APP_LEN - 1] = '\0';
    } else {
        notif->app[0] = '\0';
    }
    
    notification_count++;
    notification_refresh_pending = true;  // Mark that screen needs refresh
    
    ESP_LOGI(TAG, "Notification added to queue (count=%d)", notification_count);
}

void alerts_clear_all(void) {
    ESP_LOGI(TAG, "Clearing all notifications");
    memset(notifications, 0, sizeof(notifications));
    notification_count = 0;
    refresh_alerts_screen();
}

int alerts_get_count(void) {
    return notification_count;
}

void alerts_refresh_if_active(void) {
    // Only refresh if the card container exists and not already refreshing
    if (card_container != NULL && !is_refreshing) {
        refresh_alerts_screen();
        notification_refresh_pending = false;
    } else {
        ESP_LOGD(TAG, "Refresh deferred: container=%p, is_refreshing=%d", card_container, is_refreshing);
    }
}

bool alerts_has_pending_refresh(void) {
    return notification_refresh_pending;
}
void alerts_event_cb_to_menu(lv_event_t *e){
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
    if (dir == LV_DIR_RIGHT) {
        ui_show_watchface();

    }
}
