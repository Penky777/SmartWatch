#include "max30102.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/i2c_master.h"
#include "max30102.h"       
#include <stdio.h>
#include "driver/i2c.h"

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
    
    // Initialize I2C here
    i2c_config_t conf = {
    .mode = I2C_MODE_MASTER,
    .sda_io_num = 21,
    .scl_io_num = 22,
    .sda_pullup_en = GPIO_PULLUP_ENABLE,
    .scl_pullup_en = GPIO_PULLUP_ENABLE,
    .master = {
        .clk_speed = 400000
    }
};

    i2c_param_config(I2C_NUM_0, &conf);
    i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0);

    // Configure MAX30102 registers here via I2C
    // For example: write mode config, LED pulse amplitude, etc.

    ESP_LOGI(TAG, "MAX30102 initialized");
    return ESP_OK;
}

void max_set_ui_update_callback(void (*cb)(uint16_t, uint8_t, uint8_t))
{
    ui_callback = cb;
}



esp_err_t max_read(uint8_t *spo2, uint8_t *heart_rate)
{
    *spo2 = 98;       // dummy data
    *heart_rate = 70; // dummy data
    return ESP_OK;    // ESP_OK = 0
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

        if (max_read(&data.spo2, &data.heart_rate) == ESP_OK)
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
    if (running) {
        running = false;
        ESP_LOGI(TAG, "MAX30102 stopped");
    }
}

/* ----------------------- TASK CREATION ---------------------------- */

__attribute__((constructor))
static void create_task_after_boot(void)
{
    xTaskCreate(max_task, "max_task", 4096, NULL, 5, &max_task_handle);
}
