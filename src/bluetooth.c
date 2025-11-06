#include "esp_log.h"
#include "esp_err.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

#include "host/ble_uuid.h"


static const char *TAG = "BLE_C6";

static void ble_app_on_sync(void);
static void ble_app_advertise(void);



static const ble_uuid128_t gatt_svc_uuid =
    BLE_UUID128_INIT(0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0, 0x12, 0x34, 0x56, 0x78, 0x90, 0xab, 0xcd, 0xef);

static const ble_uuid128_t gatt_chr_rx_uuid =
    BLE_UUID128_INIT(0xab, 0xcd, 0xef, 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0, 0x12, 0x34, 0x56, 0x78, 0x90);

static int ble_rx_write_cb(uint16_t conn_handle, uint16_t attr_handle,
                           struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    // Data written from phone is in ctxt->om (OS memory buffer)
    uint16_t len = OS_MBUF_PKTLEN(ctxt->om);
    uint8_t buf[128];
    os_mbuf_copydata(ctxt->om, 0, len, buf);
    buf[len] = '\0';

    ESP_LOGI("BLE_RX", "Received data from phone: %s", buf);
    return 0; // Success
}

static const struct ble_gatt_svc_def gatt_svcs[] = {
    {
        /*** Custom service definition ***/
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &gatt_svc_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = &gatt_chr_rx_uuid.u,
                .access_cb = ble_rx_write_cb,
                .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP,
            },
            {0} /* No more characteristics */
        },
    },
    {0} /* No more services */
};

// BLE host task
void ble_host_task(void *param)
{
    ESP_LOGI(TAG, "NimBLE host task started");
    nimble_port_run(); // This function will return only when NimBLE is stopped
    nimble_port_freertos_deinit();
}

// Called when NimBLE stack is synced
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

    struct ble_gap_adv_params adv_params = {0};
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    int rc = ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER,
                               &adv_params, NULL, NULL);

    if (rc == 0) {
        ESP_LOGI(TAG, "Advertising started successfully");
    } else {
        ESP_LOGE(TAG, "Advertising failed: %d", rc);
    }
}

void bluetooth_init(void)
{
    ESP_LOGI(TAG, "Initializing NimBLE...");

    // Initialize NimBLE host
    nimble_port_init();

    // Register the sync callback BEFORE initializing services
    ble_hs_cfg.sync_cb = ble_app_on_sync;

    // Initialize the default GATT and GAP services
    ble_svc_gap_init();
    ble_svc_gatt_init();


    ble_gatts_count_cfg(gatt_svcs);
    ble_gatts_add_svcs(gatt_svcs);

    // Start the NimBLE host task
    nimble_port_freertos_init(ble_host_task);
}
