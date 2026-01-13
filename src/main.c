#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include "bluetooth.h"
#include "nvs_flash.h"
#include "ui_manager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/ledc.h"
#include "driver/i2c_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_heap_caps.h"
#include "lvgl.h"
#include "bsp_pcf85063.h"
#include "bsp_qmi8658.h"
#include "time_screen.h"
#include "menu_screen.h"
#include "settings_screen.h"
#include "esp_sleep.h"
#include "esp_pm.h"
#include "max30102.h"
#include "activity_screen.h"
#include "vibration.h"
#include "bsp_pwr.h"
#include "bsp_battery.h"
#include "watchface_screen.h"

// ============================================================================
// PIN CONFIGURATION
// ============================================================================
#define LCD_SCLK_GPIO   1
#define LCD_MOSI_GPIO   2
#define LCD_DC_GPIO     3
#define LCD_RST_GPIO    4
#define LCD_CS_GPIO     5
#define LCD_BL_GPIO     6
#define TP_SCL_GPIO     7
#define TP_SDA_GPIO     8
#define TP_INT_GPIO     11

#define LCD_WIDTH           240
#define LCD_HEIGHT          280
#define LV_TICK_PERIOD_MS   2
#define BUFFER_ROWS         20  // Reduced from 40 to save memory

#define I2C_PORT            I2C_NUM_0
#define CST816_I2C_ADDR     0x15
#define REG_GESTURE         0x01
#define REG_XH              0x03
#define REG_XL              0x04
#define REG_YH              0x05
#define REG_YL              0x06
#define REG_ID_G_C          0xA7

// Timeouts
#define SCREEN_TIMEOUT_MS       10000
#define HEAP_CHECK_INTERVAL_MS  2000
#define CLOCK_UPDATE_MS         1000

// ============================================================================
// GLOBAL VARIABLES
// ============================================================================
static const char *TAG = "SMARTWATCH";
static esp_lcd_panel_handle_t g_panel = NULL;
static i2c_master_bus_handle_t g_i2c_bus = NULL;
static i2c_master_dev_handle_t g_touch_dev = NULL;
static i2c_master_dev_handle_t g_max_dev = NULL;
static QueueHandle_t touch_evt_queue = NULL;
static SemaphoreHandle_t gui_mutex = NULL;
static lv_indev_t *indev_touch = NULL;

uint32_t g_last_activity_ms = 0;
bool g_backlight_on = true;
uint8_t g_backlight_level = 100;
static volatile bool s_touch_irq_flag = false;

typedef struct {
    bool pressed;
    int16_t x;
    int16_t y;
} touch_sample_t;

static volatile touch_sample_t s_last_touch = {false, 0, 0};

// ============================================================================
// HEAP MONITORING
// ============================================================================
typedef struct {
    size_t free_heap;
    size_t free_internal;
    size_t largest_block;
    size_t min_free_ever;
} heap_stats_t;

static void get_heap_stats(heap_stats_t *stats) {
    stats->free_heap = esp_get_free_heap_size();
    stats->free_internal = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    stats->largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
    stats->min_free_ever = esp_get_minimum_free_heap_size();
}

static void log_heap_stats(const char *context) {
    heap_stats_t stats;
    get_heap_stats(&stats);
    
    ESP_LOGI("HEAP", "[%s] Free: %u bytes | Internal: %u | Largest block: %u | Min ever: %u",
             context,
             (unsigned)stats.free_heap,
             (unsigned)stats.free_internal,
             (unsigned)stats.largest_block,
             (unsigned)stats.min_free_ever);
    
    // Warn if fragmentation is severe
    if (stats.largest_block < stats.free_internal / 2) {
        ESP_LOGW("HEAP", "Fragmentation detected! Largest block only %u of %u free",
                 (unsigned)stats.largest_block, (unsigned)stats.free_internal);
    }
    
    // Critical warning
    if (stats.free_internal < 30000) {
        ESP_LOGE("HEAP", "CRITICAL: Only %u bytes internal RAM remaining!",
                 (unsigned)stats.free_internal);
    }
}

