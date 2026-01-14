#include "esp_log.h"
#include "esp_err.h"
#include "esp_random.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/ble_gap.h"
#include "host/ble_sm.h"
#include "host/ble_gatt.h"
#include "host/ble_l2cap.h"
#include "host/ble_store.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

#include "../comm_mng/comm_manager.h"
#include "../Bsp_qmi/bsp_qmi8658.h"
#include "../Ui_manager/ui_manager.h"
#include "gui.h"

static const char *TAG = "BLE_C6";

// Forward declarations
static void ble_app_on_sync(void);
static void ble_app_advertise(void);
static int ble_app_gap_event(struct ble_gap_event *event, void *arg);
void ble_host_task(void *param);
void bluetooth_send_bytes(const uint8_t *data, uint16_t len);
static void send_pin_now(void);

//  Bluetooth state 
static bool ble_enabled = false;
static bool ble_running = false;
static bool connected = false;
static uint16_t conn_handle = BLE_HS_CONN_HANDLE_NONE;
static int pairing_pin = 0;
static TimerHandle_t pairing_timer = NULL;
static TimerHandle_t pairing_timeout_timer = NULL;
static bool pairing_confirmed = false;
static bool mtu_negotiated = false;
static bool client_subscribed = false;  // Track if client subscribed to notifications
static volatile bool pairing_hide_requested = false;
static volatile uint32_t pairing_complete_time = 0;  // Track when pairing completed
static struct ble_gap_event_listener gap_event_listener;

//  GATT SERVICE
static const ble_uuid128_t gatt_svc_uuid =
    BLE_UUID128_INIT(0xef,0xcd,0xab,0x90,0x78,0x56,0x34,0x12,0xf0,0xde,0xbc,0x9a,0x78,0x56,0x34,0x12);

static const ble_uuid128_t gatt_chr_rx_uuid =
    BLE_UUID128_INIT(0x90,0x78,0x56,0x34,0x12,0xf0,0xde,0xbc,0x9a,0x78,0x56,0x34,0x12,0xef,0xcd,0xab);

// TX characteristic (Watch -> Phone) - notify/read
static const ble_uuid128_t gatt_chr_tx_uuid =
    BLE_UUID128_INIT(0x91,0x78,0x56,0x34,0x12,0xf0,0xde,0xbc,0x9a,0x78,0x56,0x34,0x12,0xef,0xcd,0xab);

// Buffer for TX characteristic value
#define BT_TX_MAX_LEN 240
static uint8_t bt_tx_value[BT_TX_MAX_LEN];
static int bt_tx_len = 0;
static uint16_t bt_tx_val_handle = 0;

// Timer for periodic test messages
static TimerHandle_t bt_test_timer = NULL;
static uint32_t bt_test_counter = 0;

// Timer for periodic PIN sending during pairing
static TimerHandle_t pin_send_timer = NULL;
static int pin_send_count = 0;  // Track how many times PIN has been sent

// Periodic PIN sending callback (every 1 second while pairing)
static void pin_send_timer_callback(TimerHandle_t xTimer)
{
    if (!ble_enabled || !connected || pairing_confirmed) {
        // Stop sending PIN once pairing is confirmed
        if (pin_send_timer != NULL) {
            xTimerStop(pin_send_timer, 0);
            pin_send_timer = NULL;
        }
        return;
    }
    
    // Only send PIN up to 1 time, then stop
    if (pairing_pin > 0 && client_subscribed && pin_send_count < 1) {
        send_pin_now();
        pin_send_count++;
        ESP_LOGI(TAG, "Periodic PIN send #%d", pin_send_count);
        
        // Stop timer after 1 sends
        if (pin_send_count >= 1) {
            ESP_LOGI(TAG, "Reached max PIN sends, stopping timer");
            if (pin_send_timer != NULL) {
                xTimerStop(pin_send_timer, 0);
                pin_send_timer = NULL;
            }
        }
    }
}

