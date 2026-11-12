/* Harness M1: SPI1 + ILI9341, chay o HSI 8 MHz sau hardware reset.
 * Khong goi BSP stub; khong thay main.c/config/FreeRTOSConfig.h cua repo.
 * SPI tu doc PCLK2 -> SCK 4 MHz. TFT/OLED/game task chua duoc tich hop.
 * LED PC13: ~1 Hz khi dang chay; 10 Hz khi driver bao loi.
 */
#include "FreeRTOS.h"
#include "task.h"
#include "bsp.h"
#include "hal_sys.h"
#include "hal_spi.h"
#include "ili9341.h"
#include "font5x7.h"

#define REG(a) (*(volatile uint32_t *)(a))
#define RCC_APB2ENR REG(0x40021018UL)
#define GPIOC_CRH   REG(0x40011004UL)
#define GPIOC_ODR   REG(0x4001100CUL)
#define GPIOC_BSRR  REG(0x40011010UL)
#define T_MS 5000U  /* timeout huu han, du cho full-screen polling o HSI 8 MHz */

static uint16_t s_line[ILI9341_WIDTH * 2U]; /* 960 B, khong framebuffer */
static volatile uint32_t s_fill_ms, s_blit_ms, s_spi_hz;
static volatile uint32_t s_init_status = HAL_NOT_IMPLEMENTED;
static volatile uint32_t s_io_errors;
static volatile hal_status_t s_last_error = HAL_OK;

static void led_init(void)
{
    RCC_APB2ENR |= (1UL << 4);
    (void)RCC_APB2ENR;
    GPIOC_BSRR = (1UL << 13);
    const uint32_t cs = hal_sys_enter_critical();
    GPIOC_CRH = (GPIOC_CRH & ~(0xFUL << 20)) | (0x2UL << 20);
    hal_sys_exit_critical(cs);
}
static void led_toggle(void) { GPIOC_ODR ^= (1UL << 13); }

static void require_ok(hal_status_t st)
{
    if (st == HAL_OK) { return; }
    s_last_error = st;
    s_io_errors++;
    for (;;) { led_toggle(); vTaskDelay(pdMS_TO_TICKS(50U)); }
}

static uint32_t elapsed_ms(TickType_t start)
{
    return (uint32_t)(xTaskGetTickCount() - start) * portTICK_PERIOD_MS;
}

static char *put_dec(char *p, uint32_t v)
{
    char t[10]; unsigned n = 0;
    do { t[n++] = (char)('0' + v % 10U); v /= 10U; } while (v && n < sizeof t);
    while (n) { *p++ = t[--n]; }
    *p = '\0';
    return p;
}

static void text(uint16_t x, uint16_t y, const char *s, uint16_t fg, uint16_t bg)
{
    for (; *s; s++, x = (uint16_t)(x + 12U)) {
        const uint8_t *g = font5x7_glyph(*s);
        for (unsigned yy = 0; yy < 16U; yy++) {
            for (unsigned xx = 0; xx < 12U; xx++) {
                const unsigned gx = xx / 2U, gy = yy / 2U;
                const int on = gx < FONT5X7_WIDTH && ((g[gx] >> gy) & 1U);
                s_line[yy * 12U + xx] = ILI9341_BE(on ? fg : bg);
            }
        }
        require_ok(ili9341_blit(x, y, 12U, 16U, s_line, T_MS));
    }
}

static void test_task(void *arg)
{
    (void)arg;
    char buf[16];
    s_init_status = (uint32_t)ili9341_init(T_MS);
    require_ok((hal_status_t)s_init_status);
    s_spi_hz = hal_spi_clock_hz();

    TickType_t t0 = xTaskGetTickCount();
    require_ok(ili9341_fill_rect(0, 0, ILI9341_WIDTH, ILI9341_HEIGHT, 0x0000U, T_MS));
    s_fill_ms = elapsed_ms(t0);

    static const uint16_t bars[8] = { 0xFFFF, 0xFFE0, 0x07FF, 0x07E0, 0xF81F, 0xF800, 0x001F, 0x0000 };
    for (unsigned i = 0; i < ILI9341_WIDTH * 2U; i++) {
        s_line[i] = ILI9341_BE(bars[(i % ILI9341_WIDTH) / 30U]);
    }
    t0 = xTaskGetTickCount();
    hal_status_t st = ili9341_begin(0, 0, ILI9341_WIDTH, ILI9341_HEIGHT, T_MS);
    if (st == HAL_OK) {
        for (unsigned y = 0; y < ILI9341_HEIGHT && st == HAL_OK; y += 2U) {
            st = ili9341_push(s_line, sizeof s_line, T_MS);
        }
        ili9341_end(); /* nhả mutex ngay ca khi push loi */
    }
    require_ok(st);
    s_blit_ms = elapsed_ms(t0);

    require_ok(ili9341_fill_rect(0, 0, ILI9341_WIDTH, 1, 0xF800U, T_MS));
    require_ok(ili9341_fill_rect(0, ILI9341_HEIGHT - 1U, ILI9341_WIDTH, 1, 0xF800U, T_MS));
    require_ok(ili9341_fill_rect(0, 0, 1, ILI9341_HEIGHT, 0xF800U, T_MS));
    require_ok(ili9341_fill_rect(ILI9341_WIDTH - 1U, 0, 1, ILI9341_HEIGHT, 0xF800U, T_MS));

    (void)put_dec(buf, s_fill_ms);
    text(8, 260, "FILL", 0xFFFF, 0x0000); text(64, 260, buf, 0xFFE0, 0x0000);
    (void)put_dec(buf, s_blit_ms);
    text(8, 280, "BLIT", 0xFFFF, 0x0000); text(64, 280, buf, 0xFFE0, 0x0000);
    (void)put_dec(buf, s_spi_hz / 1000000U);
    text(8, 300, "SPI MHZ", 0xFFFF, 0x0000); text(100, 300, buf, 0xFFE0, 0x0000);

    uint16_t x = 0;
    TickType_t last = xTaskGetTickCount();
    uint32_t n = 0;
    for (;;) {
        require_ok(ili9341_fill_rect(x, 230, 16, 16, 0x0000U, T_MS));
        x = (uint16_t)((x + 4U) % (ILI9341_WIDTH - 16U));
        require_ok(ili9341_fill_rect(x, 230, 16, 16, 0x07E0U, T_MS));
        if (++n % 25U == 0U) { led_toggle(); }
        vTaskDelayUntil(&last, pdMS_TO_TICKS(20U));
    }
}

int main(void)
{
    /* Sau hardware reset: HSI 8 MHz, khop configCPU_CLOCK_HZ hien tai.
     * Khong chay tiep tai PC cu sau firmware PLL; hay reset board khi nap. */
    led_init();
    if (hal_spi_init() != HAL_OK) { bsp_panic(); }
    if (xTaskCreate(test_task, "TFT", 384U, NULL, 2U, NULL) != pdPASS) { bsp_panic(); }
    vTaskStartScheduler();
    bsp_panic();
}
