#include "comm_manager.h"
#include "../Bluetooth/bluetooth.h"
#include "esp_log.h"
#include <string.h>
#include <time.h>
#include "../Ui_manager/ui_manager.h"
#include "../lib/PCF85063/bsp_pcf85063.h"
#include "../alerts_screen.h"

// Forward declaration
extern int alerts_get_count(void);

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

// Parse and handle notification JSON
static void handle_notification(const char *msg)
{
    ESP_LOGI("COMM", "Notification received");
    
    char title[101] = {0};
    char text[151] = {0};
    char app[64] = {0};
    
    // Extract title if present (safely)
    const char *title_start = strstr(msg, "\"title\":\"");
    if (title_start) {
        title_start += 9; // skip "title":"
        const char *title_end = strchr(title_start, '\"');
        if (title_end && (title_end - title_start) < 100) {
            int len = title_end - title_start;
            if (len > 100) len = 100;
            memcpy(title, title_start, len);
            title[len] = '\0';
            ESP_LOGI("COMM", "Notification title: %s", title);
        }
    }
    
    // Extract text if present (safely)
    const char *text_start = strstr(msg, "\"text\":\"");
    if (text_start) {
        text_start += 8; // skip "text":"
        const char *text_end = strchr(text_start, '\"');
        if (text_end && (text_end - text_start) < 150) {
            int len = text_end - text_start;
            if (len > 150) len = 150;
            memcpy(text, text_start, len);
            text[len] = '\0';
            ESP_LOGI("COMM", "Notification text: %s", text);
        }
    }
    
    // Extract app package name if present
    const char *app_start = strstr(msg, "\"app\":\"");
    if (app_start) {
        app_start += 7; // skip "app":"
        const char *app_end = strchr(app_start, '\"');
        if (app_end && (app_end - app_start) < 63) {
            int len = app_end - app_start;
            if (len > 63) len = 63;
            memcpy(app, app_start, len);
            app[len] = '\0';
            ESP_LOGI("COMM", "Notification app: %s", app);
        }
    }
    
    // Store notification (UI update will happen in main task)
    alerts_add_notification(title, text, app);
    ESP_LOGI("COMM", "Notification stored (count=%d)", alerts_get_count());
}

// Parse and handle time sync JSON: {"type":"sync","ts":1768301013,"tzMin":60}
static void handle_time_sync(const char *msg)
{
    // Find the "ts" field
    const char *ts_ptr = strstr(msg, "\"ts\":");
    if (!ts_ptr) {
        ESP_LOGW("COMM", "Time sync: 'ts' field not found");
        return;
    }
    
    // Parse timestamp
    long unsigned int unix_ts = 0;
    if (sscanf(ts_ptr, "\"ts\":%lu", &unix_ts) != 1) {
        ESP_LOGW("COMM", "Time sync: failed to parse timestamp");
        return;
    }
    
    // Find the "tzMin" field for timezone offset
    long int tz_min = 0;
    const char *tz_ptr = strstr(msg, "\"tzMin\":");
    if (tz_ptr) {
        sscanf(tz_ptr, "\"tzMin\":%ld", &tz_min);
    }
    
    // Convert UNIX timestamp to struct tm
    // UNIX timestamp is UTC, so we convert it
    time_t ts = (time_t)unix_ts;
    struct tm *gmt = gmtime(&ts);
    
    if (!gmt) {
        ESP_LOGE("COMM", "Time sync: gmtime failed");
        return;
    }
    
    // Apply timezone offset (in minutes)
    struct tm local_tm = *gmt;
    long int offset_secs = tz_min * 60;
    
    // Adjust the time by timezone
    time_t ts_local = ts + offset_secs;
    struct tm *tm_adjusted = gmtime(&ts_local);
    if (tm_adjusted) {
        local_tm = *tm_adjusted;
    }
    
    // Set the RTC with the new time
    bsp_pcf85063_set_time(&local_tm);
    
    ESP_LOGI("COMM", "Time sync: set to %04d-%02d-%02d %02d:%02d:%02d (UTC%+ld:%02ld)",
             local_tm.tm_year + 1900, local_tm.tm_mon + 1, local_tm.tm_mday,
             local_tm.tm_hour, local_tm.tm_min, local_tm.tm_sec,
             tz_min / 60, labs(tz_min) % 60);
}

void comm_manager_on_rx(const char *msg)
{
    // Don't log the full message if it's too long (emojis can cause issues)
    int msg_len = strlen(msg);
    if (msg_len > 100) {
        ESP_LOGI("COMM", "Phone -> Watch: [%d bytes] %.80s...", msg_len, msg);
    } else {
        ESP_LOGI("COMM", "Phone -> Watch: %s", msg);
    }
    
    // Handle pairing confirmation
    if (strcmp(msg, "confirm") == 0) {
        ESP_LOGI("COMM", "Pairing confirmed");
        bluetooth_confirm_pairing();
        return;
    }
    
    // Handle time sync
    if (strstr(msg, "\"type\":\"sync\"")) {
        ESP_LOGI("COMM", "Time sync request received");
        handle_time_sync(msg);
        return;
    }
    
    // Handle notifications
    if (strstr(msg, "\"type\":\"notification\"")) {
        ESP_LOGI("COMM", "Notification request received");
        handle_notification(msg);
        return;
    }
    
    // Pass to app callback if registered (but be careful!)
    if (app_rx_cb) {
        // Only pass if message is reasonable size
        if (msg_len < 256) {
            app_rx_cb(msg);
        } else {
            ESP_LOGW("COMM", "Message too long for app callback, ignoring");
        }
    }
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

