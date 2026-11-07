#ifndef HAL_STATUS_H
#define HAL_STATUS_H
#include <stddef.h>
#include <stdint.h>
typedef enum { HAL_OK = 0, HAL_TIMEOUT, HAL_INVALID_ARG, HAL_IO_ERROR, HAL_NOT_IMPLEMENTED } hal_status_t;
/* All timeout_ms values are finite milliseconds (0 = nonblocking).
 * Task-context API only. Any future ISR API must explicitly use _from_isr.
 * Shared bus drivers must serialize access with a FreeRTOS mutex and timeout.
 * Async DMA must document buffer ownership until completion. */
#endif
