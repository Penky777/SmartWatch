#ifndef MAX30102_H
#define MAX30102_H

#include "esp_err.h"
#include "driver/i2c_master.h"
#include <stdint.h>
#include <stdbool.h>

// Status structure
typedef struct {
    bool initialized;
    bool running;
    bool data_valid;
    uint8_t last_heart_rate;
    uint8_t last_spo2;
} max_status_t;

/**
 * @brief Initialize MAX30102 sensor
 * 
 * @param bus I2C bus handle
 * @return ESP_OK on success
 */
esp_err_t max_init(i2c_master_bus_handle_t bus);

/**
 * @brief Create background task for sensor reading
 * Call this after max_init()
 * 
 * @return ESP_OK on success
 */
esp_err_t max_create_task(void);

/**
 * @brief Start measurement
 */
void max_start(void);

/**
 * @brief Stop measurement
 */
void max_stop(void);

/**
 * @brief Check if measurement is running
 * 
 * @return true if running
 */
bool max_is_running(void);

/**
 * @brief Read latest heart rate and SpO2 values
 * 
 * @param spo2 Pointer to store SpO2 percentage (70-100)
 * @param heart_rate Pointer to store heart rate in BPM (40-200)
 * @return ESP_OK if data available, ESP_ERR_NOT_FOUND if no valid data
 */
esp_err_t max_read(uint8_t *spo2, uint8_t *heart_rate);

/**
 * @brief Read die temperature
 * 
 * @param temp Pointer to store temperature in Celsius
 * @return ESP_OK on success
 */
esp_err_t max_read_temperature(float *temp);

/**
 * @brief Get sensor status
 * 
 * @param status Pointer to status structure
 */
void max_get_status(max_status_t *status);

#endif // MAX30102_H
