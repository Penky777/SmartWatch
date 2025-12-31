#ifndef BSP_PWR_H
#define BSP_PWR_H

#include <stdbool.h>
#include "esp_err.h"


#define BAT_EN_PIN      15
#define PWR_KEY_PIN     18

void bsp_pwr_init(void);
bool bsp_pwr_is_screen_sleeping(void);
void bsp_pwr_wake_screen(void);
void bsp_pwr_sleep_screen(void);

#endif 
