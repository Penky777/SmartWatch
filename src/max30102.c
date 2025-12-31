// max30102.c - Heart rate and SpO2 sensor driver
// Simplified polling-based architecture

#include "max30102.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/i2c_master.h"
#include <stdio.h>
#include <string.h>

// MAX30102 I2C Address
#define MAX30102_I2C_ADDR 0x57

// MAX30102 Registers
#define MAX30102_REG_INT_STATUS_1   0x00
#define MAX30102_REG_INT_STATUS_2   0x01
#define MAX30102_REG_INT_ENABLE_1   0x02
#define MAX30102_REG_INT_ENABLE_2   0x03
#define MAX30102_REG_FIFO_WR_PTR    0x04
#define MAX30102_REG_FIFO_OVF_CNT   0x05
#define MAX30102_REG_FIFO_RD_PTR    0x06
#define MAX30102_REG_FIFO_DATA      0x07
#define MAX30102_REG_MODE_CONFIG    0x09
#define MAX30102_REG_SPO2_CONFIG    0x0A
#define MAX30102_REG_LED1_PA        0x0C  // Red LED
#define MAX30102_REG_LED2_PA        0x0D  // IR LED
#define MAX30102_REG_TEMP_INT       0x1F
#define MAX30102_REG_TEMP_FRAC      0x20
#define MAX30102_REG_REV_ID         0xFE
#define MAX30102_REG_PART_ID        0xFF

static const char *TAG = "MAX30102";

// Task handle
static TaskHandle_t max_task_handle = NULL;

// I2C handles
static i2c_master_bus_handle_t i2c_bus_ref = NULL;
static i2c_master_dev_handle_t max_dev_handle = NULL;

// Sensor state
static volatile bool running = false;
static volatile bool initialized = false;

// Latest readings (updated by background task)
static uint8_t g_last_spo2 = 0;
static uint8_t g_last_heart_rate = 0;
static bool g_data_valid = false;

// Test data counter (for fake data)
static uint32_t test_counter = 0;

/* ==================== PRIVATE I2C FUNCTIONS ==================== */

