#ifndef SPI_TEST_FREERTOS_H
#define SPI_TEST_FREERTOS_H
#include <stddef.h>
#include <stdint.h>
typedef uint32_t TickType_t;
typedef int BaseType_t;
#define configTICK_RATE_HZ 1000U
#define portMAX_DELAY UINT32_MAX
#define pdTRUE 1
#define pdMS_TO_TICKS(ms) ((TickType_t)(ms))
#endif
