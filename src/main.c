#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/ledc.h"
#include "driver/i2c_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "lvgl.h"
#include "bsp_pcf85063.h"
#include "time_screen.h"

#include "icons/Calendar.h"
#include "icons/Activity.h"
#include "icons/Settings.h"
#include "icons/Calculator.h"

// ----------- PIN & BUS CONFIG -----------
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
#define BUFFER_ROWS         40

#define I2C_PORT            I2C_NUM_0
#define CST816_I2C_ADDR     0x15
#define REG_GESTURE         0x01
#define REG_XH              0x03
#define REG_XL              0x04
#define REG_YH              0x05
#define REG_YL              0x06
#define REG_ID_G_C          0xA7

// ----------- GLOBALS -----------
static const char *TAG = "APP_TOUCH_MENU";
static esp_lcd_panel_handle_t g_panel = NULL;
static i2c_master_bus_handle_t g_i2c_bus = NULL;
static i2c_master_dev_handle_t g_touch_dev = NULL;
static QueueHandle_t touch_evt_queue = NULL;
static SemaphoreHandle_t gui_mutex = NULL;

static lv_obj_t *header_time_lbl = NULL;
static lv_indev_t *indev_touch = NULL;
static uint32_t g_last_activity_ms = 0;
static bool g_backlight_on = true;
static uint8_t g_backlight_level = 100;
static volatile bool s_touch_irq_flag = false;

typedef struct {
    bool pressed;
    int16_t x;
    int16_t y;
} touch_sample_t;

static volatile touch_sample_t s_last_touch = {false, 0, 0};

// ----------- BACKLIGHT CONTROL -----------
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

static void backlight_set(uint8_t percent) {
    if (percent > 100) percent = 100;
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, (1023 * percent) / 100));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
}

// ----------- LVGL INTEGRATION -----------
static void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    int x1 = area->x1, y1 = area->y1;
    int x2e = area->x2 + 1, y2e = area->y2 + 1;
    ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(g_panel, x1, y1, x2e, y2e, px_map));
    lv_display_flush_ready(disp);
}

static void lvgl_tick_cb(void *arg) {
    (void)arg;
    lv_tick_inc(LV_TICK_PERIOD_MS);
}

static void gui_lock(void) {
    if (gui_mutex) xSemaphoreTake(gui_mutex, portMAX_DELAY);
}

static void gui_unlock(void) {
    if (gui_mutex) xSemaphoreGive(gui_mutex);
}

// ----------- I2C & TOUCH -----------
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

// ----------- INTERRUPTS & TOUCH READING -----------
static void IRAM_ATTR touch_isr(void *arg) {
    (void)arg;
    s_touch_irq_flag = true;
    BaseType_t xHigher = pdFALSE;
    uint8_t sig = 1;
    if (touch_evt_queue) xQueueSendFromISR(touch_evt_queue, &sig, &xHigher);
    if (xHigher) portYIELD_FROM_ISR();
}

static esp_err_t cst816_read_sample(touch_sample_t *out) {
    uint8_t buf[6] = {0}, start = REG_GESTURE;
    esp_err_t err = i2c_master_transmit_receive(g_touch_dev, &start, 1, buf, sizeof(buf), 50);
    if (err != ESP_OK) return err;
    out->pressed = (buf[1] & 0x0F) > 0;
    uint16_t x = ((buf[2] & 0x0F) << 8) | buf[3];
    uint16_t y = ((buf[4] & 0x0F) << 8) | buf[5];
    if (x >= LCD_WIDTH) x = LCD_WIDTH - 1;
    if (y >= LCD_HEIGHT) y = LCD_HEIGHT - 1;
    out->x = (int16_t)x;
    out->y = (int16_t)y;
    return ESP_OK;
}

static void lvgl_touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data) {
    (void)indev;
    data->state = s_last_touch.pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
    data->point.x = s_last_touch.x;
    data->point.y = s_last_touch.y;
    data->continue_reading = false;

    if (data->state == LV_INDEV_STATE_PRESSED) {
        g_last_activity_ms = lv_tick_get();
        if (!g_backlight_on) { backlight_set(g_backlight_level); g_backlight_on = true; }
    }
}

// ----------- UI BUILDERS -----------
static void calendar_event_cb(lv_event_t *e) {
    lv_obj_t *btn = lv_event_get_target(e);
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        lv_label_set_text(lv_obj_get_child(btn, 1), "Opened!");
    }
}