// Periodic test message callback (every 5 seconds)
static void bt_test_timer_callback(TimerHandle_t xTimer)
{
    if (!ble_enabled || !connected || !pairing_confirmed || bt_tx_val_handle == 0) return;
    
    bt_test_counter++;
    // Generate mock heart rate data (60-100 BPM)
    int mock_hr = 60 + (bt_test_counter % 41);  // Cycles through 60-100
    
    // Get current steps
    uint32_t steps = bsp_qmi8658_get_software_steps();
    
    // Demo SpO2 (98%)
    int spo2 = 98;
    
    char json_msg[128];
    snprintf(json_msg, sizeof(json_msg), "{\"heartRate\":%d,\"steps\":%lu,\"spo2\":%d}\n", mock_hr, steps, spo2);
    bluetooth_send_bytes((const uint8_t *)json_msg, strlen(json_msg));
    ESP_LOGI(TAG, "TX -> Phone: %s", json_msg);
}

static void pairing_timer_callback(TimerHandle_t xTimer)
{
    ESP_LOGW(TAG, "Pairing timeout - conn_handle: %d, connected: %d", conn_handle, connected);
    
    // Check for valid connection state
    if (conn_handle == BLE_HS_CONN_HANDLE_NONE) {
        ESP_LOGW(TAG, "Pairing timeout but conn_handle is NONE, just clean up");
        gui_lock();
        ui_hide_pairing();
        gui_unlock();
        return;
    }
    
    if (!connected) {
        ESP_LOGW(TAG, "Pairing timeout but not connected, just hide pairing screen");
        gui_lock();
        ui_hide_pairing();
        gui_unlock();
        return;
    }
    
    ESP_LOGW(TAG, "Pairing timeout, disconnecting");
    ble_gap_terminate(conn_handle, BLE_ERR_REM_USER_CONN_TERM);
    gui_lock();
    ui_hide_pairing();
    gui_unlock();
}

// Send PIN callback (2 seconds after connection)
static void send_pin_callback(TimerHandle_t xTimer)
{
    if (!connected) return;
    
    // Send PIN regardless of MTU - phone app should handle truncation if needed
    send_pin_now();
}

// Send PIN immediately (called when MTU is ready or after timeout)
static void send_pin_now(void)
{
    // Use short format that works with default MTU
    char pin_msg[16];
    snprintf(pin_msg, sizeof(pin_msg), "{\"pin\":%d}", pairing_pin);
    bluetooth_send_bytes((const uint8_t *)pin_msg, strlen(pin_msg));
    ESP_LOGI(TAG, "Sent PIN message: %s", pin_msg);
}

static int ble_rx_write_cb(uint16_t conn_handle, uint16_t attr_handle,
                           struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    uint16_t len = OS_MBUF_PKTLEN(ctxt->om);
    uint8_t buf[128];
    os_mbuf_copydata(ctxt->om, 0, len, buf);
    buf[len] = '\0';

    ESP_LOGI(TAG, "RX <- Phone: %s", buf);
    // Forward to comm manager (phone -> watch)
    comm_manager_on_rx((const char *)buf);

    return 0;
}

// Read callback for TX characteristic: central reads current value
static int ble_tx_read_cb(uint16_t conn_handle, uint16_t attr_handle,
                          struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    if (ctxt->op != BLE_GATT_ACCESS_OP_READ_CHR) return BLE_ATT_ERR_UNLIKELY;
    
    // If we have pending data in tx_value, return that
    if (bt_tx_len > 0) {
        int rc = os_mbuf_copyinto(ctxt->om, 0, bt_tx_value, bt_tx_len);
        return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }
    
    // Otherwise, if pairing is in progress, return the PIN
    if (connected && pairing_pin > 0 && !pairing_confirmed) {
        char pin_str[32];
        int len = snprintf(pin_str, sizeof(pin_str), "{\"pin\":%d}", pairing_pin);
        int rc = os_mbuf_copyinto(ctxt->om, 0, (uint8_t*)pin_str, len);
        ESP_LOGI("BLE_C6", "PIN read by client: %s", pin_str);
        return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }
    
    return 0;
}

static const struct ble_gatt_svc_def gatt_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &gatt_svc_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                /* RX: phone -> watch (write) */
                .uuid = &gatt_chr_rx_uuid.u,
                .access_cb = ble_rx_write_cb,
                .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP,
            },
            {
                /* TX: watch -> phone (read + notify) */
                .uuid = &gatt_chr_tx_uuid.u,
                .access_cb = ble_tx_read_cb,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                .val_handle = &bt_tx_val_handle,
            },
            {0}
        },
    },
    {0}
};

//  Host task 
void ble_host_task(void *param)
{
    nimble_port_run();
    nimble_port_freertos_deinit();
}

