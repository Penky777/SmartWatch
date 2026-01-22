#include "bsp_qmi8658.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>
#include <string.h>

#include "esp_log.h"

#include "bsp_i2c.h"

static const char *TAG = "bsp_qmi8658";
#define ACC_SENSITIVITY (4 / 32768.0f) // 每LSB对应的加速度（单位为g）

static i2c_master_dev_handle_t dev_handle;

// Step counting variables
static uint32_t step_count = 0;
static bool step_detected = false;
static uint32_t last_step_time = 0;


// 读取QMI8658寄存器的值
static esp_err_t bsp_qmi8658_reg_read(uint8_t reg_addr, uint8_t *data, size_t len)
{
    //return bsp_i2c_reg8_read(QMI8658_SENSOR_ADDR, reg_addr, data, len);
    return i2c_master_transmit_receive(dev_handle, &reg_addr, 1, data, len, pdMS_TO_TICKS(1000));
}

// 给QMI8658的寄存器写值
static esp_err_t bsp_qmi8658_reg_write_byte(uint8_t reg_addr, uint8_t *data, size_t len)
{
    uint8_t buf[len + 1];
    buf[0] = reg_addr;
    memcpy(buf + 1, data, len);
    //return bsp_i2c_reg8_write(QMI8658_SENSOR_ADDR, reg_addr, data, len);
    return i2c_master_transmit(dev_handle, buf, len + 1, pdMS_TO_TICKS(1000));
}

bool bsp_qmi8658_read_data(qmi8658_data_t *data)
{
    uint8_t status;
    float mask;
    uint16_t buf[6];
    bsp_qmi8658_reg_read(QMI8658_STATUS0, &status, 1); // 读状态寄存器
    if (status & 0x03)
    {
        bsp_qmi8658_reg_read(QMI8658_AX_L, (uint8_t *)buf, 12); // 读加速度和陀螺仪值
        data->acc_x = buf[0];
        data->acc_y = buf[1];
        data->acc_z = buf[2];
        data->gyr_x = buf[3];
        data->gyr_y = buf[4];
        data->gyr_z = buf[5];
        // ESP_LOGI(TAG, "QMI8658 read data success!");

        mask = (float)data->acc_x / sqrt(((float)data->acc_y * (float)data->acc_y + (float)data->acc_z * (float)data->acc_z));
        data->AngleX = atan(mask) * 57.29578f; // 180/π=57.29578
        mask = (float)data->acc_y / sqrt(((float)data->acc_x * (float)data->acc_x + (float)data->acc_z * (float)data->acc_z));
        data->AngleY = atan(mask) * 57.29578f; // 180/π=57.29578
        mask = sqrt(((float)data->acc_x * (float)data->acc_x + (float)data->acc_y * (float)data->acc_y)) / (float)data->acc_z;
        data->AngleZ = atan(mask) * 57.29578f; // 180/π=57.29578
        return true;
    }
    return false;
}

// Read step count from the sensor's built-in pedometer
uint32_t bsp_qmi8658_read_steps(void)
{
    uint8_t buf[3];
    esp_err_t ret = bsp_qmi8658_reg_read(QMI8658_STEP_CNT_LOW, buf, 3);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read step count");
        return 0;
    }
    uint32_t steps = buf[0] | (buf[1] << 8) | (buf[2] << 16);
    return steps;
}

// Get software step count
uint32_t bsp_qmi8658_get_software_steps(void)
{
    return step_count;
}

// Software step detection task
static void step_detection_task(void *arg)
{
    qmi8658_data_t data;
    const float threshold = 1.2f; // Adjust threshold for step detection
    const uint32_t min_step_interval = 300; // Minimum ms between steps

    while (1) {
        if (bsp_qmi8658_read_data(&data)) {
            // Calculate acceleration magnitude in g
            float acc_x_g = data.acc_x * ACC_SENSITIVITY;
            float acc_y_g = data.acc_y * ACC_SENSITIVITY;
            float acc_z_g = data.acc_z * ACC_SENSITIVITY;
            float magnitude = sqrt(acc_x_g * acc_x_g + acc_y_g * acc_y_g + acc_z_g * acc_z_g);

            // Simple high-pass filter (remove gravity)
            float filtered = magnitude - 1.0f; // Assuming 1g gravity

            // Detect step: look for peak above threshold
            uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
            if (filtered > threshold && !step_detected && (now - last_step_time) > min_step_interval) {
                step_detected = true;
                step_count++;
                last_step_time = now;
                // ESP_LOGI(TAG, "Step detected! Total steps: %lu", step_count);  // Disabled to save battery
            } else if (filtered < threshold / 2) {
                step_detected = false;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50)); // 20 Hz sampling
    }
}

// Start step detection
void bsp_qmi8658_start_step_detection(void)
{
    xTaskCreate(step_detection_task, "step_detect", 4096, NULL, 1, NULL);
}

void bsp_qmi8658_init(i2c_master_bus_handle_t bus_handle)
{
    uint8_t id = 0;
    ESP_LOGI(TAG, "QMI8658 Initialize");
    
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = QMI8658_SENSOR_ADDR,
        .scl_speed_hz = 400000,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));
    
    ESP_ERROR_CHECK(bsp_qmi8658_reg_read(QMI8658_WHO_AM_I, &id, 1));
    
    if (0x05 != id)
    {
        ESP_LOGI(TAG, "QMI8658 not found");
    }
    ESP_LOGI(TAG, "Find QMI8658");
    bsp_qmi8658_reg_write_byte(QMI8658_RESET, (uint8_t[]){0xb0}, 1); // 
    vTaskDelay(pdMS_TO_TICKS(100));                                   // 
    bsp_qmi8658_reg_write_byte(QMI8658_CTRL1, (uint8_t[]){0x40}, 1); // CTRL1 
    bsp_qmi8658_reg_write_byte(QMI8658_CTRL7, (uint8_t[]){0x03}, 1); // CTRL7 
    bsp_qmi8658_reg_write_byte(QMI8658_CTRL2, (uint8_t[]){0x95}, 1); // CTRL2 
    bsp_qmi8658_reg_write_byte(QMI8658_CTRL3, (uint8_t[]){0xd5}, 1); // CTRL3 
    bsp_qmi8658_reg_write_byte(QMI8658_CTRL8, (uint8_t[]){0x01}, 1); // CTRL8 - Enable pedometer 
}

static void qmi8658_test_task(void *arg)
{
    qmi8658_data_t data;
    while (1)
    {
        bsp_qmi8658_read_data(&data);
        uint32_t hw_steps = bsp_qmi8658_read_steps();
        uint32_t sw_steps = bsp_qmi8658_get_software_steps();
        printf("Acc: %6d %6d %6d  -----  Gyr: %6d %6d %6d  -----  Angle: %6.2f %6.2f %6.2f  -----  HW Steps: %lu  SW Steps: %lu\n", data.acc_x, data.acc_y, data.acc_z, data.gyr_x, data.gyr_y, data.gyr_z, data.AngleX, data.AngleY, data.AngleZ, hw_steps, sw_steps);
        vTaskDelay(pdMS_TO_TICKS(1000)); // Changed to 1 second for testing
    }
}

void bsp_qmi8658_test(void)
{
    xTaskCreate(qmi8658_test_task, "qmi8658_test", 4096, NULL, 0, NULL);
}