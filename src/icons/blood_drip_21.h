#ifndef BLOOD_DRIP_21_H
#define BLOOD_DRIP_21_H

#ifdef __has_include
#  if __has_include("lvgl.h")
#    ifndef LV_LVGL_H_INCLUDE_SIMPLE
#      define LV_LVGL_H_INCLUDE_SIMPLE
#    endif
#  endif
#endif

#if defined(LV_LVGL_H_INCLUDE_SIMPLE)
#  include "lvgl.h"
#else
#  include "lvgl/lvgl.h"
#endif

#ifndef LV_ATTRIBUTE_MEM_ALIGN
#define LV_ATTRIBUTE_MEM_ALIGN
#endif

#ifndef LV_ATTRIBUTE_IMG_BLOOD_DRIP_21
#define LV_ATTRIBUTE_IMG_BLOOD_DRIP_21
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern const lv_image_dsc_t blood_drip_21;

#ifdef __cplusplus
}
#endif

#endif /* BLOOD_DRIP_21_H */
