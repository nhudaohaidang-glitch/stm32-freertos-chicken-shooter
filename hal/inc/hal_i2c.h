#ifndef HAL_I2C_H
#define HAL_I2C_H
#include "hal_status.h"
hal_status_t hal_i2c_init(void);
/* addr7 is an unshifted 7-bit address; OLED address must be measured. */
hal_status_t hal_i2c_write(uint8_t addr7, const uint8_t *data, size_t size, uint32_t timeout_ms);
#endif
