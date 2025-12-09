#pragma once
#include "stdint.h"
#include "stdbool.h"
#include "esp_err.h"
#include "driver/i2c_master.h"

typedef struct {
    uint8_t spo2;
    uint8_t heart_rate;
    bool valid;
} max_data_t;



esp_err_t max_init(i2c_master_bus_handle_t bus);
void max_start(void);
void max_stop(void);
void max_set_ui_update_callback(void (*cb)(uint16_t steps, uint8_t bpm, uint8_t spo2));
esp_err_t max_read(uint8_t *out_spo2, uint8_t *out_heart_rate);