static void build_ui(void) {
    lv_obj_t *scr = lv_scr_act();

    // Nastav čierne pozadie celej obrazovky
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    lv_obj_t *container = lv_obj_create(scr);
    lv_obj_set_width(container, LV_PCT(100));
    lv_obj_set_height(container, LV_SIZE_CONTENT);  // dynamická výška podľa obsahu

    // Povolené vertikálne scrollovanie
    lv_obj_set_scroll_dir(container, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(container, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_snap_y(container, LV_SCROLL_SNAP_NONE);

    // Flex flow - vertikálny stĺpec
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Paddingy
    lv_obj_set_style_pad_row(container, 10, 0);
    lv_obj_set_style_pad_column(container, 0, 0);
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_style_bg_color(container, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(container, LV_OPA_COVER, 0);



    // Nastavenie čierneho pozadia pre kontajner
    lv_obj_set_style_bg_color(container, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(container, LV_OPA_COVER, 0);

    // Menu nadpis v bielej farbe
    lv_obj_t *menu_lbl = lv_label_create(container);
    lv_label_set_text(menu_lbl, "Menu");
    lv_obj_set_style_text_color(menu_lbl, lv_color_white(), 0);
    lv_obj_set_style_text_align(menu_lbl, LV_TEXT_ALIGN_CENTER, 0);
    

    // Calendar ikona + popis
    lv_obj_t *calendar_img = lv_img_create(container);
    lv_img_set_src(calendar_img, &Calendar);
    lv_obj_set_size(calendar_img, 90, 90);
    lv_obj_center(calendar_img);

    lv_obj_t *calendar_lbl = lv_label_create(container);
    lv_label_set_text(calendar_lbl, "Calendar");
    lv_obj_set_style_text_color(calendar_lbl, lv_color_white(), 0);
    lv_obj_set_style_text_align(calendar_lbl, LV_TEXT_ALIGN_CENTER, 0);

    // Activity ikona + popis
    lv_obj_t *activity = lv_img_create(container);
    lv_img_set_src(activity, &Activity);   
    lv_obj_set_size(activity, 90, 90);
    lv_obj_center(activity);

    lv_obj_t *activity_lbl = lv_label_create(container);
    lv_label_set_text(activity_lbl, "Activity");
    lv_obj_set_style_text_color(activity_lbl, lv_color_white(), 0);
    lv_obj_set_style_text_align(activity_lbl, LV_TEXT_ALIGN_CENTER, 0);

    // Settings ikona + popis
    lv_obj_t *settings = lv_img_create(container);
    lv_img_set_src(settings, &Settings);   
    lv_obj_set_size(settings, 90, 90);
    lv_obj_center(settings);

    lv_obj_t *settings_lbl = lv_label_create(container);
    lv_label_set_text(settings_lbl, "Settings");
    lv_obj_set_style_text_color(settings_lbl, lv_color_white(), 0);
    lv_obj_set_style_text_align(settings_lbl, LV_TEXT_ALIGN_CENTER, 0);

    // Calculator ikona + popis
    lv_obj_t *calculator = lv_img_create(container);
    lv_img_set_src(calculator, &Calculator);   
    lv_obj_set_size(calculator, 90, 90);
    lv_obj_center(calculator);

    lv_obj_t *calculator_lbl = lv_label_create(container);
    lv_label_set_text(calculator_lbl, "Calculator");
    lv_obj_set_style_text_color(calculator_lbl, lv_color_white(), 0);
    lv_obj_set_style_text_align(calculator_lbl, LV_TEXT_ALIGN_CENTER, 0);
}


// ----------- FREERTOS TASKS -----------
static void clock_task(void *arg) {
    (void)arg;
    while (1) {
        struct tm now;
        if (bsp_pcf85063_get_time(&now)) {
            char buf[16];
            snprintf(buf, sizeof(buf), "%02d:%02d:%02d", now.tm_hour, now.tm_min, now.tm_sec);
            gui_lock();
            if(header_time_lbl) lv_label_set_text(header_time_lbl, buf);
            time_screen_update(buf);
            gui_unlock();
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void touch_task(void *arg) {
    (void)arg;
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

    touch_sample_t samp = {0};
    (void)cst816_read_sample(&samp);

    while (1) {
        uint8_t sig;
        (void)xQueueReceive(touch_evt_queue, &sig, pdMS_TO_TICKS(20));
        if (s_touch_irq_flag || s_last_touch.pressed) {
            if(cst816_read_sample(&samp) == ESP_OK)
                s_last_touch = samp;
            s_touch_irq_flag = false;
        }
    }
}

// ----------- MAIN FLOW -----------
void app_main(void) {
    // SPI & Panel
    spi_bus_config_t buscfg = {
        .sclk_io_num = LCD_SCLK_GPIO,
        .mosi_io_num = LCD_MOSI_GPIO,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_WIDTH * BUFFER_ROWS * 2
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));

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

    // Backlight
    backlight_init();
    backlight_set(100);

    // I2C and peripherals
    ESP_ERROR_CHECK(i2c_bus_init());
    bsp_pcf85063_init(g_i2c_bus);

    ESP_ERROR_CHECK(cst816_add_device());
    uint8_t id = 0;
    if (cst816_probe_id(&id) == ESP_OK)
        ESP_LOGI(TAG, "CST816 ID=0x%02X", id);

    // LVGL Init
    lv_init();
    g_last_activity_ms = lv_tick_get();
    gui_mutex = xSemaphoreCreateMutex();

    static lv_color_t buf1[LCD_WIDTH * BUFFER_ROWS];
    static lv_color_t buf2[LCD_WIDTH * BUFFER_ROWS];
    lv_display_t *disp = lv_display_create(LCD_WIDTH, LCD_HEIGHT);
    lv_display_set_flush_cb(disp, lvgl_flush_cb);
    lv_display_set_buffers(disp, buf1, buf2, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);

    indev_touch = lv_indev_create();
    lv_indev_set_type(indev_touch, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev_touch, lvgl_touch_read_cb);

    // Periodic tick
    const esp_timer_create_args_t tick_args = {
        .callback = &lvgl_tick_cb,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "lv_tick"
    };
    esp_timer_handle_t tick_timer;
    ESP_ERROR_CHECK(esp_timer_create(&tick_args, &tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(tick_timer, LV_TICK_PERIOD_MS * 1000));

    // Initialize screen & UI
    time_screen_init();

    gui_lock(); build_ui(); gui_unlock();

    // Start FreeRTOS tasks
    touch_evt_queue = xQueueCreate(8, 1);
    xTaskCreate(touch_task, "touch_task", 4096, NULL, 6, NULL);
    xTaskCreate(clock_task, "clock_task", 4096, NULL, 5, NULL);

    // LVGL Mainloop with inactivity/backlight handling
    while (1) {
        gui_lock(); lv_timer_handler(); gui_unlock();
        uint32_t now = lv_tick_get();
        if (g_backlight_on && now - g_last_activity_ms > 10000) {
            backlight_set(0);
            g_backlight_on = false;
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
