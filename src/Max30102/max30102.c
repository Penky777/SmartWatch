// max30102.c - Heart rate and SpO2 sensor driver with real algorithm
#include "max30102.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/i2c_master.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

// MAX30102 I2C Address
#define MAX30102_I2C_ADDR 0x57

// Registers
#define MAX30102_REG_INT_STATUS_1   0x00
#define MAX30102_REG_INT_ENABLE_1   0x02
#define MAX30102_REG_FIFO_WR_PTR    0x04
#define MAX30102_REG_FIFO_RD_PTR    0x06
#define MAX30102_REG_FIFO_DATA      0x07
#define MAX30102_REG_MODE_CONFIG    0x09
#define MAX30102_REG_SPO2_CONFIG    0x0A
#define MAX30102_REG_LED1_PA        0x0C  // Red LED
#define MAX30102_REG_LED2_PA        0x0D  // IR LED
#define MAX30102_REG_PART_ID        0xFF

static const char *TAG = "MAX30102";

// I2C handles
static i2c_master_bus_handle_t i2c_bus_ref = NULL;
static i2c_master_dev_handle_t max_dev_handle = NULL;

// State
static volatile bool running = false;
static volatile bool initialized = false;

// Latest readings
static uint8_t g_last_spo2 = 0;
static uint8_t g_last_heart_rate = 0;
static bool g_data_valid = false;

// Signal processing buffers
#define BUFFER_SIZE 100
static uint32_t ir_buffer[BUFFER_SIZE];
static uint32_t red_buffer[BUFFER_SIZE];
static uint8_t buffer_index = 0;

// Peak detection for heart rate
#define MIN_PEAK_DISTANCE 20  // Minimum samples between peaks (~200ms at 100Hz)
#define PEAK_THRESHOLD 10000  // Minimum IR value to consider valid signal

typedef struct {
    uint32_t last_peak_time;
    uint32_t peak_count;
    uint32_t interval_sum;
    bool finger_detected;
} hr_state_t;

static hr_state_t hr_state = {0};

/* ==================== I2C FUNCTIONS ==================== */

static esp_err_t max_write_reg(uint8_t reg, uint8_t value) {
    if (!max_dev_handle) return ESP_ERR_INVALID_STATE;
    
    uint8_t data[2] = {reg, value};
    return i2c_master_transmit(max_dev_handle, data, 2, 100);
}

static esp_err_t max_read_reg(uint8_t reg, uint8_t *value) {
    if (!max_dev_handle || !value) return ESP_ERR_INVALID_ARG;
    return i2c_master_transmit_receive(max_dev_handle, &reg, 1, value, 1, 100);
}

static esp_err_t max_read_fifo(uint32_t *red, uint32_t *ir) {
    uint8_t data[6];
    uint8_t reg = MAX30102_REG_FIFO_DATA;
    
    esp_err_t ret = i2c_master_transmit_receive(max_dev_handle, &reg, 1, data, 6, 100);
    if (ret != ESP_OK) return ret;
    
    // Combine 3 bytes for each LED (18-bit data)
    *red = ((uint32_t)data[0] << 16) | ((uint32_t)data[1] << 8) | data[2];
    *ir  = ((uint32_t)data[3] << 16) | ((uint32_t)data[4] << 8) | data[5];
    
    // Mask to 18 bits
    *red &= 0x3FFFF;
    *ir  &= 0x3FFFF;
    
    return ESP_OK;
}

/* ==================== CONFIGURATION ==================== */

static esp_err_t max_reset(void) {
    ESP_LOGI(TAG, "Resetting sensor...");
    
    esp_err_t ret = max_write_reg(MAX30102_REG_MODE_CONFIG, 0x40);
    if (ret != ESP_OK) return ret;
    
    vTaskDelay(pdMS_TO_TICKS(100));
    return ESP_OK;
}

