#include "ui_manager.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "watchface_screen.h"

static const char *TAG = "UI_MANAGER";

#define MAX_SCREEN_HISTORY 10
static lv_obj_t *screen_history[MAX_SCREEN_HISTORY] = {0};
static int screen_history_count = 0;

// ========== FORWARD DECLARATIONS ==========
lv_obj_t *build_watchface_screen(void);
lv_obj_t *build_menu_screen(void);
lv_obj_t *build_calendar_screen(void);
lv_obj_t *build_activity_screen(void);
lv_obj_t *build_settings_screen(void);
lv_obj_t *build_calculator_screen(void);
lv_obj_t *build_reset_screen(void);
lv_obj_t *build_brightness_screen(void);
lv_obj_t *build_time_screen(void);
lv_obj_t *build_pairing_screen(int pin);
lv_obj_t *build_flashlight_screen(void);
lv_obj_t *build_detail_screen(const char *date);


static void switch_screen(lv_obj_t **cache, lv_obj_t *(*builder)(void), const char *name);

// ========== SCREEN CACHE ==========
static lv_obj_t *screen_watchface = NULL;
static lv_obj_t *screen_menu = NULL;
static lv_obj_t *screen_calendar = NULL;
static lv_obj_t *screen_activity = NULL;
static lv_obj_t *screen_settings = NULL;
static lv_obj_t *screen_calculator = NULL;
static lv_obj_t *screen_brightness = NULL;
static lv_obj_t *screen_reset = NULL;
static lv_obj_t *screen_time = NULL;
static lv_obj_t *screen_pairing = NULL;
static lv_obj_t *screen_flashlight = NULL;
static lv_obj_t *current_screen = NULL;
static lv_obj_t *screen_detail = NULL;


// ========== HELPER FUNCTIONS ==========

static void log_heap(const char *ctx) {
    ESP_LOGI(TAG, "[%s] Heap: %u | Largest: %u", ctx,
        (unsigned)esp_get_free_heap_size(),
        (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
}

static void push_screen_history(lv_obj_t *screen) {
    if (screen_history_count < MAX_SCREEN_HISTORY) {
        screen_history[screen_history_count++] = screen;
        ESP_LOGD(TAG, "Screen history: %d screens", screen_history_count);
    } else {
        ESP_LOGW(TAG, "Screen history full! Cannot store more.");
    }
}

static lv_obj_t *pop_screen_history(void) {
    if (screen_history_count > 0) {
        screen_history_count--;
        lv_obj_t *prev = screen_history[screen_history_count];
        screen_history[screen_history_count] = NULL;
        ESP_LOGI(TAG, "Going back → Screen history: %d screens", screen_history_count);
        return prev;
    }
    return NULL;
}

// ========== INITIALIZATION ==========

void ui_manager_init(void) {
    ESP_LOGI(TAG, "Init UI Manager");
    log_heap("before");
    
    screen_watchface = NULL;  
    screen_menu = NULL;
    screen_calendar = NULL;
    screen_activity = NULL;
    screen_settings = NULL;
    screen_calculator = NULL;
    screen_brightness = NULL;
    screen_reset = NULL;
    screen_time = NULL;
    screen_pairing = NULL;
    screen_flashlight = NULL;
    current_screen = NULL;
    screen_detail = NULL;


    screen_history_count = 0;
    for (int i = 0; i < MAX_SCREEN_HISTORY; i++) {
        screen_history[i] = NULL;
    }
    
    log_heap("after");
    ESP_LOGI(TAG, "UI Manager ready (screens will be created on-demand)");
}

// ========== SCREEN SWITCHING ==========

static void switch_screen(lv_obj_t **cache, lv_obj_t *(*builder)(void), const char *name) {
    ESP_LOGI(TAG, "Switch to %s", name);
    log_heap("before switch");
    
    if (current_screen) {
        lv_anim_del(current_screen, NULL);
    }
    
    if (!(*cache)) {
        ESP_LOGI(TAG, "Creating %s screen...", name);
        *cache = builder();
        
        if (!(*cache)) {
            ESP_LOGE(TAG, "Failed to create %s screen!", name);
            return;
        }
        
        ESP_LOGI(TAG, "Created %s screen: %p", name, *cache);
    }
    
    if (current_screen == *cache) {
        ESP_LOGI(TAG, "%s already active", name);
        return;
    }
    
    
    if (current_screen != NULL && current_screen != *cache) {
        push_screen_history(current_screen);
    }
    
    ESP_LOGI(TAG, "Loading %s screen directly", name);
    lv_scr_load(*cache);
    current_screen = *cache;
    
    log_heap("after switch");
    vTaskDelay(pdMS_TO_TICKS(10));  // Give time for screen to settle
}

// ========== PUBLIC SCREEN FUNCTIONS ==========

void ui_show_detail(const char *date) {
    ESP_LOGI(TAG, "Show detail for: %s", date);
    
    // Delete old detail screen
    if (screen_detail) {
        lv_obj_del(screen_detail);
        screen_detail = NULL;
    }
    
    // Stop animations
    if (current_screen) {
        lv_anim_del(current_screen, NULL);
    }
    
    // Save to history
    if (current_screen) {
        push_screen_history(current_screen);
    }
    
    // Create and show
    screen_detail = build_detail_screen(date);
    lv_scr_load(screen_detail);
    current_screen = screen_detail;
    
    vTaskDelay(pdMS_TO_TICKS(10));
}

void ui_show_watchface(void) {
    switch_screen(&screen_watchface, build_watchface_screen, "WATCHFACE");
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

void ui_show_flashlight(void) {
    switch_screen(&screen_flashlight, build_flashlight_screen, "FLASHLIGHT");
    
    // Set brightness to 100%
    extern void backlight_set(uint8_t percent);
    extern uint8_t g_backlight_level;
    
    backlight_set(100);
    g_backlight_level = 100;
    
    ESP_LOGI(TAG, "Flashlight ON - brightness at 100%%");
}

void ui_show_pairing(int pin) {
    ESP_LOGI(TAG, "Show pairing (PIN: %d)", pin);
    
    // Stop animations
    if (current_screen) {
        lv_anim_del(current_screen, NULL);
    }
    
    // Save to history
    if (current_screen != NULL) {
        push_screen_history(current_screen);
    }

    // Delete old pairing screen
    if (screen_pairing) {
        lv_obj_del(screen_pairing);
    }
    
    screen_pairing = build_pairing_screen(pin);
    
    // no animation
    lv_scr_load(screen_pairing);
    current_screen = screen_pairing;
    
    vTaskDelay(pdMS_TO_TICKS(10));
}

void ui_hide_pairing(void) {
    ESP_LOGI(TAG, "Hide pairing");
    
    if (screen_pairing) {
        lv_obj_del(screen_pairing);
        screen_pairing = NULL;
    }
    
    if (ui_can_go_back()) {
        ui_go_back();
    } else {
        ui_show_menu();
    }
}

// ========== BACK NAVIGATION ==========

bool ui_can_go_back(void) {
    return screen_history_count > 0;
}

void ui_go_back(void) {
    if (!ui_can_go_back()) {
        ESP_LOGI(TAG, "No screen history - staying on current screen");
        return;
    }
    
    lv_obj_t *prev_screen = pop_screen_history();
    if (prev_screen) {
        ESP_LOGI(TAG, "Going back to previous screen");
        
        if (current_screen) {
            lv_anim_del(current_screen, NULL);
        }
        
        lv_scr_load(prev_screen);
        current_screen = prev_screen;
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
