#include "esp_log.h"
#include "esp_err.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

#include "comm_manager.h"

static const char *TAG = "BLE_C6";

// Forward declarations
static void ble_app_on_sync(void);
static void ble_app_advertise(void);
void ble_host_task(void *param);
void bluetooth_send_bytes(const uint8_t *data, uint16_t len);

//  Bluetooth state 
static bool ble_enabled = false;
static bool ble_running = false;

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

// Periodic test message callback (every 5 seconds)
static void bt_test_timer_callback(TimerHandle_t xTimer)
{
    if (!ble_enabled || bt_tx_val_handle == 0) return;
    
    bt_test_counter++;
    char msg[64];
    snprintf(msg, sizeof(msg), "Test %lu", bt_test_counter);
    bluetooth_send_bytes((const uint8_t *)msg, strlen(msg));
    ESP_LOGI(TAG, "TX -> Phone: %s", msg);
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
    if (bt_tx_len <= 0) return 0;
    int rc = os_mbuf_copyinto(ctxt->om, 0, bt_tx_value, bt_tx_len);
    return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
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
    ble_app_advertise();
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

// Send bytes to connected centrals via GATT notification.
// Copies up to BT_TX_MAX_LEN bytes into the characteristic value and
// triggers a chr_updated which will send notifications to subscribed clients.
void bluetooth_send_bytes(const uint8_t *data, uint16_t len)
{
    if (!ble_enabled) return;
    if (bt_tx_val_handle == 0) return;

    if (len > BT_TX_MAX_LEN) len = BT_TX_MAX_LEN;
    memcpy(bt_tx_value, data, len);
    bt_tx_len = len;

    // Notify subscribed centrals. Use the value handle.
    ble_gatts_chr_updated(bt_tx_val_handle);
}

//  DO NOT ENABLE BY DEFAULT 
void bluetooth_init(void)
{
    // Start OFF
    ble_enabled = false;
}
