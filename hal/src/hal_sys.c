#include "hal_sys.h"
#include "FreeRTOS.h"
#include "task.h"

#define RCC_CFGR (*(volatile uint32_t *)0x40021004UL)

uint32_t hal_sys_sysclk_hz(void)
{
    const uint32_t cfgr = RCC_CFGR;
    switch ((cfgr >> 2) & 0x3U) {           /* SWS */
    case 1U:
        return HAL_SYS_HSE_HZ;
    case 2U: {                              /* PLL */
        uint32_t mul = ((cfgr >> 18) & 0xFU) + 2U;
        if (mul > 16U) { mul = 16U; }       /* 0b1111 cũng là x16 */
        uint32_t src;
        if ((cfgr & (1UL << 16)) == 0U) {   /* PLLSRC = HSI/2 */
            src = 8000000UL / 2U;
        } else {
            src = HAL_SYS_HSE_HZ;
            if ((cfgr & (1UL << 17)) != 0U) { src /= 2U; } /* PLLXTPRE */
        }
        return src * mul;
    }
    default:
        return 8000000UL;                   /* HSI */
    }
}

uint32_t hal_sys_hclk_hz(void)
{
    static const uint16_t div[8] = { 2U, 4U, 8U, 16U, 64U, 128U, 256U, 512U };
    const uint32_t hpre = (RCC_CFGR >> 4) & 0xFU;
    const uint32_t sys = hal_sys_sysclk_hz();
    return (hpre < 8U) ? sys : sys / div[hpre - 8U];
}

static uint32_t apb_div(uint32_t ppre)
{
    return (ppre < 4U) ? 1U : (2UL << (ppre - 4U));
}

uint32_t hal_sys_pclk1_hz(void)
{
    return hal_sys_hclk_hz() / apb_div((RCC_CFGR >> 8) & 0x7U);
}

uint32_t hal_sys_pclk2_hz(void)
{
    return hal_sys_hclk_hz() / apb_div((RCC_CFGR >> 11) & 0x7U);
}

uint32_t hal_sys_tim_apb1_hz(void)
{
    const uint32_t d = apb_div((RCC_CFGR >> 8) & 0x7U);
    const uint32_t p = hal_sys_hclk_hz() / d;
    return (d == 1U) ? p : 2U * p;
}

uint32_t hal_sys_enter_critical(void)
{
    uint32_t primask;
    __asm volatile ("mrs %0, primask\n cpsid i" : "=r"(primask) :: "memory");
    return primask;
}

void hal_sys_exit_critical(uint32_t state)
{
    __asm volatile ("msr primask, %0" :: "r"(state) : "memory");
}

bool hal_sys_rtos_running(void)
{
    return xTaskGetSchedulerState() == taskSCHEDULER_RUNNING;
}

void hal_deadline_start(hal_deadline_t *d, uint32_t timeout_ms)
{
    d->use_tick = hal_sys_rtos_running();
    if (d->use_tick) {
        d->start = (uint32_t)xTaskGetTickCount();
        d->span = (uint32_t)pdMS_TO_TICKS(timeout_ms) + 1U; /* +1: không hết hạn sớm */
    } else {
        /* Mỗi vòng kiểm tra tốn >= ~10 chu kỳ: ước lượng an toàn, luôn hữu hạn. */
        uint64_t spins = (uint64_t)timeout_ms * (hal_sys_sysclk_hz() / 10000U);
        d->start = 0U;
        d->span = (spins > 0xFFFFFFFFULL) ? 0xFFFFFFFFUL : (uint32_t)spins;
    }
}

bool hal_deadline_expired(hal_deadline_t *d)
{
    if (d->use_tick) {
        return ((uint32_t)xTaskGetTickCount() - d->start) >= d->span;
    }
    if (d->start >= d->span) { return true; }
    d->start++;
    return false;
}

uint32_t hal_deadline_remaining_ticks(const hal_deadline_t *d)
{
    if (!d->use_tick) { return 0U; }
    const uint32_t elapsed = (uint32_t)xTaskGetTickCount() - d->start;
    return (elapsed >= d->span) ? 0U : d->span - elapsed;
}

void hal_sys_delay_us(uint32_t us)
{
    /* Vòng lặp ~4 chu kỳ/lần (Cortex-M3, -Og/-O2). Sai số chấp nhận được cho
     * xung SCL khôi phục bus, nơi chỉ cần "đủ chậm". */
    uint32_t n = (hal_sys_sysclk_hz() / 4000000UL) * us + 1U;
    __asm volatile (
        "1: subs %0, %0, #1\n"
        "   bne 1b\n"
        : "+r"(n) :: "cc");
}
