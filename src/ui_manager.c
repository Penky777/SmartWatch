#include "ui_manager.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"

static const char *TAG = "UI_MANAGER";

// Forward declarations
lv_obj_t *build_menu_screen(void);
lv_obj_t *build_calendar_screen(void);
lv_obj_t *build_activity_screen(void);
lv_obj_t *build_settings_screen(void);
lv_obj_t *build_calculator_screen(void);
lv_obj_t *build_reset_screen(void);
lv_obj_t *build_brightness_screen(void);
lv_obj_t *build_time_screen(void);
lv_obj_t *build_pairing_screen(int pin);

// Cache screens - created once, reused forever
static lv_obj_t *screen_menu = NULL;
static lv_obj_t *screen_calendar = NULL;
static lv_obj_t *screen_activity = NULL;
static lv_obj_t *screen_settings = NULL;
static lv_obj_t *screen_calculator = NULL;
static lv_obj_t *screen_brightness = NULL;
static lv_obj_t *screen_reset = NULL;
static lv_obj_t *screen_time = NULL;
static lv_obj_t *screen_pairing = NULL;
static lv_obj_t *current_screen = NULL;

static void log_heap(const char *ctx) {
    ESP_LOGI(TAG, "[%s] Heap: %u | Largest: %u", ctx,
        (unsigned)esp_get_free_heap_size(),
        (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
}

void ui_manager_init(void) {
    ESP_LOGI(TAG, "Init UI Manager");
    log_heap("before");
    
    // Initialize all cache pointers to NULL
    screen_menu = NULL;
    screen_calendar = NULL;
    screen_activity = NULL;
    screen_settings = NULL;
    screen_calculator = NULL;
    screen_brightness = NULL;
    screen_reset = NULL;
    screen_time = NULL;
    screen_pairing = NULL;
    current_screen = NULL;
    
    log_heap("after");
    ESP_LOGI(TAG, "UI Manager ready (screens will be created on-demand)");
}

static void switch_screen(lv_obj_t **cache, lv_obj_t *(*builder)(void), const char *name) {
    ESP_LOGI(TAG, "Switch to %s", name);
    log_heap("before switch");
    
    // Create if not cached
    if (!(*cache)) {
        ESP_LOGI(TAG, "Creating %s screen...", name);
        *cache = builder();
        
        if (!(*cache)) {
            ESP_LOGE(TAG, "Failed to create %s screen!", name);
            return;
        }
        
        ESP_LOGI(TAG, "Created %s screen: %p", name, *cache);
    }
    
    // Skip if already showing
    if (current_screen == *cache) {
        ESP_LOGI(TAG, "%s already active", name);
        return;
    }
    
    // First screen load - no animation
    if (current_screen == NULL) {
        ESP_LOGI(TAG, "First screen load - no animation");
        lv_scr_load(*cache);
        current_screen = *cache;
    } else {
        // Subsequent loads - with animation
        // ✅ CHANGED: false means DON'T auto-delete old screen (we manage them)
        ESP_LOGI(TAG, "Loading with animation (keeping old screen cached)");
        lv_scr_load_anim(*cache, LV_SCR_LOAD_ANIM_FADE_IN, 150, 0, false);
        current_screen = *cache;
    }
    
    log_heap("after switch");
    vTaskDelay(1);
}

void ui_show_menu(void) {
    switch_screen(&screen_menu, build_menu_screen, "MENU");
}

void ui_show_calendar(void) {
    switch_screen(&screen_calendar, build_calendar_screen, "CALENDAR");
}

void ui_show_activity(void) {
    switch_screen(&screen_activity, build_activity_screen, "ACTIVITY");
}

void ui_show_settings(void) {
    switch_screen(&screen_settings, build_settings_screen, "SETTINGS");
}

void ui_show_calculator(void) {
    switch_screen(&screen_calculator, build_calculator_screen, "CALCULATOR");
}

void ui_show_brightness(void) {
    switch_screen(&screen_brightness, build_brightness_screen, "BRIGHTNESS");
}

void ui_show_reset(void) {
    switch_screen(&screen_reset, build_reset_screen, "RESET");
}

void ui_show_time(void) {
    switch_screen(&screen_time, build_time_screen, "TIME");
}

void ui_show_pairing(int pin) {
    ESP_LOGI(TAG, "Show pairing (PIN: %d)", pin);
    
    // Always recreate pairing screen (PIN changes each time)
    if (screen_pairing) {
        lv_obj_del(screen_pairing);
    }
    
    screen_pairing = build_pairing_screen(pin);
    
    if (current_screen == NULL) {
        lv_scr_load(screen_pairing);
    } else {
        // ✅ CHANGED: false here too
        lv_scr_load_anim(screen_pairing, LV_SCR_LOAD_ANIM_FADE_IN, 150, 0, false);
    }
    
    current_screen = screen_pairing;
    vTaskDelay(1);
}

void ui_hide_pairing(void) {
    ESP_LOGI(TAG, "Hide pairing");
    
    if (screen_pairing) {
        lv_obj_del(screen_pairing);
        screen_pairing = NULL;
    }
    
    ui_show_menu();
}
