#include "ui_manager.h"
#include "calculator_screen.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "UI_MANAGER";

// Forward declarations of your builders
lv_obj_t *build_menu_screen(void);
lv_obj_t *build_calendar_screen(void);
lv_obj_t *build_activity_screen(void);
lv_obj_t *build_settings_screen(void);
lv_obj_t *build_calculator_screen(void);
lv_obj_t *build_reset_screen(void);

// No more static screen caching - LVGL manages screen lifecycle
// pair_scr only kept for pairing flow control
static lv_obj_t *pair_scr = NULL;
static bool pairing_hidden = false;

// Forward declaration of helper functions
static void load_screen(lv_obj_t **slot, lv_obj_t *(*builder)(void));
static void delete_screen_timer_cb(lv_timer_t *timer);

void ui_manager_init(void) {
    pair_scr = NULL;
}

static void load_screen(lv_obj_t **slot, lv_obj_t *(*builder)(void)) {
    ESP_LOGI(TAG, "load_screen called");
    
    // Always create a fresh screen
    lv_obj_t *new_screen = builder();
    ESP_LOGI(TAG, "Created new screen: %p", new_screen);
    
    // Load WITHOUT auto-delete, DON'T cache the pointer
    // LVGL will keep the new screen as active, and we'll delete old one later if needed
    lv_scr_load_anim(new_screen, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, false);
    
    ESP_LOGI(TAG, "Screen loaded: %p", new_screen);
}

static void delete_screen_timer_cb(lv_timer_t *timer) {
    lv_obj_t *scr = (lv_obj_t*)lv_timer_get_user_data(timer);
    if (scr) {
        lv_obj_del(scr);
    }
}

extern lv_obj_t *build_brightness_screen(void);

void ui_show_brightness(void) {
    // Dummy pointer for load_screen - we don't cache screens anymore
    static lv_obj_t *dummy = NULL;
    load_screen(&dummy, build_brightness_screen);
}

void ui_show_pairing(int pin) {
    pair_scr = build_pairing_screen(pin);
    lv_scr_load_anim(pair_scr, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, false);
    pairing_hidden = false; // Reset flag when showing pairing
}

void ui_hide_pairing(void) {
    if (pairing_hidden) {
        ESP_LOGI(TAG, "Pairing already hidden, returning");
        return; // Already hidden
    }
    
    ESP_LOGI(TAG, "Hiding pairing screen");
    
    // Just load the menu screen, old pairing screen will be cleaned up later
    ui_show_menu();
    pairing_hidden = true;
}

void ui_show_calculator(void) {
    // Dummy pointer for load_screen - we don't cache screens anymore
    static lv_obj_t *dummy = NULL;
    load_screen(&dummy, build_calculator_screen);
}

void ui_show_reset(void) {
    // Dummy pointer for load_screen - we don't cache screens anymore
    static lv_obj_t *dummy = NULL;
    load_screen(&dummy, build_reset_screen);
}

void ui_show_menu(void) {
    // Dummy pointer for load_screen - we don't cache screens anymore
    static lv_obj_t *dummy = NULL;
    load_screen(&dummy, build_menu_screen);
}
void ui_show_calendar(void) {
    // Dummy pointer for load_screen - we don't cache screens anymore
    static lv_obj_t *dummy = NULL;
    load_screen(&dummy, build_calendar_screen);
}

void ui_show_activity(void) {
    // Dummy pointer for load_screen - we don't cache screens anymore
    static lv_obj_t *dummy = NULL;
    load_screen(&dummy, build_activity_screen);
}

void ui_show_settings(void) {
    // Dummy pointer for load_screen - we don't cache screens anymore
    static lv_obj_t *dummy = NULL;
    load_screen(&dummy, build_settings_screen);
}

