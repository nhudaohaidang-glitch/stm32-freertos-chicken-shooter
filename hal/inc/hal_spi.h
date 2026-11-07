#ifndef HAL_SPI_H
#define HAL_SPI_H
#include "hal_status.h"
hal_status_t hal_spi_init(void);
/* SPI1 TX: PA5/PA7; device driver manages CS/DC transaction boundaries. */
hal_status_t hal_spi_write(const uint8_t *data, size_t size, uint32_t timeout_ms);
#endif
