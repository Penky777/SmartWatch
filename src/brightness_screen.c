#include "brightness_screen.h"

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

// Event callback to go back to main menu
static void back_to_menu_cb(lv_event_t *e) {
    extern void build_ui(void); // main.c
    build_ui(); // reload main menu
}

#include "lvgl.h"

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

    // Optional: add back swipe event here

    lv_scr_load(scr);
}

