#ifndef HAL_UART_H
#define HAL_UART_H
#include "hal_status.h"
hal_status_t hal_uart_init(void);
hal_status_t hal_uart_write(const uint8_t *data, size_t size, uint32_t timeout_ms);
#endif
