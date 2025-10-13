#ifndef ACTIVITY_H
#define ACTIVITY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

// Deklarácia bitmapy Activity_map (extern, definovaná v .c)
extern const uint8_t Activity_map[];

// Deklarácia štruktúry obrázka
extern const lv_image_dsc_t Activity;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ACTIVITY_H */
