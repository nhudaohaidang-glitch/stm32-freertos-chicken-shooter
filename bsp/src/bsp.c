#include "bsp.h"

/* ---- Thanh ghi STM32F103 (RM0008) và Cortex-M3 ---- */
#define REG(a) (*(volatile uint32_t *)(a))
#define RCC_CR       REG(0x40021000UL)
#define RCC_CFGR     REG(0x40021004UL)
#define RCC_CIR      REG(0x40021008UL)
#define RCC_APB2ENR  REG(0x40021018UL)
#define FLASH_ACR    REG(0x40022000UL)
#define GPIOC_CRH    REG(0x40011004UL)
#define GPIOC_BSRR   REG(0x40011010UL)
#define GPIOC_ODR    REG(0x4001100CUL)
#define SCB_AIRCR    REG(0xE000ED0CUL)
#define DEMCR        REG(0xE000EDFCUL)
#define DWT_CTRL     REG(0xE0001000UL)
#define DWT_CYCCNT   REG(0xE0001004UL)

#define RCC_CR_HSION    (1UL << 0)
#define RCC_CR_HSIRDY   (1UL << 1)
#define RCC_CR_HSEON    (1UL << 16)
#define RCC_CR_HSERDY   (1UL << 17)
#define RCC_CR_HSEBYP   (1UL << 18)
#define RCC_CR_PLLON    (1UL << 24)
#define RCC_CR_PLLRDY   (1UL << 25)
#define CFGR_SW_MASK    (3UL << 0)
#define CFGR_SW_PLL     (2UL << 0)
#define CFGR_SWS_MASK   (3UL << 2)
#define CFGR_SWS_PLL    (2UL << 2)
#define CFGR_PPRE1_DIV2 (4UL << 8)
#define CFGR_ADCPRE_DIV6 (2UL << 14)
#define CFGR_PLLSRC_HSE (1UL << 16)
#define CFGR_PLLMUL(x)  ((uint32_t)((x) - 2U) << 18)
#define FLASH_ACR_PRFTBE (1UL << 4)
#define FLASH_ACR_LATENCY_2 (2UL << 0)
#define RCC_APB2_IOPC   (1UL << 4)
#define LED_PIN 13U

/* Vòng chờ khởi động thạch anh: ~0x20000 lần đọc (HSE_STARTUP_TIMEOUT của ST là 0x500
 * với vòng phức tạp hơn; giá trị này cho >10 ms ở HSI 8 MHz). */
#define HSE_STARTUP_LOOPS 0x20000UL
#define PLL_LOCK_LOOPS    0x20000UL

uint32_t SystemCoreClock = 8000000UL;
static bsp_clock_src_t s_clock = BSP_CLK_HSI_8MHZ;

static bool wait_set(volatile uint32_t *reg, uint32_t mask, uint32_t value, uint32_t loops)
{
    while ((*reg & mask) != value) {
        if (loops-- == 0U) { return false; }
    }
    return true;
}

static void led_init(void)
{
    RCC_APB2ENR |= RCC_APB2_IOPC;
    (void)RCC_APB2ENR;
    GPIOC_BSRR = (1UL << LED_PIN);                     /* tắt trước khi thành output */
    GPIOC_CRH = (GPIOC_CRH & ~(0xFUL << 20)) | (0x2UL << 20); /* PC13 output PP 2 MHz */
}