static bool check_heap_integrity(const char *context) {
    bool ok = heap_caps_check_integrity_all(true);
    if (!ok) {
        ESP_LOGE("HEAP", "CORRUPTION DETECTED at %s!", context);
    }
    return ok;
}

// ============================================================================
// BACKLIGHT CONTROL
// ============================================================================
static void backlight_init(void) {
    ledc_timer_config_t timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .freq_hz = 5000,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer));

    ledc_channel_config_t channel = {
        .gpio_num = LCD_BL_GPIO,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&channel));
}

void backlight_set(uint8_t percent) {
    if (percent > 100) percent = 100;
    uint32_t duty = (1023 * percent) / 100;
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
}

// ============================================================================
// GUI LOCK
// ============================================================================
void gui_lock(void) {
    if (gui_mutex) {
        xSemaphoreTake(gui_mutex, portMAX_DELAY);
    }
}

void gui_unlock(void) {
    if (gui_mutex) {
        xSemaphoreGive(gui_mutex);
    }
}

// ============================================================================
// LIGHT SLEEP
// ============================================================================
static void enter_light_sleep(void) {
    ESP_LOGI(TAG, "Entering light sleep...");
    
    backlight_set(0);
    g_backlight_on = false;

    // Configure wake on touch interrupt
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << TP_INT_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));
    ESP_ERROR_CHECK(esp_sleep_enable_ext1_wakeup(1ULL << TP_INT_GPIO, ESP_EXT1_WAKEUP_ANY_HIGH));

    vTaskDelay(pdMS_TO_TICKS(50));
    esp_light_sleep_start();
    
    ESP_LOGI(TAG, "Woke from light sleep");
    backlight_set(g_backlight_level);
    g_backlight_on = true;
    g_last_activity_ms = lv_tick_get();
}

// ============================================================================
// LVGL CALLBACKS
// ============================================================================
static void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    int x1 = area->x1;
    int y1 = area->y1;
    int x2e = area->x2 + 1;
    int y2e = area->y2 + 1;
    
    esp_err_t ret = esp_lcd_panel_draw_bitmap(g_panel, x1, y1, x2e, y2e, px_map);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LCD draw failed: %s", esp_err_to_name(ret));
    }
    
    lv_display_flush_ready(disp);
}

static void lvgl_tick_cb(void *arg) {
    (void)arg;
    lv_tick_inc(LV_TICK_PERIOD_MS);
}

static void lvgl_touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data) {
    (void)indev;
    
    data->state = s_last_touch.pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
    data->point.x = s_last_touch.x;
    data->point.y = s_last_touch.y;
    data->continue_reading = false;

    if (data->state == LV_INDEV_STATE_PRESSED) {
    
        if (bsp_pwr_is_screen_sleeping()) {
            bsp_pwr_wake_screen();
            return;  // Don't process this touch, just wake
        }
        
        g_last_activity_ms = lv_tick_get();
    }
}

// ============================================================================
// I2C & TOUCH INITIALIZATION
// ============================================================================
static esp_err_t i2c_bus_init(void) {
    i2c_master_bus_config_t cfg = {
        .i2c_port = I2C_PORT,
        .sda_io_num = TP_SDA_GPIO,
        .scl_io_num = TP_SCL_GPIO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags = {.enable_internal_pullup = true},
    };
    return i2c_new_master_bus(&cfg, &g_i2c_bus);
}

static esp_err_t cst816_add_device(void) {
    i2c_device_config_t dev_cfg = {
        .device_address = CST816_I2C_ADDR,
        .scl_speed_hz = 400000,
    };
    return i2c_master_bus_add_device(g_i2c_bus, &dev_cfg, &g_touch_dev);
}

static esp_err_t cst816_probe_id(uint8_t *out_id) {
    uint8_t reg = REG_ID_G_C;
    return i2c_master_transmit_receive(g_touch_dev, &reg, 1, out_id, 1, 50);
}

