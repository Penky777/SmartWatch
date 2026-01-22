#include "lvgl.h"
#include "../Ui_manager/ui_manager.h"
#include <stdio.h>
#include "esp_system.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "RESET";

// Function to perform complete factory reset
static void perform_factory_reset(void) {
    ESP_LOGI(TAG, "=== FACTORY RESET STARTING ===");
    
    // 1. Clear BLE bonding data
    ESP_LOGI(TAG, "Clearing BLE bonding data...");
    nvs_handle_t handle;
    esp_err_t err = nvs_open("nimble_bond", NVS_READWRITE, &handle);
    if (err == ESP_OK) {
        nvs_erase_all(handle);
        nvs_commit(handle);
        nvs_close(handle);
        ESP_LOGI(TAG, "✓ BLE bonding data cleared");
    } else {
        ESP_LOGW(TAG, "Failed to open nimble_bond: %s", esp_err_to_name(err));
    }
    
    // 2. Erase ALL NVS storage (complete reset)
    ESP_LOGI(TAG, "Erasing entire NVS flash...");
    err = nvs_flash_erase();
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "✓ All NVS storage erased");
    } else {
        ESP_LOGW(TAG, "Failed to erase NVS: %s", esp_err_to_name(err));
    }
    
    // 3. Reinitialize NVS
    ESP_LOGI(TAG, "Reinitializing NVS...");
    err = nvs_flash_init();
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "✓ NVS reinitialized");
    }
    
    ESP_LOGI(TAG, "=== FACTORY RESET COMPLETE ===");
}

// Forward declarations
static void confirm_btn_event_cb(lv_event_t *e);
static void cancel_btn_event_cb(lv_event_t *e);
static void swipe_back_event_cb(lv_event_t *e);

lv_obj_t *build_reset_screen(void) {
    lv_obj_t *scr = lv_obj_create(NULL);
    
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    
    lv_obj_add_event_cb(scr, swipe_back_event_cb, LV_EVENT_GESTURE, NULL);

    // Title
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "Factory Reset");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFF5555), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

    // Warning message
    lv_obj_t *warning = lv_label_create(scr);
    lv_label_set_text(warning, "This will erase all data\nand settings!");
    lv_obj_set_style_text_color(warning, lv_color_white(), 0);
    lv_obj_set_style_text_font(warning, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_align(warning, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(warning, LV_ALIGN_CENTER, 0, -30);
    lv_obj_set_width(warning, LV_PCT(90));

    // Confirm button
    lv_obj_t *confirm_btn = lv_btn_create(scr);
    lv_obj_set_size(confirm_btn, 180, 50);
    lv_obj_align(confirm_btn, LV_ALIGN_CENTER, 0, 50);
    lv_obj_set_style_bg_color(confirm_btn, lv_color_hex(0xFF3333), 0);
    lv_obj_set_style_radius(confirm_btn, 25, 0);
    lv_obj_add_event_cb(confirm_btn, confirm_btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *confirm_lbl = lv_label_create(confirm_btn);
    lv_label_set_text(confirm_lbl, "RESET");
    lv_obj_set_style_text_color(confirm_lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(confirm_lbl, &lv_font_montserrat_20, 0);
    lv_obj_center(confirm_lbl);

    // Cancel button
    lv_obj_t *cancel_btn = lv_btn_create(scr);
    lv_obj_set_size(cancel_btn, 180, 50);
    lv_obj_align(cancel_btn, LV_ALIGN_CENTER, 0, 110);
    lv_obj_set_style_bg_color(cancel_btn, lv_color_hex(0x555555), 0);
    lv_obj_set_style_radius(cancel_btn, 25, 0);
    lv_obj_add_event_cb(cancel_btn, cancel_btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *cancel_lbl = lv_label_create(cancel_btn);
    lv_label_set_text(cancel_lbl, "Cancel");
    lv_obj_set_style_text_color(cancel_lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(cancel_lbl, &lv_font_montserrat_20, 0);
    lv_obj_center(cancel_lbl);

    // Hint
    lv_obj_t *hint = lv_label_create(scr);
    lv_label_set_text(hint, "← Swipe to go back");
    lv_obj_set_style_text_color(hint, lv_color_hex(0x666666), 0);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_14, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -10);

    return scr;
}

static void confirm_btn_event_cb(lv_event_t *e) {
    (void)e;
    
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    
    lv_obj_t *msg = lv_label_create(scr);
    lv_label_set_text(msg, "Factory Reset\nErasing all data...");
    lv_obj_set_style_text_color(msg, lv_color_white(), 0);
    lv_obj_set_style_text_font(msg, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_align(msg, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(msg);
    
    lv_scr_load(scr);
    
    // Perform complete factory reset
    perform_factory_reset();
    
    // Wait a moment before reboot
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // Reboot device
    ESP_LOGI(TAG, "Rebooting device...");
    esp_restart();
}

static void cancel_btn_event_cb(lv_event_t *e) {
    (void)e;
    ui_show_settings();
}

static void swipe_back_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_GESTURE) {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
        if (dir == LV_DIR_RIGHT) {
            ui_show_settings();
        }
    }
}
