#include "hal_pwm.h"
#include "hal_sys.h"
#include <stdbool.h>

#define REG(a) (*(volatile uint32_t *)(a))
#define RCC_APB2ENR  REG(0x40021018UL)
#define RCC_APB1ENR  REG(0x4002101CUL)
#define GPIOB_CRL    REG(0x40010C00UL)
#define TIM3_CR1     REG(0x40000400UL)
#define TIM3_EGR     REG(0x40000414UL)
#define TIM3_CCMR2   REG(0x4000041CUL)
#define TIM3_CCER    REG(0x40000420UL)
#define TIM3_CNT     REG(0x40000424UL)
#define TIM3_PSC     REG(0x40000428UL)
#define TIM3_ARR     REG(0x4000042CUL)
#define TIM3_CCR3    REG(0x4000043CUL)

#define RCC_APB2_IOPB (1UL << 3)
#define RCC_APB1_TIM3 (1UL << 1)
#define CR1_CEN   (1UL << 0)
#define CR1_ARPE  (1UL << 7)
#define EGR_UG    (1UL << 0)
#define CCMR2_OC3PE      (1UL << 3)
#define CCMR2_OC3M_PWM1  (6UL << 4)
#define CCER_CC3E        (1UL << 8)
#define PB0_MASK         (0xFUL << 0)
#define PB0_AF_PP_2MHZ   (0xAUL << 0)  /* MODE=10, CNF=10 */

#define COUNTER_HZ 1000000UL

static bool s_ready;

hal_status_t hal_pwm_init(void)
{
    const uint32_t tim_clk = hal_sys_tim_apb1_hz();
    if (tim_clk < COUNTER_HZ || (tim_clk % COUNTER_HZ) != 0U) { return HAL_IO_ERROR; }

    RCC_APB2ENR |= RCC_APB2_IOPB;
    RCC_APB1ENR |= RCC_APB1_TIM3;
    (void)RCC_APB1ENR;

    TIM3_CR1 = 0U;
    TIM3_PSC = tim_clk / COUNTER_HZ - 1U;  /* 8 MHz -> 7, 72 MHz -> 71 */
    TIM3_ARR = 999U;
    TIM3_CCR3 = 0U;                         /* im lặng: chân giữ mức thấp */
    TIM3_CCMR2 = CCMR2_OC3M_PWM1 | CCMR2_OC3PE;
    TIM3_CCER = CCER_CC3E;
    TIM3_EGR = EGR_UG;                      /* nạp PSC/ARR/CCR từ preload */
    TIM3_CR1 = CR1_ARPE | CR1_CEN;

    const uint32_t cs = hal_sys_enter_critical(); /* CRL dùng chung PB1/PB6/PB7 */
    GPIOB_CRL = (GPIOB_CRL & ~PB0_MASK) | PB0_AF_PP_2MHZ;
    hal_sys_exit_critical(cs);

    s_ready = true;
    return HAL_OK;
}

hal_status_t hal_pwm_set_buzzer(uint32_t freq_hz, uint16_t duty_permille)
{
    if (duty_permille > 1000U) { return HAL_INVALID_ARG; }
    if (freq_hz != 0U && (freq_hz < HAL_PWM_MIN_FREQ_HZ || freq_hz > HAL_PWM_MAX_FREQ_HZ)) {
        return HAL_INVALID_ARG;
    }
    if (!s_ready) { return HAL_IO_ERROR; }

    if (freq_hz == 0U || duty_permille == 0U) {
        TIM3_CCR3 = 0U;
        TIM3_EGR = EGR_UG;
        return HAL_OK;
    }
    const uint32_t period = COUNTER_HZ / freq_hz;          /* 50..50000 tick */
    const uint32_t pulse = (period * duty_permille) / 1000U;
    const uint32_t cs = hal_sys_enter_critical();
    TIM3_ARR = period - 1U;
    TIM3_CCR3 = pulse;
    TIM3_CNT = 0U;
    TIM3_EGR = EGR_UG;                                     /* áp dụng ngay */
    hal_sys_exit_critical(cs);
    return HAL_OK;
}