static hal_status_t clock_init(void)
{
    bool hse_ok;

    /* Bảo đảm đang chạy HSI trước khi cấu hình lại PLL (an toàn khi debugger reset mềm). */
    RCC_CR |= RCC_CR_HSION;
    (void)wait_set(&RCC_CR, RCC_CR_HSIRDY, RCC_CR_HSIRDY, PLL_LOCK_LOOPS);
    RCC_CFGR &= ~CFGR_SW_MASK;
    (void)wait_set(&RCC_CFGR, CFGR_SWS_MASK, 0U, PLL_LOCK_LOOPS);
    RCC_CR &= ~RCC_CR_PLLON;
    RCC_CIR = 0x009F0000UL;                            /* xóa cờ ngắt RCC, tắt ngắt RCC */

    RCC_CR &= ~RCC_CR_HSEBYP;
    RCC_CR |= RCC_CR_HSEON;
    hse_ok = wait_set(&RCC_CR, RCC_CR_HSERDY, RCC_CR_HSERDY, HSE_STARTUP_LOOPS);

    /* 2 wait state cho 48 < SYSCLK <= 72 MHz, bật prefetch: PHẢI làm trước khi tăng clock. */
    FLASH_ACR = FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY_2;

    uint32_t cfgr = CFGR_PPRE1_DIV2 | CFGR_ADCPRE_DIV6; /* AHB /1, APB2 /1 */
    if (hse_ok) {
        cfgr |= CFGR_PLLSRC_HSE | CFGR_PLLMUL(9U);     /* 8 MHz x 9 = 72 MHz */
    } else {
        RCC_CR &= ~RCC_CR_HSEON;
        cfgr |= CFGR_PLLMUL(16U);                       /* HSI/2 = 4 MHz x 16 = 64 MHz */
    }
    RCC_CFGR = cfgr;

    RCC_CR |= RCC_CR_PLLON;
    if (!wait_set(&RCC_CR, RCC_CR_PLLRDY, RCC_CR_PLLRDY, PLL_LOCK_LOOPS)) {
        return HAL_TIMEOUT;                             /* vẫn HSI 8 MHz, SystemCoreClock giữ nguyên */
    }
    RCC_CFGR = (RCC_CFGR & ~CFGR_SW_MASK) | CFGR_SW_PLL;
    if (!wait_set(&RCC_CFGR, CFGR_SWS_MASK, CFGR_SWS_PLL, PLL_LOCK_LOOPS)) {
        return HAL_TIMEOUT;
    }
    SystemCoreClock = hse_ok ? 72000000UL : 64000000UL;
    s_clock = hse_ok ? BSP_CLK_HSE_PLL_72MHZ : BSP_CLK_HSI_PLL_64MHZ;
    return HAL_OK;
}

hal_status_t bsp_system_init(void)
{
    /* NVIC: PRIGROUP = 3 -> 4 bit priority đều là preempt (port.c kiểm tra bằng configASSERT). */
    SCB_AIRCR = (0x05FAUL << 16) | (3UL << 8);

    led_init();
    hal_status_t st = clock_init();

    /* DWT cycle counter cho đo hiệu năng. */
    DEMCR |= (1UL << 24);                               /* TRCENA */
    DWT_CYCCNT = 0U;
    DWT_CTRL |= 1UL;                                    /* CYCCNTENA */
    return st;
}

bsp_clock_src_t bsp_clock_source(void)
{
    return s_clock;
}

uint32_t bsp_cycles(void)
{
    return DWT_CYCCNT;
}

uint32_t bsp_cycles_to_us(uint32_t cycles)
{
    const uint32_t mhz = SystemCoreClock / 1000000UL;
    return (mhz != 0U) ? cycles / mhz : cycles;
}

void bsp_led_set(bool on)
{
    GPIOC_BSRR = on ? (1UL << (LED_PIN + 16U)) : (1UL << LED_PIN);
}

void bsp_led_toggle(void)
{
    GPIOC_ODR ^= (1UL << LED_PIN);
}

_Noreturn void bsp_panic(void)
{
    __asm volatile ("cpsid i" ::: "memory");
    RCC_APB2ENR |= RCC_APB2_IOPC;
    GPIOC_CRH = (GPIOC_CRH & ~(0xFUL << 20)) | (0x2UL << 20);
    for (;;) {
        GPIOC_ODR ^= (1UL << LED_PIN);                  /* nháy nhanh: hệ thống đã dừng */
        for (volatile uint32_t i = 0U; i < 200000UL; i++) { }
    }
}
