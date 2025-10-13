#ifndef SETTINGS_H
#define SETTINGS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

// Deklarácia bitmapy Activity_map (extern, definovaná v .c)
extern const uint8_t Settings_map[];

// Deklarácia štruktúry obrázka
extern const lv_image_dsc_t Settings;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* SETTINGS_H */
