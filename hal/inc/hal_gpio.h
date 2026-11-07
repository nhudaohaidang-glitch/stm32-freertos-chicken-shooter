#ifndef HAL_GPIO_H
#define HAL_GPIO_H
#include "hal_status.h"
hal_status_t hal_gpio_init(void);
/* bits 0/1 = Fire / Start-Pause; logical pressed, independent of polarity. */
hal_status_t hal_gpio_read_buttons(uint8_t *pressed);
/* Debounce in HMI; EXTI, if used, only records edges via ...FromISR APIs. */
#endif