//  BLE sync callback
static void ble_app_on_sync(void)
{
    ble_svc_gap_device_name_set("SmartWatchC6");
    
    // Configure security
    ble_hs_cfg.sm_io_cap = BLE_HS_IO_DISPLAY_ONLY;
    ble_hs_cfg.sm_bonding = 1;
    ble_hs_cfg.sm_mitm = 1;
    ble_hs_cfg.sm_sc = 0;  // Disable SC for better compatibility with legacy devices
    ble_hs_cfg.sm_our_key_dist = BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;
    ble_hs_cfg.sm_their_key_dist = BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;
    
    ble_gap_event_listener_register(&gap_event_listener, ble_app_gap_event, NULL);
    ble_app_advertise();
}

// GAP event handler
static int ble_app_gap_event(struct ble_gap_event *event, void *arg)
{
    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            if (event->connect.status == 0) {
                connected = true;
                conn_handle = event->connect.conn_handle;
                mtu_negotiated = false;
                client_subscribed = false;  // Reset subscription state
                pin_send_count = 0;  // Reset PIN send counter
                ESP_LOGI(TAG, "Connected to device");

                // Generate random PIN (6 digits)
                pairing_pin = (esp_random() % 900000) + 100000;
                pairing_confirmed = false;

                // Show pairing screen on watch for user to verify PIN
                gui_lock();
                ui_show_pairing(pairing_pin);
                gui_unlock();

                // Start pairing timeout timer (30 seconds from now)
                if (pairing_timeout_timer == NULL) {
                    pairing_timeout_timer = xTimerCreate("pairing_timeout", pdMS_TO_TICKS(30000), pdFALSE, NULL, pairing_timer_callback);
                }
                if (pairing_timeout_timer != NULL) {
                    xTimerStart(pairing_timeout_timer, 0);
                }
                
                // Start periodic PIN sending timer (send PIN every 1 second until subscription or pairing confirmed)
                // This ensures the app gets the PIN even if there's a timing issue with subscription
                if (pin_send_timer == NULL) {
                    pin_send_timer = xTimerCreate("pin_send", pdMS_TO_TICKS(1000), pdTRUE, NULL, pin_send_timer_callback);
                }
                if (pin_send_timer != NULL) {
                    xTimerStart(pin_send_timer, 0);
                }
            } else {
                ESP_LOGE(TAG, "Connection failed: %d", event->connect.status);
            }
            break;

        case BLE_GAP_EVENT_MTU:
            if (event->mtu.conn_handle == conn_handle && event->mtu.channel_id == BLE_L2CAP_CID_ATT) {
                mtu_negotiated = true;
                ESP_LOGI(TAG, "MTU negotiated: %d bytes", event->mtu.value);
                // Try sending PIN immediately after MTU negotiation
                // This helps if client subscribed before MTU completed
                if (connected && pairing_pin > 0 && !pairing_confirmed && client_subscribed) {
                    vTaskDelay(pdMS_TO_TICKS(100));  // Small delay to ensure stack is ready
                    send_pin_now();
                }
            }
            break;

        case BLE_GAP_EVENT_DISCONNECT:
            connected = false;
            conn_handle = BLE_HS_CONN_HANDLE_NONE;
            client_subscribed = false;
            ESP_LOGI(TAG, "Disconnected");
            gui_lock();
            ui_hide_pairing();
            gui_unlock();
            // Clean up timers
            if (pairing_timer != NULL) {
                xTimerStop(pairing_timer, 0);
                pairing_timer = NULL;
            }
            if (pairing_timeout_timer != NULL) {
                xTimerStop(pairing_timeout_timer, 0);
                pairing_timeout_timer = NULL;
            }
            if (pin_send_timer != NULL) {
                xTimerStop(pin_send_timer, 0);
                pin_send_timer = NULL;
            }
            break;

        case BLE_GAP_EVENT_PASSKEY_ACTION:
            ESP_LOGI(TAG, "Passkey action requested, action=%d", event->passkey.params.action);
            // Provide our PIN as the passkey
            struct ble_sm_io pkey = {0};
            pkey.action = event->passkey.params.action;
            if (pkey.action == BLE_SM_IOACT_DISP) {
                pkey.passkey = pairing_pin;
                ESP_LOGI(TAG, "Injecting passkey: %d", pairing_pin);
                ble_sm_inject_io(conn_handle, &pkey);
            } else {
                ESP_LOGW(TAG, "Unexpected passkey action: %d", pkey.action);
            }
            break;

        case BLE_GAP_EVENT_ENC_CHANGE:
            ESP_LOGI(TAG, "Encryption change event: status=%d, conn_handle=%d", event->enc_change.status, event->enc_change.conn_handle);
            if (event->enc_change.status == 0) {
                // Pairing/encryption successful
                ESP_LOGI(TAG, "Pairing completed successfully!");
                pairing_confirmed = true;
                pairing_complete_time = lv_tick_get();
                
                // Stop periodic PIN sending
                if (pin_send_timer != NULL) {
                    xTimerStop(pin_send_timer, 0);
                    pin_send_timer = NULL;
                }
                
                // Clean up timeout timer
                if (pairing_timeout_timer != NULL) {
                    xTimerStop(pairing_timeout_timer, 0);
                    pairing_timeout_timer = NULL;
                }
                
                // Send acknowledgment to phone
                bluetooth_send_bytes((const uint8_t *)"{\"status\":\"paired\"}", strlen("{\"status\":\"paired\"}"));
                ESP_LOGI(TAG, "Sent pairing acknowledgment to phone");
                
                // Don't hide immediately - let user see the PIN for a moment
                // The poll() function will handle delayed hiding
                ESP_LOGI(TAG, "Pairing complete, will hide screen after delay");
            } else {
                ESP_LOGE(TAG, "Pairing failed with status: %d", event->enc_change.status);
            }
            break;

        case BLE_GAP_EVENT_SUBSCRIBE:
            ESP_LOGI(TAG, "Subscribe event: conn_handle=%d, attr_handle=%d, reason=%d, prev=%d, cur=%d",
                     event->subscribe.conn_handle,
                     event->subscribe.attr_handle,
                     event->subscribe.reason,
                     event->subscribe.prev_notify,
                     event->subscribe.cur_notify);
            if (event->subscribe.cur_notify) {
                ESP_LOGI(TAG, "Client subscribed to notifications");
                client_subscribed = true;
                
                // Now that client is subscribed, send the PIN immediately
                if (connected && pairing_pin > 0 && !pairing_confirmed) {
                    // Give a tiny delay to ensure subscription is fully set up
                    vTaskDelay(pdMS_TO_TICKS(50));
                    send_pin_now();
                    pin_send_count++;
                    ESP_LOGI(TAG, "Sent initial PIN after subscription (count=%d)", pin_send_count);
                    // Don't stop timer - let it send a few more times to ensure app receives it
                }
            } else {
                ESP_LOGI(TAG, "Client unsubscribed from notifications");
                client_subscribed = false;
            }
            break;

        default:
            ESP_LOGI(TAG, "GAP event: %d", event->type);
            break;
    }
    return 0;
}