static esp_err_t cst816_read_sample(touch_sample_t *out) {
    uint8_t buf[6] = {0};
    uint8_t start = REG_GESTURE;
    
    esp_err_t err = i2c_master_transmit_receive(g_touch_dev, &start, 1, buf, sizeof(buf), 50);
    if (err != ESP_OK) {
        return err;
    }
    
    out->pressed = (buf[1] & 0x0F) > 0;
    uint16_t x = ((buf[2] & 0x0F) << 8) | buf[3];
    uint16_t y = ((buf[4] & 0x0F) << 8) | buf[5];
    
    if (x >= LCD_WIDTH) x = LCD_WIDTH - 1;
    if (y >= LCD_HEIGHT) y = LCD_HEIGHT - 1;
    
    out->x = (int16_t)x;
    out->y = (int16_t)y;
    
    return ESP_OK;
}

// ============================================================================
// TOUCH INTERRUPT
// ============================================================================
static void IRAM_ATTR touch_isr(void *arg) {
    (void)arg;
    s_touch_irq_flag = true;
    
    BaseType_t xHigher = pdFALSE;
    uint8_t sig = 1;
    if (touch_evt_queue) {
        xQueueSendFromISR(touch_evt_queue, &sig, &xHigher);
    }
    if (xHigher) {
        portYIELD_FROM_ISR();
    }
}

// ============================================================================
// FREERTOS TASKS
// ============================================================================
static void clock_task(void *arg) {
    (void)arg;
    
    ESP_LOGI(TAG, "Clock task started");
    
    while (1) {
        struct tm now;
        if (bsp_pcf85063_get_time(&now)) {
            char buf[16];
            snprintf(buf, sizeof(buf), "%02d:%02d:%02d", 
                     now.tm_hour, now.tm_min, now.tm_sec);
            
            gui_lock();
            time_screen_update(buf);
            gui_unlock();
        }
        
        vTaskDelay(pdMS_TO_TICKS(CLOCK_UPDATE_MS));
    }
}

static void touch_task(void *arg) {
    (void)arg;
    
    ESP_LOGI(TAG, "Touch task started");
    
    // Configure touch interrupt GPIO
    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << TP_INT_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE
    };
    ESP_ERROR_CHECK(gpio_config(&cfg));
    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    ESP_ERROR_CHECK(gpio_isr_handler_add(TP_INT_GPIO, touch_isr, NULL));

    // Clear any pending touch event
    touch_sample_t samp = {0};
    cst816_read_sample(&samp);
    
    static uint32_t touch_count = 0;

    while (1) {
        uint8_t sig;
        xQueueReceive(touch_evt_queue, &sig, pdMS_TO_TICKS(20));
        
        if (s_touch_irq_flag || s_last_touch.pressed) {
            if (cst816_read_sample(&samp) == ESP_OK) {
                s_last_touch = samp;
                
                if (samp.pressed) {
                    touch_count++;
                    ESP_LOGD("TOUCH", "Event #%lu | X:%d Y:%d", 
                             touch_count, samp.x, samp.y);
                }
            }
            s_touch_irq_flag = false;
        }
    }
}

