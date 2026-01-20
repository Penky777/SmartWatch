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
// Helper function: Extract JSON string field value, handling UTF-8 and escaped quotes
// Replaces emoji and non-ASCII characters with '?' for safety
static bool extract_json_string(const char *msg, const char *field, char *output, int max_len)
{
    if (!msg || !field || !output || max_len <= 1) {
        return false;
    }
    
    output[0] = '\0';
    
    // Build search string: "fieldname":"
    char search_buf[64];
    int search_len = snprintf(search_buf, sizeof(search_buf), "\"%s\":\"", field);
    if (search_len <= 0 || search_len >= sizeof(search_buf)) {
        return false;
    }
    
    const char *field_start = strstr(msg, search_buf);
    if (!field_start) {
        return false;
    }
    
    // Move past the field prefix
    const char *value_start = field_start + search_len;
    
    // Find the closing quote, handling escaped quotes
    int copied = 0;
    const char *pos = value_start;
    
    while (*pos != '\0' && copied < max_len - 1) {
        if (*pos == '\\' && *(pos + 1) != '\0') {
            // Handle escape sequences
            char escaped = *(pos + 1);
            if (escaped == '"') {
                output[copied++] = '"';
                pos += 2;
            } else if (escaped == '\\') {
                output[copied++] = '\\';
                pos += 2;
            } else if (escaped == 'n') {
                output[copied++] = '\n';
                pos += 2;
            } else if (escaped == 'r') {
                output[copied++] = '\r';
                pos += 2;
            } else if (escaped == 't') {
                output[copied++] = '\t';
                pos += 2;
            } else {
                // Unknown escape, just copy the character
                output[copied++] = *pos;
                pos++;
            }
        } else if (*pos == '"') {
            // Found unescaped closing quote
            output[copied] = '\0';
            return true;
        } else if ((unsigned char)*pos < 32 || (unsigned char)*pos >= 127) {
            // Replace emoji and non-ASCII characters with '?'
            // ASCII control chars (0-31) and non-ASCII (128+) get replaced
            output[copied++] = '?';
            // Skip multi-byte UTF-8 sequences (they have high bit set)
            if ((unsigned char)*pos >= 128) {
                pos++;
                // Skip continuation bytes (10xxxxxx pattern)
                while ((unsigned char)*pos >= 128 && (unsigned char)*pos < 192 && copied < max_len - 1) {
                    pos++;
                }
                continue;
            }
            pos++;
        } else {
            // Regular ASCII character
            output[copied++] = *pos;
            pos++;
        }
    }
    
    output[copied] = '\0';
    return copied > 0;
}

// Parse and handle notification JSON
static void handle_notification(const char *msg)
{
    ESP_LOGI("COMM", "→ Parsing notification JSON...");
    
    // Match buffer sizes with alerts_screen.c definitions
    char title[64] = {0};    // MAX_TITLE_LEN from alerts_screen.c
    char text[128] = {0};    // MAX_TEXT_LEN from alerts_screen.c
    char app[32] = {0};      // MAX_APP_LEN from alerts_screen.c
    
    bool has_title = extract_json_string(msg, "title", title, sizeof(title));
    bool has_text = extract_json_string(msg, "text", text, sizeof(text));
    bool has_app = extract_json_string(msg, "app", app, sizeof(app));
    
    if (has_title) {
        ESP_LOGI("COMM", "  ✓ Title: %s", title);
    } else {
        ESP_LOGW("COMM", "  ✗ No title field found");
    }
    
    if (has_text) {
        ESP_LOGI("COMM", "  ✓ Text: %s", text);
    } else {
        ESP_LOGW("COMM", "  ✗ No text field found");
    }
    
    if (has_app) {
        ESP_LOGI("COMM", "  ✓ App: %s", app);
    } else {
        ESP_LOGI("COMM", "  ℹ No app field (optional)");
    }
    
    // Validate we got at least title or text
    if (!has_title && !has_text) {
        ESP_LOGE("COMM", "  ✗ Invalid notification: missing both title and text!");
        return;
    }
    
    // Store notification (UI update will happen in main task)
    ESP_LOGI("COMM", "→ Storing notification in alerts system...");
    alerts_add_notification(title, text, app);
    ESP_LOGI("COMM", "✓ Notification stored successfully (total count=%d)", alerts_get_count());
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
    
    // Handle notifications - with detailed debug logging
    if (strstr(msg, "\"type\":\"notification\"")) {
        ESP_LOGI("COMM", "✓ Notification request received - triggering handler");
        handle_notification(msg);
        ESP_LOGI("COMM", "✓ Notification handler completed");
        return;
    }
    
    // If we get here, message type wasn't recognized
    ESP_LOGW("COMM", "⚠ Unknown message type received (len=%d)", msg_len);
    
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