static void ble_app_advertise(void)
{
    struct ble_hs_adv_fields fields = {0};
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.name = (uint8_t *)"SmartWatchC6";
    fields.name_len = strlen("SmartWatchC6");
    fields.name_is_complete = 1;

    ble_gap_adv_set_fields(&fields);

    struct ble_gap_adv_params params = {0};
    params.conn_mode = BLE_GAP_CONN_MODE_UND;
    params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    int rc = ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL,
                               BLE_HS_FOREVER, &params, NULL, NULL);

    if (rc == 0) {
        ESP_LOGI(TAG, "Advertising started");
    } else {
        ESP_LOGE(TAG, "Advertising start error: %d", rc);
    }
}

//  PUBLIC API: ENABLE 
void bluetooth_enable(void)
{
    if (ble_enabled) return;
    ble_enabled = true;

    ESP_LOGI(TAG, "Bluetooth ENABLE");

    nimble_port_init();
    ble_hs_cfg.sync_cb = ble_app_on_sync;

    ble_svc_gap_init();
    ble_svc_gatt_init();

    ble_gatts_count_cfg(gatt_svcs);
    ble_gatts_add_svcs(gatt_svcs);

    if (!ble_running) {
        nimble_port_freertos_init(ble_host_task);
        ble_running = true;
    }

    // Start periodic test timer (5000 ms = 5 seconds)
    if (bt_test_timer == NULL) {
        bt_test_timer = xTimerCreate("bt_test", pdMS_TO_TICKS(5000), pdTRUE, NULL, bt_test_timer_callback);
    }
    if (bt_test_timer != NULL) {
        xTimerStart(bt_test_timer, 0);
    }
}

