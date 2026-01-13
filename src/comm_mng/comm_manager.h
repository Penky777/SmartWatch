#ifndef COMM_MANAGER_H
#define COMM_MANAGER_H

#include <stdint.h>

void comm_manager_on_rx(const char *msg);
void comm_manager_on_bt_ready(void);


// Initialize comm layer
void comm_manager_init(void);

// Send string or raw bytes to phone
void comm_send_str(const char *msg);
void comm_send_bytes(const uint8_t *data, uint16_t len);

// Phone → Watch callback
typedef void (*comm_rx_callback_t)(const char *msg);
void comm_set_rx_callback(comm_rx_callback_t cb);

#endif // COMM_MANAGER_H
