#ifndef BSP_PWR_H
#define BSP_PWR_H

#include <stdbool.h>
#include "esp_err.h"

#define BAT_EN_PIN      15
#define PWR_KEY_PIN     18

// ✅ ADD: Event types enum
typedef enum {
    PWR_EVENT_NONE,
    PWR_EVENT_WAKE,
    PWR_EVENT_SLEEP,
    PWR_EVENT_GO_BACK,
    PWR_EVENT_SHUTDOWN
} pwr_event_t;

void bsp_pwr_init(void);
bool bsp_pwr_is_screen_sleeping(void);
void bsp_pwr_wake_screen(void);
void bsp_pwr_sleep_screen(void);

pwr_event_t bsp_pwr_get_event(void);

#endif
