#include "brightness_screen.h"
#include "lvgl.h"
#include "settings_screen.h"


// Keep a static handle to the slider (optional)
static lv_obj_t *brightness_slider = NULL;

// Event callback for the slider
static void brightness_slider_event_cb(lv_event_t *e) {
    lv_obj_t *slider = lv_event_get_target(e);
    int val = lv_slider_get_value(slider);

    // Adjust backlight
    extern void backlight_set(uint8_t p); // main.c has this
    backlight_set((uint8_t)val);
}

static void brightness_swipe_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_GESTURE) {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
        if (dir == LV_DIR_RIGHT) {
            // Get current screen
            lv_obj_t *old_scr = lv_scr_act();
            
            // Create and load settings
            lv_obj_t *settings_scr = lv_obj_create(NULL);
            build_settings_screen(settings_scr);
            lv_scr_load(settings_scr);
            
            // Delete old screen
            lv_obj_del(old_scr);
        }
    }
}




void brightness_screen_init(lv_event_t * e) {
    // You can ignore the argument if you don't need it:
    (void)e;

    // Create new screen
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);

    // Create slider
    lv_obj_t *slider = lv_slider_create(scr);
    lv_slider_set_range(slider, 5, 100);
    lv_slider_set_value(slider, 100, LV_ANIM_OFF);
    lv_obj_center(slider);
    
    lv_obj_add_event_cb(scr, brightness_swipe_event_cb, LV_EVENT_GESTURE, NULL);
    

    lv_scr_load(scr);
}

