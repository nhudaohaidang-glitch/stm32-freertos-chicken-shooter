#include "hal_i2c.h"
#include "hal_sys.h"
#include "FreeRTOS.h"
#include "semphr.h"

/* ---- Thanh ghi STM32F103 (RM0008) ---- */
#define REG(a) (*(volatile uint32_t *)(a))
#define RCC_APB2ENR   REG(0x40021018UL)
#define RCC_APB1ENR   REG(0x4002101CUL)
#define RCC_APB1RSTR  REG(0x40021010UL)
#define GPIOB_CRL     REG(0x40010C00UL)
#define GPIOB_IDR     REG(0x40010C08UL)
#define GPIOB_BSRR    REG(0x40010C10UL)
#define I2C1_CR1      REG(0x40005400UL)
#define I2C1_CR2      REG(0x40005404UL)
#define I2C1_DR       REG(0x40005410UL)
#define I2C1_SR1      REG(0x40005414UL)
#define I2C1_SR2      REG(0x40005418UL)
#define I2C1_CCR      REG(0x4000541CUL)
#define I2C1_TRISE    REG(0x40005420UL)

#define RCC_APB2_IOPB  (1UL << 3)
#define RCC_APB1_I2C1  (1UL << 21)
#define CR1_PE     (1UL << 0)
#define CR1_START  (1UL << 8)
#define CR1_STOP   (1UL << 9)
#define CR1_SWRST  (1UL << 15)
#define SR1_SB     (1UL << 0)
#define SR1_ADDR   (1UL << 1)
#define SR1_BTF    (1UL << 2)
#define SR1_TXE    (1UL << 7)
#define SR1_BERR   (1UL << 8)
#define SR1_ARLO   (1UL << 9)
#define SR1_AF     (1UL << 10)
#define SR1_ERRORS (SR1_BERR | SR1_ARLO | SR1_AF)
#define SR2_BUSY   (1UL << 1)

#define PIN_SCL 6U
#define PIN_SDA 7U
#define CRL_SHIFT(pin) ((pin) * 4U)
#define MODE_OUT_OD_2MHZ  0x6U  /* MODE=10, CNF=01 */
#define MODE_AF_OD_50MHZ  0xFU  /* MODE=11, CNF=11 */

static SemaphoreHandle_t s_mutex;
static bool s_ready;

static void pins_mode(uint32_t mode)
{
    const uint32_t mask = (0xFUL << CRL_SHIFT(PIN_SCL)) | (0xFUL << CRL_SHIFT(PIN_SDA));
    const uint32_t val = (mode << CRL_SHIFT(PIN_SCL)) | (mode << CRL_SHIFT(PIN_SDA));
    const uint32_t cs = hal_sys_enter_critical(); /* CRL dùng chung với PB0/PB1 */
    GPIOB_CRL = (GPIOB_CRL & ~mask) | val;
    hal_sys_exit_critical(cs);
}

static void line(uint32_t pin, bool high)
{
    GPIOB_BSRR = high ? (1UL << pin) : (1UL << (pin + 16U));
}

/* Slave bị reset giữa chừng có thể giữ SDA thấp: phát tối đa 9 xung SCL
 * cho tới khi SDA nhả, rồi tạo STOP thủ công. */
static void bus_recover(void)
{
    line(PIN_SCL, true);
    line(PIN_SDA, true);
    pins_mode(MODE_OUT_OD_2MHZ);
    hal_sys_delay_us(10U);
    for (int i = 0; i < 9 && (GPIOB_IDR & (1UL << PIN_SDA)) == 0U; i++) {
        line(PIN_SCL, false);
        hal_sys_delay_us(5U);
        line(PIN_SCL, true);
        hal_sys_delay_us(5U);
    }
    line(PIN_SDA, false);   /* STOP: SDA lên khi SCL đang cao */
    hal_sys_delay_us(5U);
    line(PIN_SCL, true);
    hal_sys_delay_us(5U);
    line(PIN_SDA, true);
    hal_sys_delay_us(5U);
    pins_mode(MODE_AF_OD_50MHZ);
}

static hal_status_t hw_setup(void)
{
    const uint32_t pclk1 = hal_sys_pclk1_hz();
    const uint32_t mhz = pclk1 / 1000000UL;
    if (mhz < 2U || mhz > 36U) { return HAL_IO_ERROR; }
    if (HAL_I2C_CLOCK_HZ > 100000UL && mhz < 4U) { return HAL_IO_ERROR; }

    bus_recover();

    /* Reset toàn bộ khối I2C1 (cũng gỡ lỗi BUSY kẹt trong errata F1). */
    RCC_APB1RSTR |= RCC_APB1_I2C1;
    RCC_APB1RSTR &= ~RCC_APB1_I2C1;
    I2C1_CR1 = CR1_SWRST;
    I2C1_CR1 = 0U;

    I2C1_CR2 = mhz;
    uint32_t ccr;
    if (HAL_I2C_CLOCK_HZ <= 100000UL) {
        ccr = pclk1 / (2U * HAL_I2C_CLOCK_HZ);            /* Tlow = Thigh */
        if (ccr < 4U) { ccr = 4U; }
        I2C1_TRISE = mhz + 1U;                             /* 1000 ns */
    } else {
        ccr = (pclk1 + 3U * HAL_I2C_CLOCK_HZ - 1U) / (3U * HAL_I2C_CLOCK_HZ); /* Tlow = 2 Thigh, làm tròn lên */
        if (ccr < 1U) { ccr = 1U; }
        ccr |= (1UL << 15);                                /* F/S */
        I2C1_TRISE = (mhz * 300U) / 1000U + 1U;            /* 300 ns */
    }
    I2C1_CCR = ccr;
    I2C1_CR1 = CR1_PE;
    return HAL_OK;
}

