#pragma once

#include "esp_err.h"
#include "driver/i2c_master.h"
#include <stdint.h>
#include <stdbool.h>

#define MAX30102_ADDR 0x57

esp_err_t max_init(i2c_master_bus_handle_t bus);
void max_stop(void);
esp_err_t max_read(uint8_t *spo2, uint8_t *heart_rate);