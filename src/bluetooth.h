#pragma once

#include <stdint.h>
#include <stdbool.h>

void bluetooth_init(void);
void bluetooth_enable(void);
void bluetooth_disable(void);
bool bluetooth_is_enabled(void);

// Send bytes to connected central(s) via notification. Data length limited
// by MTU (we cap to 240 bytes here).
void bluetooth_send_bytes(const uint8_t *data, uint16_t len);