hal_status_t hal_i2c_init(void)
{
    RCC_APB2ENR |= RCC_APB2_IOPB;
    RCC_APB1ENR |= RCC_APB1_I2C1;
    (void)RCC_APB1ENR; /* đọc lại: chờ clock ổn định trước khi truy cập */

    if (s_mutex == NULL) {
        s_mutex = xSemaphoreCreateMutex();
        if (s_mutex == NULL) { return HAL_IO_ERROR; }
    }
    hal_status_t st = hw_setup();
    s_ready = (st == HAL_OK);
    return st;
}

/* Chờ cờ trong SR1; báo lỗi sớm nếu NACK/arbitration/bus error. */
static hal_status_t wait_sr1(uint32_t flag, hal_deadline_t *dl)
{
    for (;;) {
        const uint32_t sr1 = I2C1_SR1;
        if ((sr1 & SR1_ERRORS) != 0U) { return HAL_IO_ERROR; }
        if ((sr1 & flag) != 0U) { return HAL_OK; }
        if (hal_deadline_expired(dl)) { return HAL_TIMEOUT; }
    }
}

static hal_status_t transfer(uint8_t addr7, const uint8_t *data, size_t size, hal_deadline_t *dl)
{
    while ((I2C1_SR2 & SR2_BUSY) != 0U) {
        if (hal_deadline_expired(dl)) { return HAL_TIMEOUT; }
    }
    I2C1_CR1 |= CR1_START;
    hal_status_t st = wait_sr1(SR1_SB, dl);
    if (st != HAL_OK) { return st; }

    I2C1_DR = (uint32_t)addr7 << 1;           /* bit0 = 0: ghi */
    st = wait_sr1(SR1_ADDR, dl);
    if (st != HAL_OK) { return st; }
    (void)I2C1_SR1;                           /* xóa ADDR: đọc SR1 rồi SR2 */
    (void)I2C1_SR2;

    for (size_t i = 0; i < size; i++) {
        st = wait_sr1(SR1_TXE, dl);
        if (st != HAL_OK) { return st; }
        I2C1_DR = data[i];
    }
    if (size > 0U) {
        st = wait_sr1(SR1_BTF, dl);           /* byte cuối đã ra khỏi shift register */
        if (st != HAL_OK) { return st; }
    }
    I2C1_CR1 |= CR1_STOP;
    while ((I2C1_CR1 & CR1_STOP) != 0U) {     /* phần cứng xóa STOP khi đã phát */
        if (hal_deadline_expired(dl)) { return HAL_TIMEOUT; }
    }
    return HAL_OK;
}

hal_status_t hal_i2c_write(uint8_t addr7, const uint8_t *data, size_t size, uint32_t timeout_ms)
{
    if (addr7 > 0x7FU || (data == NULL && size > 0U) || timeout_ms == 0U) {
        return HAL_INVALID_ARG;
    }
    if (!s_ready) { return HAL_IO_ERROR; }

    hal_deadline_t dl;
    hal_deadline_start(&dl, timeout_ms);
    const bool rtos = hal_sys_rtos_running();
    if (rtos && xSemaphoreTake(s_mutex, (TickType_t)hal_deadline_remaining_ticks(&dl)) != pdTRUE) {
        return HAL_TIMEOUT;
    }

    hal_status_t st = transfer(addr7, data, size, &dl);
    const uint32_t sr1 = I2C1_SR1;
    if (st == HAL_IO_ERROR && (sr1 & (SR1_BERR | SR1_ARLO)) == 0U) {
        /* NACK: xóa AF, nhả bus. Đây là lỗi "bình thường" (sai địa chỉ, probe). */
        I2C1_SR1 = ~SR1_AF & 0xFFFFUL;
        I2C1_CR1 |= CR1_STOP;
        hal_deadline_t stop_dl;
        hal_deadline_start(&stop_dl, 2U);
        while ((I2C1_CR1 & CR1_STOP) != 0U && !hal_deadline_expired(&stop_dl)) { }
    } else if (st != HAL_OK) {
        /* Bus error / arbitration / timeout: reset cứng để task sau không bị treo. */
        s_ready = (hw_setup() == HAL_OK);
    }

    if (rtos) { (void)xSemaphoreGive(s_mutex); }
    return st;
}
