#include "hal_spi.h"
#include "hal_sys.h"
#include "FreeRTOS.h"
#include "semphr.h"

/* ---- Thanh ghi STM32F103 (RM0008) ---- */
#define REG(a) (*(volatile uint32_t *)(a))
#define RCC_APB2ENR  REG(0x40021018UL)
#define GPIOA_CRL    REG(0x40010800UL)
#define SPI1_CR1     REG(0x40013000UL)
#define SPI1_CR2     REG(0x40013004UL)
#define SPI1_SR      REG(0x40013008UL)
#define SPI1_DR      REG(0x4001300CUL)

#define RCC_APB2_IOPA  (1UL << 2)
#define RCC_APB2_SPI1  (1UL << 12)
#define CR1_MSTR    (1UL << 2)
#define CR1_BR_SHIFT 3U
#define CR1_SPE     (1UL << 6)
#define CR1_SSI     (1UL << 8)
#define CR1_SSM     (1UL << 9)
#define CR1_BIDIOE  (1UL << 14)
#define CR1_BIDIMODE (1UL << 15)
#define SR_TXE      (1UL << 1)
#define SR_BSY      (1UL << 7)

/* PA5: bit 23:20, PA7: bit 31:28 trong CRL. AF push-pull 50 MHz = 0xB. */
#define PA5_PA7_MASK ((0xFUL << 20) | (0xFUL << 28))
#define PA5_PA7_AF   ((0xBUL << 20) | (0xBUL << 28))

static SemaphoreHandle_t s_mutex;
static bool s_ready;
static uint32_t s_clock_hz;

hal_status_t hal_spi_init(void)
{
    if (s_mutex == NULL) {
        s_mutex = xSemaphoreCreateMutex();
        if (s_mutex == NULL) { return HAL_IO_ERROR; }
    }

    RCC_APB2ENR |= RCC_APB2_IOPA | RCC_APB2_SPI1;
    (void)RCC_APB2ENR;

    const uint32_t cs = hal_sys_enter_critical();   /* CRL dùng chung PA0/PA1 (ADC), PA3/PA4 */
    GPIOA_CRL = (GPIOA_CRL & ~PA5_PA7_MASK) | PA5_PA7_AF;
    hal_sys_exit_critical(cs);

    /* Chọn bộ chia nhỏ nhất (2,4,...,256) để SCK <= HAL_SPI_MAX_HZ. */
    const uint32_t pclk2 = hal_sys_pclk2_hz();
    uint32_t br = 0U;
    while (br < 7U && (pclk2 >> (br + 1U)) > HAL_SPI_MAX_HZ) { br++; }
    s_clock_hz = pclk2 >> (br + 1U);

    SPI1_CR1 = 0U;
    SPI1_CR2 = 0U;
    /* Master, mode 0, 8 bit, MSB trước, NSS phần mềm, chỉ truyền một dây. */
    SPI1_CR1 = CR1_BIDIMODE | CR1_BIDIOE | CR1_SSM | CR1_SSI | CR1_MSTR | (br << CR1_BR_SHIFT);
    SPI1_CR1 |= CR1_SPE;

    s_ready = true;
    return HAL_OK;
}

uint32_t hal_spi_clock_hz(void)
{
    return s_ready ? s_clock_hz : 0U;
}

hal_status_t hal_spi_lock(uint32_t timeout_ms)
{
    if (!s_ready) { return HAL_IO_ERROR; }
    if (!hal_sys_rtos_running()) { return HAL_OK; }
    uint64_t ticks = ((uint64_t)timeout_ms * configTICK_RATE_HZ) / 1000U;
    if (timeout_ms != 0U) { ticks++; } /* không hết hạn sớm do biên tick */
    if (ticks >= (uint64_t)portMAX_DELAY) { ticks = (uint64_t)portMAX_DELAY - 1U; }
    return (xSemaphoreTake(s_mutex, (TickType_t)ticks) == pdTRUE) ? HAL_OK : HAL_TIMEOUT;
}

void hal_spi_unlock(void)
{
    if (s_ready && hal_sys_rtos_running()) { (void)xSemaphoreGive(s_mutex); }
}

/* Chờ byte cuối ra khỏi shift register: TXE=1 rồi BSY=0. */
static hal_status_t wait_idle(hal_deadline_t *dl)
{
    while ((SPI1_SR & SR_TXE) == 0U) {
        if (hal_deadline_expired(dl)) { return HAL_TIMEOUT; }
    }
    while ((SPI1_SR & SR_BSY) != 0U) {
        if (hal_deadline_expired(dl)) { return HAL_TIMEOUT; }
    }
    return HAL_OK;
}

static inline hal_status_t put(uint8_t b, hal_deadline_t *dl)
{
    /* Kiểm tra cả khi TXE luôn sẵn sàng, không chỉ lúc phải chờ cờ. */
    if (hal_deadline_expired(dl)) { return HAL_TIMEOUT; }
    while ((SPI1_SR & SR_TXE) == 0U) {
        if (hal_deadline_expired(dl)) { return HAL_TIMEOUT; }
    }
    *(volatile uint8_t *)&SPI1_DR = b;
    return HAL_OK;
}

hal_status_t hal_spi_write(const uint8_t *data, size_t size, uint32_t timeout_ms)
{
    if (data == NULL && size > 0U) { return HAL_INVALID_ARG; }
    if (!s_ready) { return HAL_IO_ERROR; }
    if (timeout_ms == 0U) {
        /* Không khởi động giao dịch đồng bộ khi caller không cho phép chờ. */
        return (size == 0U && (SPI1_SR & (SR_TXE | SR_BSY)) == SR_TXE) ? HAL_OK : HAL_TIMEOUT;
    }
    hal_deadline_t dl;
    hal_deadline_start(&dl, timeout_ms);
    for (size_t i = 0; i < size; i++) {
        if (put(data[i], &dl) != HAL_OK) { return HAL_TIMEOUT; }
    }
    return wait_idle(&dl);
}

hal_status_t hal_spi_write_repeat16(uint16_t value, uint32_t count, uint32_t timeout_ms)
{
    if (!s_ready) { return HAL_IO_ERROR; }
    if (timeout_ms == 0U) {
        return (count == 0U && (SPI1_SR & (SR_TXE | SR_BSY)) == SR_TXE) ? HAL_OK : HAL_TIMEOUT;
    }
    const uint8_t hi = (uint8_t)(value >> 8);
    const uint8_t lo = (uint8_t)value;
    hal_deadline_t dl;
    hal_deadline_start(&dl, timeout_ms);
    while (count-- > 0U) {
        if (put(hi, &dl) != HAL_OK || put(lo, &dl) != HAL_OK) { return HAL_TIMEOUT; }
    }
    return wait_idle(&dl);
}
