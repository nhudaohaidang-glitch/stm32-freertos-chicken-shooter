#define _GNU_SOURCE
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include "FreeRTOS.h"
#include "semphr.h"
#include "hal_sys.h"
#include "hal_spi.h"
#include "ili9341.h"

#define REG(a) (*(volatile uint32_t *)(a))
#define SPI_SR REG(0x40013008UL)
#define SPI_DR REG(0x4001300CUL)
#define GPIOA_BSRR REG(0x40010810UL)
#define GPIOA_CRL REG(0x40010800UL)
#define TXE (1U << 1)
#define BSY (1U << 7)
#define CHECK(expr) do { assert(expr); checks++; } while (0)
static unsigned checks, takes, gives;
static bool available = true, held;
static uint32_t now, pclk2 = 8000000U;
static TickType_t last_wait;

SemaphoreHandle_t xSemaphoreCreateMutex(void) { return (void *)1; }
BaseType_t xSemaphoreTake(SemaphoreHandle_t mutex, TickType_t ticks)
{
    (void)mutex; takes++; last_wait = ticks;
    if (!available || held) { return 0; }
    held = true;
    return pdTRUE;
}
BaseType_t xSemaphoreGive(SemaphoreHandle_t mutex)
{
    (void)mutex; assert(held); held = false; gives++; return pdTRUE;
}
uint32_t hal_sys_pclk2_hz(void) { return pclk2; }
uint32_t hal_sys_enter_critical(void) { return 0; }
void hal_sys_exit_critical(uint32_t state) { (void)state; }
bool hal_sys_rtos_running(void) { return true; }
void hal_sys_delay_us(uint32_t us) { (void)us; }
void vTaskDelay(TickType_t ticks) { now += ticks; }
void hal_deadline_start(hal_deadline_t *d, uint32_t ms)
{
    d->use_tick = true; d->start = now; d->span = ms;
}
bool hal_deadline_expired(hal_deadline_t *d)
{
    /* Advance virtual time to terminate injected stuck-flag waits. */
    now++;
    return (uint32_t)(now - d->start) >= d->span;
}

int main(void)
{
    void *mem = mmap((void *)0x40000000UL, 0x30000U, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
    if (mem == MAP_FAILED) { perror("reserve test MMIO"); return 2; }
    const uint8_t data[] = { 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0 };
    CHECK(hal_spi_write(data, 1U, 10U) == HAL_IO_ERROR);
    CHECK(hal_spi_clock_hz() == 0U);
    GPIOA_CRL = 0x44444444UL;
    CHECK(hal_spi_init() == HAL_OK);
    CHECK(hal_spi_clock_hz() == 4000000U);
    CHECK((GPIOA_CRL & 0x000FFFFFUL) == 0x00044444UL); /* PA0..PA4 preserved */
    pclk2 = 72000000U;
    CHECK(hal_spi_init() == HAL_OK);
    CHECK(hal_spi_clock_hz() == 18000000U);
    SPI_SR = TXE;

    available = false;
    CHECK(hal_spi_lock(0U) == HAL_TIMEOUT);
    CHECK(last_wait == 0U);
    CHECK(hal_spi_lock(UINT32_MAX) == HAL_TIMEOUT);
    CHECK(last_wait != portMAX_DELAY); /* no accidental infinite mutex wait */
    available = true;
    CHECK(hal_spi_lock(0U) == HAL_OK);
    CHECK(held);
    hal_spi_unlock();
    CHECK(!held);

    SPI_DR = 0xA5U;
    uint32_t t0 = now;
    CHECK(hal_spi_write(data, sizeof data, 0U) == HAL_TIMEOUT);
    CHECK(SPI_DR == 0xA5U && now == t0);
    CHECK(hal_spi_write_repeat16(0xF800U, 1U, 0U) == HAL_TIMEOUT);
    CHECK(SPI_DR == 0xA5U && now == t0);
    CHECK(hal_spi_write(NULL, 0U, 0U) == HAL_OK);
    CHECK(hal_spi_write(NULL, 1U, 10U) == HAL_INVALID_ARG);
    SPI_SR = TXE | BSY;
    CHECK(hal_spi_write(NULL, 0U, 0U) == HAL_TIMEOUT);
    SPI_SR = 0U;
    CHECK(hal_spi_write(data, 1U, 4U) == HAL_TIMEOUT); /* TXE stuck */
    SPI_SR = TXE | BSY;
    CHECK(hal_spi_write(data, 1U, 4U) == HAL_TIMEOUT); /* BSY stuck */
    SPI_SR = TXE;
    SPI_DR = 0xA5U;
    CHECK(hal_spi_write(data, sizeof data, 2U) == HAL_TIMEOUT);
    CHECK(SPI_DR == data[0]); /* deadline checked even with TXE always high */
    CHECK(hal_spi_write(data, 3U, 20U) == HAL_OK);
    CHECK(SPI_DR == data[2]);

    CHECK(ili9341_push(data, 2U, 10U) == HAL_IO_ERROR);
    GPIOA_BSRR = 0xDEADU;
    t0 = now;
    CHECK(ili9341_init(0U) == HAL_TIMEOUT);
    CHECK(GPIOA_BSRR == 0xDEADU && now == t0);
    CHECK(ili9341_init(1000U) == HAL_OK);
    CHECK(!held);
    unsigned before = takes;
    GPIOA_BSRR = 0xDEADU;
    CHECK(ili9341_begin(0U, 0U, 2U, 2U, 0U) == HAL_TIMEOUT);
    CHECK(takes == before && GPIOA_BSRR == 0xDEADU && !held);
    CHECK(ili9341_begin(240U, 0U, 1U, 1U, 100U) == HAL_INVALID_ARG);
    available = false;
    CHECK(ili9341_begin(0U, 0U, 2U, 2U, 100U) == HAL_TIMEOUT);
    CHECK(!held);
    available = true;
    SPI_SR = TXE | BSY;
    CHECK(ili9341_begin(0U, 0U, 2U, 2U, 4U) == HAL_TIMEOUT);
    CHECK(!held && GPIOA_BSRR == (1U << 4)); /* begin error releases CS/lock */
    SPI_SR = TXE;
    CHECK(ili9341_begin(0U, 0U, 2U, 2U, 100U) == HAL_OK);
    CHECK(held);
    CHECK(ili9341_begin(0U, 0U, 1U, 1U, 100U) == HAL_IO_ERROR);
    CHECK(ili9341_push(data, sizeof data, 100U) == HAL_OK);
    unsigned old_gives = gives;
    ili9341_end();
    CHECK(!held && gives == old_gives + 1U);
    ili9341_end();
    CHECK(gives == old_gives + 1U);
    CHECK(ili9341_begin(239U, 319U, 5U, 5U, 100U) == HAL_OK); /* clipped */
    ili9341_end();
    CHECK(ili9341_blit(239U, 319U, 2U, 2U, data, 100U) == HAL_INVALID_ARG);
    CHECK(ili9341_blit(0U, 0U, 2U, 2U, data, 100U) == HAL_OK);
    CHECK(!held);
    CHECK(ili9341_fill_rect(0U, 0U, 2U, 2U, 0xF800U, 100U) == HAL_OK);
    CHECK(!held);
    CHECK(ILI9341_BE(0xF800U) == 0x00F8U);
    CHECK(munmap(mem, 0x30000U) == 0);
    printf("SPI/TFT host control-flow tests: %u checks passed (not a kit test)\n", checks);
    return 0;
}
