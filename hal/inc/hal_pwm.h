#ifndef HAL_PWM_H
#define HAL_PWM_H
#include "hal_status.h"
/* Dải tần hợp lệ cho buzzer thụ động; ngoài dải -> HAL_INVALID_ARG. */
#define HAL_PWM_MIN_FREQ_HZ 20U
#define HAL_PWM_MAX_FREQ_HZ 20000U

/* Gọi trước khi tạo task. TIM3 đếm 1 MHz, tự tính prescaler theo clock thực. */
hal_status_t hal_pwm_init(void);
/* TIM3_CH3/PB0. freq_hz=0 silences buzzer; duty_permille range 0..1000.
 * Buzzer thụ động to nhất ở 500. Áp dụng ngay (không chờ hết chu kỳ cũ).
 * Một chủ sở hữu duy nhất (TASK_HMI); không blocking, không gọi từ ISR. */
hal_status_t hal_pwm_set_buzzer(uint32_t freq_hz, uint16_t duty_permille);
#endif
