#include "comm_manager.h"
#include "bluetooth.h"
#include "esp_log.h"
#include <string.h>

// app-provided callback
static comm_rx_callback_t app_rx_cb = NULL;

// internal forward declaration
void internal_bt_rx_handler(const unsigned char *data, uint16_t len) {
    // Example: convert to string if needed
    char buffer[256];
    uint16_t copy_len = len < sizeof(buffer)-1 ? len : sizeof(buffer)-1;
    memcpy(buffer, data, copy_len);
    buffer[copy_len] = '\0';

    

    // Or handle raw data directly
}

void comm_manager_on_rx(const char *msg)
{
    ESP_LOGI("COMM", "Phone -> Watch: %s", msg);
    if (app_rx_cb) app_rx_cb(msg);
}

void comm_manager_on_bt_ready(void)
{
    ESP_LOGI("COMM", "Bluetooth is ready");
    // Optional: notify UI or send initial data
}

// Initialize comm layer (optional for now)
void comm_manager_init(void)
{
    // Nothing special yet; keep for future expansion
}

// Send a string to phone
void comm_send_str(const char *msg)
{
    if (!msg) return;
    comm_send_bytes((const uint8_t *)msg, (uint16_t)strlen(msg));
}

// Send raw bytes to phone
void comm_send_bytes(const uint8_t *data, uint16_t len)
{
    if (!data || len == 0) return;
    bluetooth_send_bytes(data, len);
}

// App sets callback
void comm_set_rx_callback(comm_rx_callback_t cb)
{
    app_rx_cb = cb;
}

