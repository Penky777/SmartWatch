// heart rate and SpO2 sensor MAX30102 driver source file

#include "max30102.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/i2c_master.h"
#include <stdio.h>

typedef struct {
    uint8_t spo2;
    uint8_t heart_rate;
    bool valid;
} max_data_t;

 static const char *TAG = "MAX30102";

 static TaskHandle_t max_task_handle = NULL;
 static i2c_master_bus_handle_t i2c_bus_ref = NULL;
 static void (*ui_callback)(uint16_t, uint8_t, uint8_t) = NULL;

 static volatile bool running = false;
 static uint16_t fake_steps = 0;

 /* ------------------------ PUBLIC FUNCTIONS ------------------------- */

esp_err_t max_init(i2c_master_bus_handle_t bus)
{
    i2c_bus_ref = bus;

    // TODO: perform sensor-specific initialization over I2C here.
    // For now, just store the bus handle and report success.
    ESP_LOGI(TAG, "MAX30102 initialized (stub)");
    return ESP_OK;
 }

 void max_set_ui_update_callback(void (*cb)(uint16_t, uint8_t, uint8_t))
 {
     ui_callback = cb;
 }

/* ------------------------ INTERNAL TASK --------------------------- */

 static void max_task(void *arg)
 {
     max_data_t data;

     while (1)
     {
         if (!running) {
             vTaskDelay(pdMS_TO_TICKS(200));
             continue;
         }

        // Read sensor
        if (max_read(&data.spo2, &data.heart_rate))
            data.valid = true;
        else
            data.valid = false;

         fake_steps++;   // for demo

         if (ui_callback && data.valid)
             ui_callback(fake_steps, data.heart_rate, data.spo2);

         vTaskDelay(pdMS_TO_TICKS(1000));   // sample each second
     }
 }

 /* ------------------------ START/STOP SENSOR ------------------------ */

void max_start(void)
{
    if (!running) {
        running = true;
        ESP_LOGI(TAG, "MAX30102 started");
    }
}

void max_stop(void)
{
    running = false;
    ESP_LOGI(TAG, "MAX30102 stopped");
}

// Simple stubbed read routine. Populate spo2 and heart_rate with
// deterministic fake values for testing. Returns non-zero on success
// to match existing usage in `max_task`.
esp_err_t max_read(uint8_t *spo2, uint8_t *heart_rate)
{
    if (!spo2 || !heart_rate) return 0;
    // Provide stable fake data; replace with real sensor read later.
    *spo2 = 98;          // fake SpO2 percentage
    *heart_rate = 60;    // fake BPM
    return 1;
}

 /* ----------------------- TASK CREATION ---------------------------- */

 __attribute__((constructor))
 static void create_task_after_boot(void)
 {
     xTaskCreate(max_task, "max_task", 4096, NULL, 5, &max_task_handle);
 }
