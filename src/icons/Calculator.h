#ifndef CALCULATOR_H
#define CALCULATOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

// Deklarácia bitmapy Activity_map (extern, definovaná v .c)
extern const uint8_t Calculator_map[];

// Deklarácia štruktúry obrázka
extern const lv_image_dsc_t Calculator;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* CALCULATOR_H */