static esp_err_t max_configure(void) {
    ESP_LOGI(TAG, "Configuring sensor...");
    
    // Clear FIFO
    max_write_reg(MAX30102_REG_FIFO_WR_PTR, 0x00);
    max_write_reg(MAX30102_REG_FIFO_RD_PTR, 0x00);
    
    // SpO2 mode (red + IR)
    max_write_reg(MAX30102_REG_MODE_CONFIG, 0x03);
    
    // 100Hz sample rate, 411us pulse width, 4096 ADC range
    max_write_reg(MAX30102_REG_SPO2_CONFIG, 0x27);
    
    // LED currents: 7mA each
    max_write_reg(MAX30102_REG_LED1_PA, 0x24);  // Red
    max_write_reg(MAX30102_REG_LED2_PA, 0x24);  // IR
    
    ESP_LOGI(TAG, "Sensor configured");
    return ESP_OK;
}

/* ==================== SIGNAL PROCESSING ==================== */

// Simple peak detection algorithm
static bool detect_peak(uint32_t current_ir, uint32_t *sample_count) {
    static uint32_t prev_ir = 0;
    static uint32_t prev_prev_ir = 0;
    static uint32_t samples_since_peak = 0;
    
    (*sample_count)++;
    samples_since_peak++;
    
    // Check for finger presence
    if (current_ir < PEAK_THRESHOLD) {
        hr_state.finger_detected = false;
        prev_ir = current_ir;
        prev_prev_ir = prev_ir;
        return false;
    }
    
    hr_state.finger_detected = true;
    
    // Detect peak: current > prev AND prev > prev_prev (local maximum)
    bool is_peak = (prev_ir > prev_prev_ir) && 
                   (prev_ir > current_ir) && 
                   (samples_since_peak > MIN_PEAK_DISTANCE);
    
    if (is_peak) {
        samples_since_peak = 0;
    }
    
    // Shift history
    prev_prev_ir = prev_ir;
    prev_ir = current_ir;
    
    return is_peak;
}

// Calculate heart rate from R-R intervals
static uint8_t calculate_heart_rate(uint32_t interval_samples, uint32_t sample_rate) {
    if (interval_samples == 0) return 0;
    
    // HR (BPM) = (60 * sample_rate) / interval_samples
    uint32_t hr = (60 * sample_rate) / interval_samples;
    
    // Clamp to valid range
    if (hr < 40) return 0;   // Too slow, probably invalid
    if (hr > 200) return 0;  // Too fast, probably noise
    
    return (uint8_t)hr;
}

// Calculate SpO2 from red/IR ratio
static uint8_t calculate_spo2(uint32_t red_avg, uint32_t ir_avg) {
    if (ir_avg == 0) return 0;
    
    // R = (AC_red / DC_red) / (AC_ir / DC_ir)
    // SpO2 ≈ 110 - 25 * R (simplified calibration)
    
    // Simplified: just use DC ratio for now
    float ratio = (float)red_avg / (float)ir_avg;
    
    // Empirical formula (needs calibration)
    int32_t spo2 = (int32_t)(110.0f - 25.0f * ratio);
    
    // Clamp to valid range
    if (spo2 < 70) return 70;
    if (spo2 > 100) return 100;
    
    return (uint8_t)spo2;
}

/* ==================== DATA ACQUISITION ==================== */

static esp_err_t max_read_sensor_data(uint8_t *spo2, uint8_t *heart_rate) {
    if (!initialized) return ESP_ERR_INVALID_STATE;
    
    uint32_t red, ir;
    esp_err_t ret = max_read_fifo(&red, &ir);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "FIFO read failed");
        return ret;
    }
    
    // Store in circular buffer
    red_buffer[buffer_index] = red;
    ir_buffer[buffer_index] = ir;
    buffer_index = (buffer_index + 1) % BUFFER_SIZE;
    
    // Detect peaks for heart rate
    static uint32_t sample_count = 0;
    static uint32_t last_peak_sample = 0;
    
    if (detect_peak(ir, &sample_count)) {
        uint32_t interval = sample_count - last_peak_sample;
        last_peak_sample = sample_count;
        
        // Calculate heart rate (100 Hz sample rate)
        uint8_t hr = calculate_heart_rate(interval, 100);
        
        if (hr > 0) {
            *heart_rate = hr;
            ESP_LOGI(TAG, "Peak detected! HR: %u BPM", hr);
        }
    }
    
    // Calculate SpO2 from average values every 100 samples
    if (buffer_index == 0) {
        uint32_t red_sum = 0, ir_sum = 0;
        
        for (int i = 0; i < BUFFER_SIZE; i++) {
            red_sum += red_buffer[i];
            ir_sum += ir_buffer[i];
        }
        
        uint32_t red_avg = red_sum / BUFFER_SIZE;
        uint32_t ir_avg = ir_sum / BUFFER_SIZE;
        
        *spo2 = calculate_spo2(red_avg, ir_avg);
        
        ESP_LOGI(TAG, "SpO2 calculated: %u%% (Red avg: %lu, IR avg: %lu)", 
                 *spo2, red_avg, ir_avg);
    }
    
    // Check finger presence
    if (!hr_state.finger_detected) {
        ESP_LOGD(TAG, "No finger detected (IR: %lu)", ir);
        return ESP_ERR_NOT_FOUND;
    }
    
    return ESP_OK;
}

