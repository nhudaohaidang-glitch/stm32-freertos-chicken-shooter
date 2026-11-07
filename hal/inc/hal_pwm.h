#ifndef HAL_PWM_H
#define HAL_PWM_H
#include "hal_status.h"
hal_status_t hal_pwm_init(void);
/* TIM3_CH3/PB0. freq_hz=0 silences buzzer; duty_permille range 0..1000. */
hal_status_t hal_pwm_set_buzzer(uint32_t freq_hz, uint16_t duty_permille);
#endif
