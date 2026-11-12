#ifndef BSP_H
#define BSP_H
#include <stdbool.h>
#include <stdint.h>
#include "hal_status.h"
/* Leader owns BSP, startup, clock and FreeRTOSConfig.h. */

/* Tần số lõi hiện tại (Hz). FreeRTOSConfig.h dùng biến này làm configCPU_CLOCK_HZ.
 * Giá trị sau reset là 8 MHz (HSI); bsp_system_init() cập nhật sau khi đổi clock. */
extern uint32_t SystemCoreClock;

typedef enum { BSP_CLK_HSI_8MHZ = 0, BSP_CLK_HSE_PLL_72MHZ, BSP_CLK_HSI_PLL_64MHZ } bsp_clock_src_t;

/* Gọi đầu tiên trong main(), trước mọi hal_*_init().
 * - HSE 8 MHz x9 -> SYSCLK 72 MHz, AHB 72, APB1 36, APB2 72, ADC 12 MHz, Flash 2 WS.
 * - Nếu thạch anh không khởi động trong thời gian giới hạn: HSI/2 x16 = 64 MHz.
 * - NVIC: 4 bit preempt (PRIGROUP 3) theo yêu cầu port FreeRTOS Cortex-M3.
 * - Bật DWT CYCCNT để đo thời gian (bsp_cycles). LED PC13 làm đèn trạng thái.
 * Trả HAL_TIMEOUT nếu PLL không khóa (khi đó vẫn chạy HSI 8 MHz). */
hal_status_t bsp_system_init(void);
bsp_clock_src_t bsp_clock_source(void);

/* Bộ đếm chu kỳ CPU 32 bit (tràn sau ~59 s ở 72 MHz). Dùng đo thời gian render/FPS. */
uint32_t bsp_cycles(void);
uint32_t bsp_cycles_to_us(uint32_t cycles);

/* LED PC13 trên Blue Pill (tích cực mức thấp). */
void bsp_led_set(bool on);
void bsp_led_toggle(void);

/* Dừng hệ thống an toàn: tắt ngắt, LED nháy nhanh. Dùng cho assert/hook lỗi. */
_Noreturn void bsp_panic(void);
#endif