// Write single register
static esp_err_t max_write_reg(uint8_t reg, uint8_t value)
{
    if (!max_dev_handle) {
        ESP_LOGE(TAG, "Device not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    uint8_t data[2] = {reg, value};
    esp_err_t ret = i2c_master_transmit(max_dev_handle, data, 2, 100);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write reg 0x%02X: %s", reg, esp_err_to_name(ret));
    }
    
    return ret;
}

// Read single register
static esp_err_t max_read_reg(uint8_t reg, uint8_t *value)
{
    if (!max_dev_handle || !value) {
        return ESP_ERR_INVALID_ARG;
    }
    
    esp_err_t ret = i2c_master_transmit_receive(max_dev_handle, &reg, 1, value, 1, 100);
    
    if (ret != ESP_OK) {
        ESP_LOGD(TAG, "Failed to read reg 0x%02X: %s", reg, esp_err_to_name(ret));
    }
    
    return ret;
}

// Read multiple registers
static esp_err_t max_read_regs(uint8_t reg, uint8_t *buffer, size_t len)
{
    if (!max_dev_handle || !buffer) {
        return ESP_ERR_INVALID_ARG;
    }
    
    return i2c_master_transmit_receive(max_dev_handle, &reg, 1, buffer, len, 100);
}

/* ==================== SENSOR CONFIGURATION ==================== */

static esp_err_t max_reset(void)
{
    ESP_LOGI(TAG, "Resetting sensor...");
    
    // Software reset
    esp_err_t ret = max_write_reg(MAX30102_REG_MODE_CONFIG, 0x40);
    if (ret != ESP_OK) {
        return ret;
    }
    
    // Wait for reset to complete
    vTaskDelay(pdMS_TO_TICKS(50));
    
    // Verify reset completed
    uint8_t mode = 0;
    ret = max_read_reg(MAX30102_REG_MODE_CONFIG, &mode);
    if (ret == ESP_OK && (mode & 0x40)) {
        ESP_LOGW(TAG, "Reset bit still set after 50ms");
    }
    
    return ESP_OK;
}

static esp_err_t max_configure(void)
{
    ESP_LOGI(TAG, "Configuring sensor...");
    
    esp_err_t ret;
    
    // Clear FIFO pointers
    ret = max_write_reg(MAX30102_REG_FIFO_WR_PTR, 0x00);
    if (ret != ESP_OK) return ret;
    
    ret = max_write_reg(MAX30102_REG_FIFO_OVF_CNT, 0x00);
    if (ret != ESP_OK) return ret;
    
    ret = max_write_reg(MAX30102_REG_FIFO_RD_PTR, 0x00);
    if (ret != ESP_OK) return ret;
    
    // Set mode: SpO2 mode (red + IR LEDs)
    // Bit 2-0: 011 = SpO2 mode
    ret = max_write_reg(MAX30102_REG_MODE_CONFIG, 0x03);
    if (ret != ESP_OK) return ret;
    
    // Configure SpO2 sensor
    // Bit 6-5: Sample rate (00 = 50Hz, 01 = 100Hz, 10 = 200Hz, 11 = 400Hz)
    // Bit 4-2: LED pulse width (00 = 69us, 01 = 118us, 10 = 215us, 11 = 411us)
    // Bit 1-0: ADC range (00 = 2048, 01 = 4096, 10 = 8192, 11 = 16384)
    ret = max_write_reg(MAX30102_REG_SPO2_CONFIG, 0x27);  // 100Hz, 411us, 4096
    if (ret != ESP_OK) return ret;
    
    // Set LED currents (0x00 = 0mA, 0xFF = 51mA)
    // Start with moderate current
    ret = max_write_reg(MAX30102_REG_LED1_PA, 0x24);  // Red LED: ~7mA
    if (ret != ESP_OK) return ret;
    
    ret = max_write_reg(MAX30102_REG_LED2_PA, 0x24);  // IR LED: ~7mA
    if (ret != ESP_OK) return ret;
    
    ESP_LOGI(TAG, "Sensor configured successfully");
    return ESP_OK;
}

/* ==================== PUBLIC FUNCTIONS ==================== */

esp_err_t max_init(i2c_master_bus_handle_t bus)
{
    if (!bus) {
        ESP_LOGE(TAG, "Invalid I2C bus handle");
        return ESP_ERR_INVALID_ARG;
    }
    
    if (initialized) {
        ESP_LOGW(TAG, "Already initialized");
        return ESP_OK;
    }
    
    i2c_bus_ref = bus;
    
    ESP_LOGI(TAG, "Initializing MAX30102...");
    
    // Create I2C device handle
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = MAX30102_I2C_ADDR,
        .scl_speed_hz = 400000,  // 400kHz
    };
    
    esp_err_t ret = i2c_master_bus_add_device(bus, &dev_cfg, &max_dev_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add device: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Read Part ID to verify communication
    uint8_t part_id = 0;
    ret = max_read_reg(MAX30102_REG_PART_ID, &part_id);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read Part ID");
        i2c_master_bus_rm_device(max_dev_handle);
        max_dev_handle = NULL;
        return ret;
    }
    
    ESP_LOGI(TAG, "MAX30102 Part ID: 0x%02X", part_id);
    
    if (part_id != 0x15) {
        ESP_LOGW(TAG, "Unexpected Part ID (expected 0x15)");
    }
    
    // Read Revision ID
    uint8_t rev_id = 0;
    max_read_reg(MAX30102_REG_REV_ID, &rev_id);
    ESP_LOGI(TAG, "Revision ID: 0x%02X", rev_id);
    
    // Reset and configure sensor
    ret = max_reset();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Reset failed");
        return ret;
    }
    
    ret = max_configure();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Configuration failed");
        return ret;
    }
    
    initialized = true;
    ESP_LOGI(TAG, "MAX30102 initialized successfully");
    
    return ESP_OK;
}

void max_start(void)
{
    if (!initialized) {
        ESP_LOGW(TAG, "Cannot start - not initialized");
        return;
    }
    
    if (!running) {
        running = true;
        g_data_valid = false;
        test_counter = 0;
        ESP_LOGI(TAG, "Measurement started");
    }
}

void max_stop(void)
{
    if (running) {
        running = false;
        ESP_LOGI(TAG, "Measurement stopped");
    }
}

bool max_is_running(void)
{
    return running;
}

/* ==================== DATA READING ==================== */

