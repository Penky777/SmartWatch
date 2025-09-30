#include "cst816t.h"
#include "esp_log.h"
#include "esp_check.h"

static const char *TAG = "cst816t";

static esp_err_t probe_addr(i2c_master_bus_handle_t bus, uint8_t addr)
{
    return i2c_master_probe(bus, addr, 50); // 50 ms
}

esp_err_t cst816t_init(cst816t_t *dev,
                       i2c_master_bus_handle_t bus,
                       gpio_num_t int_gpio)
{
    ESP_RETURN_ON_FALSE(dev && bus, ESP_ERR_INVALID_ARG, TAG, "bad args");
    dev->bus = bus;
    dev->int_gpio = int_gpio;

    // Configure INT (optional but helpful). Active low, open-drain on many boards.
    if (int_gpio != GPIO_NUM_NC) {
        gpio_config_t io = {
            .pin_bit_mask = 1ULL << int_gpio,
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = 1,   // ensure it idles high
            .pull_down_en = 0,
            .intr_type = GPIO_INTR_DISABLE,
        };
        ESP_RETURN_ON_ERROR(gpio_config(&io), TAG, "gpio_config INT failed");
    }

    // Make sure the I2C bus has some timing config attached (IDF v5 quirk)
    // If the app hasn’t added any device yet, add a dummy at 100kHz.
    i2c_master_dev_handle_t dummy = NULL;
    i2c_device_config_t tcfg = { .device_address = 0x7F, .scl_speed_hz = 100000 };
    esp_err_t maybe = i2c_master_bus_add_device(bus, &tcfg, &dummy);
    if (maybe != ESP_OK && maybe != ESP_ERR_INVALID_STATE) {
        // INVALID_STATE just means a timing device is already present — that’s fine.
        ESP_LOGW(TAG, "bus timing device add: %s", esp_err_to_name(maybe));
    }

    // Detect address: most CST816T use 0x15; a few use 0x2A
    uint8_t candidates[] = { 0x15, 0x2A };
    esp_err_t ok = ESP_FAIL;
    for (size_t i = 0; i < sizeof(candidates); i++) {
        if (probe_addr(bus, candidates[i]) == ESP_OK) {
            dev->i2c_addr = candidates[i];
            ok = ESP_OK;
            ESP_LOGI(TAG, "CST816T detected at 0x%02X", dev->i2c_addr);
            break;
        }
    }
    ESP_RETURN_ON_ERROR(ok, TAG, "CST816T not found (try pull-ups, INT pull-up, power cycle)");

    return ESP_OK;
}

// Read n bytes from register 'reg' into buf
static esp_err_t rd(i2c_master_bus_handle_t bus, uint8_t addr, uint8_t reg, uint8_t *buf, size_t n)
{
    i2c_master_dev_handle_t dev = NULL;
    i2c_device_config_t cfg = { .device_address = addr, .scl_speed_hz = 100000 };
    ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(bus, &cfg, &dev), TAG, "add_device");
    esp_err_t e = i2c_master_transmit_receive(dev, &reg, 1, buf, n, 50);
    esp_err_t e2 = i2c_master_bus_rm_device(dev);
    if (e2 != ESP_OK) ESP_LOGW(TAG, "rm_device: %s", esp_err_to_name(e2));
    return e;
}

esp_err_t cst816t_read_point(cst816t_t *dev,
                             bool *has_touch,
                             uint16_t *out_x,
                             uint16_t *out_y)
{
    ESP_RETURN_ON_FALSE(dev && has_touch && out_x && out_y, ESP_ERR_INVALID_ARG, TAG, "bad args");

    // Optional: if INT is wired and high, there might be no touch; still read to be safe.
    if (dev->int_gpio != GPIO_NUM_NC) {
        int lvl = gpio_get_level(dev->int_gpio); // active low
        // If line is high, it's usually "no touch"; we'll still proceed to read once.
        (void)lvl;
    }

    uint8_t buf[7] = {0}; // 0x01..0x07
    esp_err_t e = rd(dev->bus, dev->i2c_addr, 0x01, buf, sizeof(buf));
    if (e != ESP_OK) {
        *has_touch = false;
        return e;
    }

    uint8_t points = buf[1]; // 0x02
    if (points == 0) {
        *has_touch = false;
        return ESP_OK;
    }

    // buf[2] = XH, buf[3] = XL, buf[4] = YH, buf[5] = YL
    uint16_t x = ((buf[2] & 0x0F) << 8) | buf[3];
    uint16_t y = ((buf[4] & 0x0F) << 8) | buf[5];

    *has_touch = true;
    *out_x = x;
    *out_y = y;
    return ESP_OK;
}
