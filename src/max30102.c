// #include "max30102.h"
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "esp_log.h"
// #include "driver/i2c_master.h"
// #include "max30102.h"       
// #include <stdio.h>

// static const char *TAG = "MAX30102";

// static TaskHandle_t max_task_handle = NULL;
// static i2c_master_bus_handle_t i2c_bus_ref = NULL;
// static void (*ui_callback)(uint16_t, uint8_t, uint8_t) = NULL;

// static volatile bool running = false;
// static uint16_t fake_steps = 0;

// /* ------------------------ PUBLIC FUNCTIONS ------------------------- */

// esp_err_t max_init(i2c_master_bus_handle_t bus)
// {
//     i2c_bus_ref = bus;

//     esp_err_t ret = max_init(bus);
//     if (ret != ESP_OK) {
//         ESP_LOGE(TAG, "MAX30102 Init failed");
//         return ret;
//     }

//     ESP_LOGI(TAG, "MAX30102 initialized");
//     return ESP_OK;
// }

// void max_set_ui_update_callback(void (*cb)(uint16_t, uint8_t, uint8_t))
// {
//     ui_callback = cb;
// }

// /* ------------------------ INTERNAL TASK --------------------------- */

// static void max_task(void *arg)
// {
//     max_data_t data;

//     while (1)
//     {
//         if (!running) {
//             vTaskDelay(pdMS_TO_TICKS(200));
//             continue;
//         }

//         // Read sensor
//         if (max_read(&data.spo2, &data.heart_rate))
//             data.valid = true;
//         else
//             data.valid = false;

//         fake_steps++;   // for demo

//         if (ui_callback && data.valid)
//             ui_callback(fake_steps, data.heart_rate, data.spo2);

//         vTaskDelay(pdMS_TO_TICKS(1000));   // sample each second
//     }
// }

// /* ------------------------ START/STOP SENSOR ------------------------ */

// void max_start(void)
// {
//     if (!running) {
//         running = true;
//         max_start();
//         ESP_LOGI(TAG, "MAX30102 started");
//     }
// }

// void max_stop(void)
// {
//     running = false;
//     max_stop();
//     ESP_LOGI(TAG, "MAX30102 stopped");
// }

// /* ----------------------- TASK CREATION ---------------------------- */

// __attribute__((constructor))
// static void create_task_after_boot(void)
// {
//     xTaskCreate(max_task, "max_task", 4096, NULL, 5, &max_task_handle);
// }
