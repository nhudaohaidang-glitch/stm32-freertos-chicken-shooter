#ifndef HAL_SYS_H
#define HAL_SYS_H
#include <stdbool.h>
#include <stdint.h>
/*
 * Tiện ích nền cho HAL: đọc tần số clock thực từ RCC, critical section
 * và deadline cho timeout. Đọc clock từ thanh ghi nên driver chạy đúng cả
 * khi BSP còn để HSI 8 MHz mặc định lẫn khi leader chuyển sang PLL 72 MHz.
 */
#ifndef HAL_SYS_HSE_HZ
#define HAL_SYS_HSE_HZ 8000000UL /* Thạch anh Blue Pill; override bằng -D nếu khác */
#endif

uint32_t hal_sys_sysclk_hz(void);
uint32_t hal_sys_hclk_hz(void);
uint32_t hal_sys_pclk1_hz(void);
uint32_t hal_sys_pclk2_hz(void);
/* Clock vào TIM2..TIM4: PCLK1 nếu APB1 prescaler = 1, ngược lại 2 × PCLK1. */
uint32_t hal_sys_tim_apb1_hz(void);

/* Critical section ngắn (vài lệnh RMW thanh ghi dùng chung, ví dụ GPIOx_CRL).
 * Dùng được trước và sau khi scheduler chạy. Không gọi từ ISR. */
uint32_t hal_sys_enter_critical(void);
void hal_sys_exit_critical(uint32_t state);

/* true khi scheduler đang chạy và có thể dùng tick, mutex, vTaskDelay. */
bool hal_sys_rtos_running(void);

/* Deadline tính theo tick khi RTOS chạy; trước scheduler thì đếm vòng lặp
 * (xấp xỉ, luôn hữu hạn). Thời gian chờ thực tế >= timeout_ms. */
typedef struct {
    bool use_tick;
    uint32_t start;
    uint32_t span;
} hal_deadline_t;

void hal_deadline_start(hal_deadline_t *d, uint32_t timeout_ms);
bool hal_deadline_expired(hal_deadline_t *d);
/* Số tick còn lại (0 nếu hết hạn hoặc chưa có RTOS) để truyền cho xSemaphoreTake. */
uint32_t hal_deadline_remaining_ticks(const hal_deadline_t *d);

/* Chờ bận ~us micro giây. Chỉ dùng cho bit-bang ngắn (khôi phục bus I2C),
 * không dùng thay vTaskDelay. */
void hal_sys_delay_us(uint32_t us);
#endif
