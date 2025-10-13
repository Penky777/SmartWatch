#ifndef CALENDAR_ICON_H
#define CALENDAR_ICON_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef LV_ATTRIBUTE_MEM_ALIGN
#define LV_ATTRIBUTE_MEM_ALIGN
#endif

#ifndef LV_ATTRIBUTE_IMAGE_CALENDAR
#define LV_ATTRIBUTE_IMAGE_CALENDAR
#endif

extern const lv_image_dsc_t Calendar; // deklarácia objektu
extern const uint8_t Calendar_map[]; // deklarácia bitmapy

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // CALENDAR_ICON_H