//  PUBLIC API: DISABLE 
void bluetooth_disable(void)
{
    if (!ble_enabled) return;
    ble_enabled = false;

    ESP_LOGW(TAG, "Bluetooth DISABLE");

    // Stop advertising
    ble_gap_adv_stop();

    if (ble_running) {
        nimble_port_stop();
        vTaskDelay(50 / portTICK_PERIOD_MS);
        nimble_port_deinit();
        ble_running = false;
    }

    // Stop periodic test timer
    if (bt_test_timer != NULL) {
        xTimerStop(bt_test_timer, 0);
    }
}

// PUBLIC API 
bool bluetooth_is_enabled(void)
{
    return ble_enabled;
}

// Confirm pairing
void bluetooth_confirm_pairing(void)
{
    ESP_LOGI(TAG, "Confirming pairing - conn_handle: %d, connected: %d", conn_handle, connected);
    
    // Only proceed if we're still connected
    if (!connected || conn_handle == BLE_HS_CONN_HANDLE_NONE) {
        ESP_LOGW(TAG, "Cannot confirm pairing - not connected");
        return;
    }
    
    if (pairing_timer != NULL) {
        xTimerStop(pairing_timer, 0);
        pairing_timer = NULL;
    }
    if (pairing_timeout_timer != NULL) {
        xTimerStop(pairing_timeout_timer, 0);
        pairing_timeout_timer = NULL;
    }
    pairing_confirmed = true;
    
    // With BLE_HS_IO_DISPLAY_ONLY, the BLE stack handles pairing automatically
    // when the central confirms. No need to call ble_sm_inject_io.
    // Send acknowledgment back to phone
    bluetooth_send_bytes((const uint8_t *)"{\"status\":\"paired\"}", strlen("{\"status\":\"paired\"}"));
    ESP_LOGI(TAG, "Sent pairing acknowledgment to phone");
    
    // Set flag to hide UI - will be checked by LVGL task poll.
    pairing_hide_requested = true;

    ESP_LOGI(TAG, "Pairing confirmed successfully");
}

// Send bytes to connected centrals via GATT notification.
// Copies up to BT_TX_MAX_LEN bytes into the characteristic value and
// triggers a chr_updated which will send notifications to subscribed clients.
void bluetooth_send_bytes(const uint8_t *data, uint16_t len)
{
    if (!ble_enabled) {
        ESP_LOGW(TAG, "Cannot send: BLE not enabled");
        return;
    }
    if (bt_tx_val_handle == 0) {
        ESP_LOGW(TAG, "Cannot send: TX handle not set");
        return;
    }
    if (!connected) {
        ESP_LOGW(TAG, "Cannot send: not connected");
        return;
    }

    if (len > BT_TX_MAX_LEN) len = BT_TX_MAX_LEN;
    memcpy(bt_tx_value, data, len);
    bt_tx_len = len;

    // Notify subscribed centrals. Use the value handle.
    ble_gatts_chr_updated(bt_tx_val_handle);
    ESP_LOGI(TAG, "Notification sent (%d bytes)", len);
}

//  DO NOT ENABLE BY DEFAULT 
void bluetooth_init(void)
{
    // Start OFF
    ble_enabled = false;
}

// Poll for UI updates - call this from LVGL task
void bluetooth_poll(void)
{
    if (pairing_hide_requested) {
        ESP_LOGI(TAG, "Poll: pairing_hide_requested=true, calling ui_hide_pairing()");
        pairing_hide_requested = false;
        // Don't hold GUI lock - ui_hide_pairing needs to do heavy LVGL operations
        ui_hide_pairing();
        ESP_LOGI(TAG, "Poll: ui_hide_pairing() returned");
    }
    
    // Fallback: if pairing was confirmed but not hidden yet, hide after 3 seconds
    if (pairing_confirmed && connected && pairing_complete_time > 0) {
        uint32_t now = lv_tick_get();
        if (now - pairing_complete_time > 3000) {
            ESP_LOGI(TAG, "Pairing hide fallback triggered after 3 seconds");
            pairing_complete_time = 0;  // Clear to avoid repeated calls
            // Don't hold GUI lock - ui_hide_pairing needs to do heavy LVGL operations
            ui_hide_pairing();
        }
    }
}