static void monitor_task(void *arg) {
    (void)arg;
    
    ESP_LOGI(TAG, "Monitor task started");
    
    uint32_t last_check = 0;
    
    while (1) {
        uint32_t now = lv_tick_get();
        
        // Log heap stats periodically
        if (now - last_check >= HEAP_CHECK_INTERVAL_MS) {
            log_heap_stats("periodic");
            check_heap_integrity("periodic");
            last_check = now;
        }
        
        // ✅ Auto-sleep with power management
        if (!bsp_pwr_is_screen_sleeping() &&      // Not already sleeping
            g_backlight_on &&                      // Backlight is on
            now - g_last_activity_ms > SCREEN_TIMEOUT_MS) {
            
            ESP_LOGI(TAG, "Inactivity timeout → Auto-sleep");
            bsp_pwr_sleep_screen();
        }
        
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// ============================================================================
// LCD INITIALIZATION
// ============================================================================
static esp_err_t lcd_init(void) {
    ESP_LOGI(TAG, "Initializing LCD...");
    
    // SPI bus configuration
    spi_bus_config_t buscfg = {
        .sclk_io_num = LCD_SCLK_GPIO,
        .mosi_io_num = LCD_MOSI_GPIO,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_WIDTH * BUFFER_ROWS * sizeof(lv_color_t)
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));

    // LCD panel IO
    esp_lcd_panel_io_handle_t io = NULL;
    esp_lcd_panel_io_spi_config_t iocfg = {
        .dc_gpio_num = LCD_DC_GPIO,
        .cs_gpio_num = LCD_CS_GPIO,
        .pclk_hz = 60 * 1000 * 1000,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST, &iocfg, &io));

    // LCD panel
    esp_lcd_panel_dev_config_t pcfg = {
        .reset_gpio_num = LCD_RST_GPIO,
        .color_space = ESP_LCD_COLOR_SPACE_RGB,
        .bits_per_pixel = 16
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io, &pcfg, &g_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(g_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(g_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(g_panel, true));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(g_panel, true));
    esp_lcd_panel_set_gap(g_panel, 0, 20);
    
    ESP_LOGI(TAG, "LCD initialized successfully");
    return ESP_OK;
}

// ============================================================================
// LVGL INITIALIZATION
// ============================================================================
static esp_err_t lvgl_init(void) {
    ESP_LOGI(TAG, "Initializing LVGL...");
    log_heap_stats("before LVGL init");
    
    lv_init();
    
    // Allocate DMA-capable buffers
    size_t buf_size = LCD_WIDTH * BUFFER_ROWS * sizeof(lv_color_t);
    ESP_LOGI(TAG, "Allocating 2 × %u bytes for LVGL buffers", (unsigned)buf_size);
    
    lv_color_t *buf1 = heap_caps_malloc(buf_size, MALLOC_CAP_DMA);
    lv_color_t *buf2 = heap_caps_malloc(buf_size, MALLOC_CAP_DMA);
    
    if (!buf1 || !buf2) {
        ESP_LOGE(TAG, "Failed to allocate LVGL buffers!");
        log_heap_stats("LVGL buffer alloc FAILED");
        if (buf1) free(buf1);
        if (buf2) free(buf2);
        return ESP_ERR_NO_MEM;
    }
    
    ESP_LOGI(TAG, "LVGL buffers allocated successfully");
    log_heap_stats("after LVGL buffer alloc");

    // Create display
    lv_display_t *disp = lv_display_create(LCD_WIDTH, LCD_HEIGHT);
    if (!disp) {
        ESP_LOGE(TAG, "Failed to create LVGL display");
        free(buf1);
        free(buf2);
        return ESP_FAIL;
    }
    
    lv_display_set_flush_cb(disp, lvgl_flush_cb);
    lv_display_set_buffers(disp, buf1, buf2, buf_size, LV_DISPLAY_RENDER_MODE_PARTIAL);
    
    // Create touch input device
    indev_touch = lv_indev_create();
    lv_indev_set_type(indev_touch, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev_touch, lvgl_touch_read_cb);

    // Start LVGL tick timer
    const esp_timer_create_args_t tick_args = {
        .callback = &lvgl_tick_cb,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "lv_tick"
    };
    esp_timer_handle_t tick_timer;
    ESP_ERROR_CHECK(esp_timer_create(&tick_args, &tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(tick_timer, LV_TICK_PERIOD_MS * 1000));
    
    ESP_LOGI(TAG, "LVGL initialized successfully");
    log_heap_stats("after LVGL init");
    
    return ESP_OK;
}

// ============================================================================
// MAIN APPLICATION
// ============================================================================
void app_main(void) {
    printf("\n\n========================================\n");
    printf("SMARTWATCH FIRMWARE STARTING\n");
    printf("========================================\n\n");

    ESP_LOGI("MAIN", "========================================");
    ESP_LOGI("MAIN", "         SMARTWATCH BOOT START         ");
    ESP_LOGI("MAIN", "========================================");
    
    // Check for crash on previous boot
    esp_reset_reason_t reset_reason = esp_reset_reason();
    const char *reason_str;
    
    switch(reset_reason) {
        case ESP_RST_UNKNOWN:   reason_str = "Unknown"; break;
        case ESP_RST_POWERON:   reason_str = "Power on"; break;
        case ESP_RST_SW:        reason_str = "Software reset"; break;
        case ESP_RST_PANIC:     reason_str = "⚠️ PANIC/CRASH"; break;
        case ESP_RST_INT_WDT:   reason_str = "⚠️ WATCHDOG"; break;
        case ESP_RST_TASK_WDT:  reason_str = "⚠️ TASK WATCHDOG"; break;
        case ESP_RST_WDT:       reason_str = "⚠️ OTHER WATCHDOG"; break;
        case ESP_RST_DEEPSLEEP: reason_str = "Deep sleep"; break;
        case ESP_RST_BROWNOUT:  reason_str = "⚠️ BROWNOUT"; break;
        default:                reason_str = "Other"; break;
    }
    
    ESP_LOGI("MAIN", "Reset reason: %s (%d)", reason_str, reset_reason);
    
    if (reset_reason == ESP_RST_PANIC || 
        reset_reason == ESP_RST_INT_WDT || 
        reset_reason == ESP_RST_TASK_WDT) {
        ESP_LOGE("MAIN", "");
        ESP_LOGE("MAIN", "╔════════════════════════════════════╗");
        ESP_LOGE("MAIN", "║  CRASHED ON PREVIOUS BOOT!        ║");
        ESP_LOGE("MAIN", "║  Check logs above for details     ║");
        ESP_LOGE("MAIN", "╚════════════════════════════════════╝");
        ESP_LOGE("MAIN", "");
        vTaskDelay(pdMS_TO_TICKS(3000));  // Pause to see message
    }
    
    ESP_LOGI("MAIN", "Free heap: %u bytes", esp_get_free_heap_size());
    
    // Check wakeup cause
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    switch (cause) {
        case ESP_SLEEP_WAKEUP_EXT0:
            ESP_LOGI(TAG, "Woke from EXT0 (touch)");
            break;
        case ESP_SLEEP_WAKEUP_EXT1:
            ESP_LOGI(TAG, "Woke from EXT1");
            break;
        default:
            ESP_LOGI(TAG, "Power-on or reset");
            break;
    }
    
    log_heap_stats("boot");
    // Initialize PWR button
    ESP_LOGI(TAG, "Initializing power management...");
    bsp_pwr_init();
    vTaskDelay(pdMS_TO_TICKS(200));  
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition erased, reinitializing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "NVS initialized");
    
    // Initialize LCD
    ESP_ERROR_CHECK(lcd_init());
    
    // Initialize backlight
    backlight_init();
    backlight_set(100);
    
    // Initialize I2C bus ONCE - used by RTC, IMU, Touch, MAX30102
    ESP_LOGI(TAG, "Initializing I2C bus...");
    ESP_ERROR_CHECK(i2c_bus_init());
    log_heap_stats("after I2C init");
    
    // Initialize RTC
    ESP_LOGI(TAG, "Initializing RTC...");
    bsp_pcf85063_init(g_i2c_bus);
    
    // Initialize IMU
    ESP_LOGI(TAG, "Initializing IMU...");
    bsp_qmi8658_init(g_i2c_bus);
    bsp_qmi8658_start_step_detection();
    bsp_qmi8658_test();
    
    // Initialize touch controller
    ESP_LOGI(TAG, "Initializing touch controller...");
    ESP_ERROR_CHECK(cst816_add_device());
    uint8_t touch_id = 0;
    if (cst816_probe_id(&touch_id) == ESP_OK) {
        ESP_LOGI(TAG, "CST816 detected, ID=0x%02X", touch_id);
    } else {
        ESP_LOGW(TAG, "CST816 probe failed");
    }
    
    // Initialize MAX30102 sensor
    ESP_LOGI(TAG, "Initializing MAX30102...");
    esp_err_t max_err = max_init(g_i2c_bus);
if (max_err != ESP_OK) {
    ESP_LOGW("MAIN", "MAX30102 sensor not detected (error 0x%x)", max_err);
    ESP_LOGW("MAIN", "Continuing without heart rate monitoring...");
} else {
    ESP_LOGI("MAIN", "MAX30102 initialized successfully");
}
    
    // Initialize vibration motor
    ESP_LOGI(TAG, "Initializing vibration motor...");
    vibe_init();
    vibe_pulse();
    
    log_heap_stats("after hardware init");
    
    //Initialize battery read
    ESP_LOGI(TAG,"Initializing battery");
    bsp_battery_init();

    // Initialize LVGL
    ESP_ERROR_CHECK(lvgl_init());
    
    // Create GUI mutex BEFORE using it
    gui_mutex = xSemaphoreCreateMutex();
    if (!gui_mutex) {
        ESP_LOGE(TAG, "Failed to create GUI mutex");
        return;
    }
    
    g_last_activity_ms = lv_tick_get();
    
    // Initialize UI manager
    ESP_LOGI(TAG, "Initializing UI manager...");
    log_heap_stats("before UI manager init");
    
    gui_lock();
    ui_manager_init();
    ui_show_watchface();
    vTaskDelay(pdMS_TO_TICKS(500));  
    gui_unlock();
    
    log_heap_stats("after UI manager init");
    check_heap_integrity("after UI init");
    
    // Initialize Bluetooth
    ESP_LOGI(TAG, "Initializing Bluetooth...");
    log_heap_stats("before BLE init");
    bluetooth_init();
    bluetooth_enable();
    log_heap_stats("after BLE init");
    
    // Create MAX30102 background task
    ESP_LOGI(TAG, "Creating MAX30102 task...");
    ESP_ERROR_CHECK(max_create_task());
    
    // Create FreeRTOS tasks
    touch_evt_queue = xQueueCreate(8, 1);
    if (!touch_evt_queue) {
        ESP_LOGE(TAG, "Failed to create touch queue");
        return;
    }
    
    ESP_LOGI(TAG, "Creating tasks...");

    xTaskCreate(touch_task, "touch", 8192, NULL, 6, NULL);   // Was 8192
    xTaskCreate(clock_task, "clock", 4096, NULL, 5, NULL);   // Was 4096
    xTaskCreate(monitor_task, "monitor", 3072, NULL, 4, NULL); // Was 3072
    
    log_heap_stats("after task creation");
    
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "INITIALIZATION COMPLETE");
    ESP_LOGI(TAG, "========================================\n");
    
    // Force initial render
    vTaskDelay(pdMS_TO_TICKS(50));
    
    // Main loop
    static uint32_t last_perf_log = 0;

    
    while (1) {
         pwr_event_t pwr_event = bsp_pwr_get_event();

         
    
    if (pwr_event != PWR_EVENT_NONE) {
        gui_lock();  
        
        switch (pwr_event) {
            case PWR_EVENT_WAKE:
                bsp_pwr_wake_screen();
                break;
                
            case PWR_EVENT_GO_BACK:
                if (ui_can_go_back()) {
                    ESP_LOGI(TAG, "→ Going back to previous screen");
                    ui_go_back();
                    g_last_activity_ms = lv_tick_get();
                } else {
                    ESP_LOGI(TAG, "→ On main screen, going to sleep");
                    bsp_pwr_sleep_screen();
                }
                break;
                
            case PWR_EVENT_SHUTDOWN:
                ESP_LOGW(TAG, "Shutting down...");
                backlight_set(0);
                vTaskDelay(pdMS_TO_TICKS(200));
                gpio_set_level(BAT_EN_PIN, 0);  // Cut power
                break;
                
            default:
                break;
        }
        
        gui_unlock();
    }
    
        // LOCK before LVGL operations
        gui_lock();
        uint32_t timeout = lv_timer_handler();
        gui_unlock();
        
        // Bluetooth polling 
        bluetooth_poll();
        
        uint32_t now = lv_tick_get();
        
        // Screen timeout check
        if (g_backlight_on && now - g_last_activity_ms > SCREEN_TIMEOUT_MS) {
            ESP_LOGI(TAG, "Screen timeout");
            bsp_pwr_sleep_screen();  
        }
        
        
        if (now - last_perf_log > 2000) {
            ESP_LOGI("PERF", "Heap: %u bytes", (unsigned)esp_get_free_heap_size());
            last_perf_log = now;
        }
        
        // lv_mem_monitor_t m  on;
        // lv_mem_monitor(&mon);   
        // ESP_LOGI("LVGL", "Used: 
            
            
            
        //     %u/%u bytes (%.1f%%)", 
        //  mon.used_cnt, mon.total_size, 
        //  (mon.used_cnt * 100.0f) / mon.total_size);




        
        uint32_t delay_ms = (timeout > 0 && timeout < 20) ? timeout : 10;
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
}