esp_err_t max_read(uint8_t *spo2, uint8_t *heart_rate)
{
    if (!spo2 || !heart_rate) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (!initialized) {
        *spo2 = 0;
        *heart_rate = 0;
        return ESP_ERR_INVALID_STATE;
    }
    
    if (!running) {
        *spo2 = 0;
        *heart_rate = 0;
        return ESP_ERR_NOT_FOUND;
    }
    
    if (!g_data_valid) {
        *spo2 = 0;
        *heart_rate = 0;
        return ESP_ERR_NOT_FOUND;  // No data yet
    }
    
    // Return latest readings
    *spo2 = g_last_spo2;
    *heart_rate = g_last_heart_rate;
    
    return ESP_OK;
}

/* ==================== SENSOR READING (STUB) ==================== */

static esp_err_t max_read_sensor_data(uint8_t *spo2, uint8_t *heart_rate)
{
    if (!initialized || !max_dev_handle) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // TODO: Implement real sensor reading algorithm
    // This requires:
    // 1. Read FIFO data (red and IR samples)
    // 2. Process samples through PPG algorithm
    // 3. Detect heartbeat peaks
    // 4. Calculate heart rate from R-R intervals
    // 5. Calculate SpO2 from red/IR ratio
    
    // For now, generate realistic-looking test data
    test_counter++;
    
    // Simulate realistic heart rate: 65-85 BPM with some variation
    uint8_t base_hr = 72;
    int8_t variation = (test_counter % 20) - 10;  // ±10 BPM
    *heart_rate = base_hr + variation;
    
    // Simulate realistic SpO2: 95-99% with small variation
    uint8_t base_spo2 = 97;
    int8_t spo2_var = (test_counter % 6) - 3;  // ±3%
    *spo2 = base_spo2 + spo2_var;
    
    // Clamp values to valid ranges
    if (*heart_rate < 40) *heart_rate = 40;
    if (*heart_rate > 200) *heart_rate = 200;
    if (*spo2 < 70) *spo2 = 70;
    if (*spo2 > 100) *spo2 = 100;
    
    ESP_LOGD(TAG, "Sensor read: HR=%u BPM, SpO2=%u%%", *heart_rate, *spo2);
    
    return ESP_OK;
}

/* ==================== BACKGROUND TASK ==================== */

static void max_task(void *arg)
{
    (void)arg;
    
    ESP_LOGI(TAG, "MAX30102 task started");
    
    uint8_t spo2, hr;
    
    while (1)
    {
        if (running && initialized) {
            // Read sensor data
            esp_err_t ret = max_read_sensor_data(&spo2, &hr);
            
            if (ret == ESP_OK) {
                // Update global readings
                g_last_spo2 = spo2;
                g_last_heart_rate = hr;
                g_data_valid = true;
                
                ESP_LOGD(TAG, "Updated: HR=%u BPM, SpO2=%u%%", hr, spo2);
            } else {
                ESP_LOGW(TAG, "Failed to read sensor: %s", esp_err_to_name(ret));
                g_data_valid = false;
            }
            
            // Sample every second
            vTaskDelay(pdMS_TO_TICKS(1000));
        } else {
            // Not running - check less frequently
            g_data_valid = false;
            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }
}

/* ==================== TASK CREATION ==================== */

esp_err_t max_create_task(void)
{
    if (max_task_handle != NULL) {
        ESP_LOGW(TAG, "Task already created");
        return ESP_OK;
    }
    
    BaseType_t ret = xTaskCreate(
        max_task,
        "max30102_task",
        4096,
        NULL,
        5,
        &max_task_handle
    );
    
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create task");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Background task created");
    return ESP_OK;
}

/* ==================== UTILITY FUNCTIONS ==================== */

esp_err_t max_read_temperature(float *temp)
{
    if (!temp || !initialized) {
        return ESP_ERR_INVALID_ARG;
    }
    
    uint8_t temp_int, temp_frac;
    
    esp_err_t ret = max_read_reg(MAX30102_REG_TEMP_INT, &temp_int);
    if (ret != ESP_OK) return ret;
    
    ret = max_read_reg(MAX30102_REG_TEMP_FRAC, &temp_frac);
    if (ret != ESP_OK) return ret;
    
    // Temperature = integer + (fraction * 0.0625)
    *temp = (float)temp_int + ((float)temp_frac * 0.0625f);
    
    return ESP_OK;
}

void max_get_status(max_status_t *status)
{
    if (!status) return;
    
    memset(status, 0, sizeof(max_status_t));
    
    status->initialized = initialized;
    status->running = running;
    status->data_valid = g_data_valid;
    status->last_heart_rate = g_last_heart_rate;
    status->last_spo2 = g_last_spo2;
}
