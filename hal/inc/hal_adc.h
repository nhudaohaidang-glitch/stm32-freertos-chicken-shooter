#ifndef HAL_ADC_H
#define HAL_ADC_H
#include "hal_status.h"
hal_status_t hal_adc_init(void);
/* Reads ADC1 channels 0/1; raw 12-bit values, calibration belongs to HMI. */
hal_status_t hal_adc_read_joystick(uint16_t *x, uint16_t *y, uint32_t timeout_ms);
#endif