/* ==================== PUBLIC API ==================== */

esp_err_t max_init(i2c_master_bus_handle_t bus) {
    if (!bus) return ESP_ERR_INVALID_ARG;
    if (initialized) return ESP_OK;
    
    i2c_bus_ref = bus;
    ESP_LOGI(TAG, "Initializing MAX30102...");
    
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = MAX30102_I2C_ADDR,
        .scl_speed_hz = 400000,
    };
    
    esp_err_t ret = i2c_master_bus_add_device(bus, &dev_cfg, &max_dev_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add device: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Verify Part ID
    uint8_t part_id = 0;
    ret = max_read_reg(MAX30102_REG_PART_ID, &part_id);
    if (ret != ESP_OK || part_id != 0x15) {
        ESP_LOGE(TAG, "Invalid Part ID: 0x%02X", part_id);
        return ESP_ERR_NOT_FOUND;
    }
    
    ESP_LOGI(TAG, "MAX30102 detected (Part ID: 0x%02X)", part_id);
    
    // Reset and configure
    max_reset();
    max_configure();
    
    initialized = true;
    memset(&hr_state, 0, sizeof(hr_state));
    
    ESP_LOGI(TAG, "MAX30102 initialized");
    return ESP_OK;
}

void max_start(void) {
    if (!initialized) {
        ESP_LOGW(TAG, "Cannot start - not initialized");
        return;
    }
    running = true;
    g_data_valid = false;
    buffer_index = 0;
    ESP_LOGI(TAG, "Measurement started");
}

void max_stop(void) {
    running = false;
    ESP_LOGI(TAG, "Measurement stopped");
}

esp_err_t max_read(uint8_t *spo2, uint8_t *heart_rate) {
    if (!spo2 || !heart_rate) return ESP_ERR_INVALID_ARG;
    if (!initialized || !running) {
        *spo2 = 0;
        *heart_rate = 0;
        return ESP_ERR_INVALID_STATE;
    }
    
    if (!g_data_valid) {
        *spo2 = 0;
        *heart_rate = 0;
        return ESP_ERR_NOT_FOUND;
    }
    
    *spo2 = g_last_spo2;
    *heart_rate = g_last_heart_rate;
    return ESP_OK;
}

/* ==================== BACKGROUND TASK ==================== */

static void max_task(void *arg) {
    ESP_LOGI(TAG, "MAX30102 task started");
    
    uint8_t spo2 = 0, hr = 0;
    
    while (1) {
        if (running && initialized) {
            esp_err_t ret = max_read_sensor_data(&spo2, &hr);
            
            if (ret == ESP_OK) {
                g_last_spo2 = spo2;
                g_last_heart_rate = hr;
                g_data_valid = true;
            }
            
            // 100Hz sampling = 10ms delay
            vTaskDelay(pdMS_TO_TICKS(10));
        } else {
            g_data_valid = false;
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

esp_err_t max_create_task(void) {
    BaseType_t ret = xTaskCreate(max_task, "max30102", 4096, NULL, 5, NULL);
    return (ret == pdPASS) ? ESP_OK : ESP_FAIL;
}

bool max_is_running(void) {
    return running;
}

void max_get_status(max_status_t *status) {
    if (!status) return;
    
    status->initialized = initialized;
    status->running = running;
    status->data_valid = g_data_valid;
    status->last_heart_rate = g_last_heart_rate;
    status->last_spo2 = g_last_spo2;
}
