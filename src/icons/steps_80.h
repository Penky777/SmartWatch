#ifndef STEPS_80_H
#define STEPS_80_H

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

#ifndef LV_ATTRIBUTE_IMG_STEPS_80
#define LV_ATTRIBUTE_IMG_STEPS_80
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern const lv_image_dsc_t steps_80;

#ifdef __cplusplus
}
#endif

#endif /* STEPS_80_H */
