#ifndef HEART_19_H
#define HEART_19_H

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

#ifndef LV_ATTRIBUTE_IMG_HEART_19
#define LV_ATTRIBUTE_IMG_HEART_19
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern const lv_image_dsc_t heart_19;

#ifdef __cplusplus
}
#endif

#endif /* HEART_19_H */
