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
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "../comm_mng/comm_manager.h"
#include "../Bsp_qmi/bsp_qmi8658.h"
#include "../Ui_manager/ui_manager.h"
#include "../Max30102/max30102.h"
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
static SemaphoreHandle_t ble_stop_sem = NULL;  // Signals when NimBLE host stops
static TaskHandle_t ble_shutdown_task = NULL;

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
    // Send PIN once when timer fires
    if (connected && pairing_pin > 0 && !pairing_confirmed && client_subscribed) {
        send_pin_now();
        pin_send_count++;
        ESP_LOGI(TAG, "Sent PIN after delay (count=%d)", pin_send_count);
    }
    
    // Stop timer after first send
    if (pin_send_timer != NULL) {
        xTimerStop(pin_send_timer, 0);
        pin_send_timer = NULL;
    }
}

// Periodic test message callback (every 5 seconds)
static void bt_test_timer_callback(TimerHandle_t xTimer)
{
    if (!ble_enabled || !connected || !pairing_confirmed || bt_tx_val_handle == 0) return;
    
    bt_test_counter++;
    
    // Get actual heart rate and SpO2 from MAX30102 sensor
    uint8_t spo2 = 0, heart_rate = 0;
    esp_err_t ret = max_read(&spo2, &heart_rate);
    
    // If sensor read fails, use 0 (invalid) instead of mock data
    if (ret != ESP_OK) {
        heart_rate = 0;
        spo2 = 0;
    }
    
    // Get current steps
    uint32_t steps = bsp_qmi8658_get_software_steps();
    
    char json_msg[128];
    snprintf(json_msg, sizeof(json_msg), "{\"heartRate\":%u,\"steps\":%lu,\"spo2\":%u}\n", heart_rate, steps, spo2);
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
    
    // Allocate buffer large enough for notifications (max 512 bytes)
    #define BLE_RX_MAX_SIZE 512
    static uint8_t buf[BLE_RX_MAX_SIZE];
    
    // Clamp to buffer size to prevent overflow
    if (len >= BLE_RX_MAX_SIZE) {
        ESP_LOGW(TAG, "RX message too large (%d bytes), truncating to %d", len, BLE_RX_MAX_SIZE - 1);
        len = BLE_RX_MAX_SIZE - 1;
    }
    
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

    // Notify any waiter that the host task exited
    if (ble_stop_sem != NULL) {
        xSemaphoreGive(ble_stop_sem);
    }
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

                // Check if we already have bonding info for this device
                struct ble_sm_io pkey = {0};
                struct ble_gap_conn_desc desc;
                if (ble_gap_conn_find(conn_handle, &desc) == 0) {
                    ESP_LOGI(TAG, "Connection from: %02x:%02x:%02x:%02x:%02x:%02x (type=%d)",
                             desc.peer_id_addr.val[5], desc.peer_id_addr.val[4],
                             desc.peer_id_addr.val[3], desc.peer_id_addr.val[2],
                             desc.peer_id_addr.val[1], desc.peer_id_addr.val[0],
                             desc.peer_id_addr.type);
                }

                // DON'T generate PIN or show UI here - wait for BLE_GAP_EVENT_PASSKEY_ACTION
                // If device is bonded, that event won't fire and connection proceeds silently
                pairing_pin = 0;
                pairing_confirmed = false;

                // Timer will be started only if BLE_GAP_EVENT_PASSKEY_ACTION fires (new pairing)
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
                    ESP_LOGI(TAG, "Sent PIN after MTU negotiation");
                }
            }
            break;

        case BLE_GAP_EVENT_DISCONNECT:
            connected = false;
            conn_handle = BLE_HS_CONN_HANDLE_NONE;
            client_subscribed = false;
            ESP_LOGI(TAG, "Disconnected");
            
            // Only hide pairing screen if we were actually showing it
            if (pairing_pin > 0 && !pairing_confirmed) {
                gui_lock();
                ui_hide_pairing();
                gui_unlock();
            }
            
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
            
            // Restart advertising so phone can reconnect
            ble_app_advertise();
            ESP_LOGI(TAG, "Advertising restarted after disconnect");
            break;

        case BLE_GAP_EVENT_PASSKEY_ACTION:
            ESP_LOGI(TAG, "Passkey action requested, action=%d", event->passkey.params.action);
            
            // Only generate PIN if we haven't already (first passkey request)
            if (pairing_pin == 0) {
                pairing_pin = (esp_random() % 900000) + 100000;
                ESP_LOGI(TAG, "Generated new pairing PIN: %d", pairing_pin);
                
                // Show pairing screen on watch
                gui_lock();
                ui_show_pairing(pairing_pin);
                gui_unlock();
                
                // Start pairing timeout timer (30 seconds) - only for new pairing
                if (pairing_timeout_timer == NULL) {
                    pairing_timeout_timer = xTimerCreate("pairing_timeout", pdMS_TO_TICKS(30000), pdFALSE, NULL, pairing_timer_callback);
                }
                if (pairing_timeout_timer != NULL) {
                    xTimerStart(pairing_timeout_timer, 0);
                    ESP_LOGI(TAG, "Pairing timeout started (30 seconds)");
                }
            }
            
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
                
                // Start timer to send PIN after 500ms delay (non-blocking)
                if (connected && pairing_pin > 0 && !pairing_confirmed && pin_send_count == 0) {
                    if (pin_send_timer == NULL) {
                        pin_send_timer = xTimerCreate("pin_send", pdMS_TO_TICKS(500), pdFALSE, NULL, pin_send_timer_callback);
                    }
                    if (pin_send_timer != NULL) {
                        xTimerStart(pin_send_timer, 0);
                        ESP_LOGI(TAG, "Started PIN send timer (500ms)");
                    }
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

    // Only initialize the NimBLE host once; on subsequent enables just re-advertise
    if (!ble_running) {
        if (ble_stop_sem == NULL) {
            ble_stop_sem = xSemaphoreCreateBinary();
        }

        nimble_port_init();
        ble_hs_cfg.sync_cb = ble_app_on_sync;

        ble_svc_gap_init();
        ble_svc_gatt_init();

        ble_gatts_count_cfg(gatt_svcs);
        ble_gatts_add_svcs(gatt_svcs);

        nimble_port_freertos_init(ble_host_task);
        ble_running = true;
    } else {
        // If host is already running (we never deinit), restart advertising
        ble_app_advertise();
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
static void ble_shutdown_task_fn(void *param)
{
    (void)param;
    ESP_LOGW(TAG, "Bluetooth DISABLE (async)");

    // Disconnect first so all GAP events settle before tearing NimBLE down
    if (connected && conn_handle != BLE_HS_CONN_HANDLE_NONE) {
        ble_gap_terminate(conn_handle, BLE_ERR_REM_USER_CONN_TERM);
        for (int i = 0; i < 15 && connected; i++) {
            vTaskDelay(pdMS_TO_TICKS(20));
        }
    }

    // Stop advertising
    int rc_adv = ble_gap_adv_stop();
    if (rc_adv != 0) {
        ESP_LOGW(TAG, "ble_gap_adv_stop rc=%d", rc_adv);
    }

    // Stop and delete timers to avoid callbacks hitting a torn-down stack
    if (bt_test_timer != NULL) {
        xTimerStop(bt_test_timer, 0);
        xTimerDelete(bt_test_timer, 0);
        bt_test_timer = NULL;
    }
    if (pairing_timer != NULL) {
        xTimerStop(pairing_timer, 0);
        xTimerDelete(pairing_timer, 0);
        pairing_timer = NULL;
    }
    if (pairing_timeout_timer != NULL) {
        xTimerStop(pairing_timeout_timer, 0);
        xTimerDelete(pairing_timeout_timer, 0);
        pairing_timeout_timer = NULL;
    }
    if (pin_send_timer != NULL) {
        xTimerStop(pin_send_timer, 0);
        xTimerDelete(pin_send_timer, 0);
        pin_send_timer = NULL;
    }

    // Reset state so future enables start cleanly
    connected = false;
    conn_handle = BLE_HS_CONN_HANDLE_NONE;
    client_subscribed = false;
    mtu_negotiated = false;
    pairing_pin = 0;
    pairing_confirmed = false;
    pairing_complete_time = 0;
    pairing_hide_requested = false;
    pin_send_count = 0;

    ESP_LOGI(TAG, "Bluetooth disabled cleanly (stack left running)\n");
    ble_shutdown_task = NULL;
    vTaskDelete(NULL);
}

void bluetooth_disable(void)
{
    if (!ble_enabled && ble_shutdown_task == NULL) return;
    ble_enabled = false;

    // If a shutdown task is already running, don't start another
    if (ble_shutdown_task != NULL) {
        ESP_LOGW(TAG, "Bluetooth disable already in progress");
        return;
    }

    BaseType_t rc = xTaskCreate(
        ble_shutdown_task_fn,
        "ble_shutdown",
        4096,
        NULL,
        tskIDLE_PRIORITY + 2,
        &ble_shutdown_task);

    if (rc != pdPASS) {
        ESP_LOGE(TAG, "Failed to create shutdown task (%ld)", (long)rc);
        ble_shutdown_task = NULL;
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
    if (!client_subscribed) {
        ESP_LOGW(TAG, "Cannot send: client not subscribed to notifications");
        return;
    }

    if (len > BT_TX_MAX_LEN) len = BT_TX_MAX_LEN;
    memcpy(bt_tx_value, data, len);
    bt_tx_len = len;

    // Notify subscribed centrals. Use the value handle.
    ble_gatts_chr_updated(bt_tx_val_handle);
    ESP_LOGI(TAG, "Notification sent successfully (%d bytes)", len);
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
