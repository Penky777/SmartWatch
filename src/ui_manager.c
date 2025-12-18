#include "ui_manager.h"
#include "calculator_screen.h"


// Forward declarations of your builders
lv_obj_t *build_menu_screen(void);
lv_obj_t *build_calendar_screen(void);
lv_obj_t *build_activity_screen(void);
lv_obj_t *build_settings_screen(void);
lv_obj_t *build_calculator_screen(void);
lv_obj_t *build_reset_screen(void);

// Static screen pointers
static lv_obj_t *menu_scr = NULL;
static lv_obj_t *calendar_scr = NULL;
static lv_obj_t *activity_scr = NULL;
static lv_obj_t *settings_scr = NULL;
static lv_obj_t *calculator_scr = NULL;
static lv_obj_t *reset_scr = NULL;
static lv_obj_t *pair_scr = NULL;
static lv_obj_t *brightness_scr = NULL;

// Forward declaration of helper functions
static void load_screen(lv_obj_t **slot, lv_obj_t *(*builder)(void));
static void delete_screen_timer_cb(lv_timer_t *timer);

void ui_manager_init(void) {
    menu_scr = NULL;
    calendar_scr = NULL;
    activity_scr = NULL;
    settings_scr = NULL;
    calculator_scr = NULL;
    reset_scr = NULL;
    pair_scr = NULL;
    brightness_scr = NULL;
}

static void load_screen(lv_obj_t **slot, lv_obj_t *(*builder)(void)) {
    if (*slot == NULL) {
        *slot = builder();
    }
    lv_scr_load_anim(*slot, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, true);
}

static void delete_screen_timer_cb(lv_timer_t *timer) {
    lv_obj_t *scr = (lv_obj_t*)lv_timer_get_user_data(timer);
    if (scr) {
        lv_obj_del(scr);
    }
}

extern lv_obj_t *build_brightness_screen(void);

void ui_show_brightness(void) {
    if (brightness_scr == NULL)
        brightness_scr = build_brightness_screen();
    lv_scr_load_anim(brightness_scr, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, false);
}

void ui_show_pairing(int pin) {
    pair_scr = build_pairing_screen(pin);
    lv_scr_load_anim(pair_scr, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, false);
}

void ui_hide_pairing(void) {
    if (pair_scr != NULL) {
        lv_obj_del(pair_scr);
        pair_scr = NULL;
    }
    // Return to menu or previous screen
    ui_show_menu();
}

void ui_show_calculator(void) {
    load_screen(&calculator_scr, build_calculator_screen);
}

void ui_show_reset(void) {
    load_screen(&reset_scr, build_reset_screen);
}

void ui_show_menu(void) {
    lv_obj_t *old_calendar = calendar_scr;
    calendar_scr = NULL;
    
    load_screen(&menu_scr, build_menu_screen);
    
    // Delete old calendar after a short delay
    if (old_calendar != NULL) {
        lv_timer_t *timer = lv_timer_create(delete_screen_timer_cb, 100, old_calendar);
        lv_timer_set_repeat_count(timer, 1);
    }
}

void ui_show_calendar(void) {
    // Always rebuild calendar with current date
    if (calendar_scr != NULL) {
        lv_obj_del(calendar_scr);
        calendar_scr = NULL;
    }
    load_screen(&calendar_scr, build_calendar_screen);
}

void ui_show_activity(void) {
    load_screen(&activity_scr, build_activity_screen);
}

void ui_show_settings(void) {
    load_screen(&settings_scr, build_settings_screen);
}

