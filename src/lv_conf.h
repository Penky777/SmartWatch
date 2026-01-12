/**
 * Minimal LVGL configuration for ESP32-C6 + ST7789 + CST816T
 *
 * Place this file in `src/` or `include/` and make sure PlatformIO
 * knows about it (e.g. add `-Iinclude` to build_flags if needed).
 */

#ifndef LV_CONF_H
#define LV_CONF_H

/*====================
   Graphical settings
 *====================*/

/* Color depth: use 16-bit RGB565 (ST7789 uses this) */
#define LV_COLOR_DEPTH     16

/* Swap the two bytes in RGB565 color (ESP-IDF + ST7789 need this ON) */
#define LV_COLOR_16_SWAP   1

/* Screen resolution */
#define LV_HOR_RES_MAX     240
#define LV_VER_RES_MAX     280

/* Default background color */
#define LV_COLOR_SCREEN    lv_color_black()

/*====================
   Memory settings
 *====================*/

/* Size of the internal memory (in bytes) for LVGL */
#define LV_MEM_SIZE        (32U * 1024U)   

/*====================
   Text and fonts
 *====================*/

/* Default font */
#define LV_FONT_DEFAULT    &lv_font_montserrat_18

/* Enable only small fonts to save space */
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_18 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_28 1
#define LV_FONT_MONTSERRAT_48 1 
/* Add more sizes if you really need them */

/*====================
   Widgets / Features
 *====================*/

/* Enable widgets you need */
#define LV_USE_LABEL       1
#define LV_USE_BTN         1
#define LV_USE_SLIDER      1
#define LV_USE_LIST        1
#define LV_USE_IMG         1

/*====================
   Others
 *====================*/
#define LV_TICK_CUSTOM     1
#define LV_TICK_CUSTOM_SYS_TIME_EXPR (esp_timer_get_time()/1000)

#endif /* LV_CONF_H */
