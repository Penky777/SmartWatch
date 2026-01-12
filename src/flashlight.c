#include "lvgl.h"
#include "esp_log.h"

static const char *TAG = "FLASHLIGHT";

extern void backlight_set(uint8_t percent);
extern uint8_t g_backlight_level;
extern uint32_t g_last_activity_ms;

static uint8_t brightness_before_flashlight = 100;

lv_obj_t *build_flashlight_screen(void) {
    ESP_LOGI(TAG, "Building flashlight screen");
    
    lv_obj_t *screen = lv_obj_create(NULL);
    
    // Set pure white background
    lv_obj_set_style_bg_color(screen, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    
    // Optional: Add a small exit hint at bottom
    lv_obj_t *label = lv_label_create(screen);
    lv_label_set_text(label, "Press button to exit");
    lv_obj_set_style_text_color(label, lv_color_black(), 0);
    lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, -10);
    
    ESP_LOGI(TAG, "Flashlight screen created");
    return screen;
}
