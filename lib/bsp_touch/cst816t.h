#ifndef CST816T_H
#define CST816T_H

#include <stdbool.h>
#include <stdint.h>
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    i2c_master_bus_handle_t bus;
    uint8_t                 i2c_addr;     // usually 0x15; we’ll auto-detect
    gpio_num_t              int_gpio;     // GPIO_NUM_NC if not used
} cst816t_t;

/**
 * Initialize CST816T:
 *  - ensures a device exists at 0x15 (or 0x2A fallback)
 *  - sets INT pin (optional) as input pull-up
 *  - sets a “timing device” to 100 kHz on the bus if caller didn’t already
 */
esp_err_t cst816t_init(cst816t_t *dev,
                       i2c_master_bus_handle_t bus,
                       gpio_num_t int_gpio);

/**
 * Poll one touch point (CST816T supports single touch).
 * Returns ESP_OK and sets out_x/out_y if a touch is present.
 * If no touch, returns ESP_OK with *has_touch == false.
 *
 * Notes (register map used):
 *   0x01: Gesture ID (ignored here)
 *   0x02: Points (0 or 1)
 *   0x03: X High  (bits11..4) + event flags (ignored)
 *   0x04: X Low   (bits3..0)
 *   0x05: Y High
 *   0x06: Y Low
 */
esp_err_t cst816t_read_point(cst816t_t *dev,
                             bool *has_touch,
                             uint16_t *out_x,
                             uint16_t *out_y);

#ifdef __cplusplus
}
#endif
#endif // CST816T_H
