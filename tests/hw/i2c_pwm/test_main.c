/*
 * Harness kiểm tra M1 cho nhánh feature/hal-i2c-pwm. KHÔNG phải main.c tích hợp.
 * Chạy ở clock reset mặc định (HSI 8 MHz, khớp configCPU_CLOCK_HZ hiện tại),
 * không gọi bsp_system_init() vì BSP còn là stub.
 *
 * Quan sát trên kit:
 *  - LED PC13 nháy 2 Hz: scheduler chạy, polling I2C không chiếm hết CPU.
 *  - LED nháy nhanh 10 Hz: không tìm thấy thiết bị I2C (kiểm tra dây/nguồn OLED).
 *  - OLED: địa chỉ đo được, thời gian flush toàn màn/1 page, kết quả test NACK,
 *    hiệu ứng đang phát, bộ đếm và thanh HP chạy (chỉ 2 page cập nhật).
 *  - Buzzer: lần lượt SHOOT -> HIT -> GAME_OVER mỗi 1,5 s.
 */
#include "FreeRTOS.h"
#include "task.h"
#include "hal_sys.h"
#include "hal_i2c.h"
#include "hal_pwm.h"
#include "ssd1306.h"
#include "buzzer.h"

#define REG(a) (*(volatile uint32_t *)(a))
#define RCC_APB2ENR REG(0x40021018UL)
#define GPIOC_CRH   REG(0x40011004UL)
#define GPIOC_ODR   REG(0x4001100CUL)

#define I2C_TIMEOUT_MS 20U
#define PROBE_TIMEOUT_MS 5U

static volatile uint32_t s_blink_ms = 250U;   /* debugger có thể đọc các biến này */
static volatile uint8_t s_oled_addr;
static volatile uint32_t s_i2c_errors;
static volatile hal_status_t s_nack_result = HAL_NOT_IMPLEMENTED;

static uint32_t now_ms(void)
{
    return (uint32_t)xTaskGetTickCount() * portTICK_PERIOD_MS;
}

/* Ghi số thập phân/hex vào buffer, không dùng printf (tránh newlib trong RTOS). */
static char *put_str(char *p, const char *s) { while (*s != '\0') { *p++ = *s++; } return p; }
static char *put_dec(char *p, uint32_t v, unsigned width)
{
    char t[10];
    unsigned n = 0;
    do { t[n++] = (char)('0' + v % 10U); v /= 10U; } while (v != 0U && n < sizeof t);
    while (n < width) { t[n++] = '0'; }
    while (n > 0U) { *p++ = t[--n]; }
    return p;
}
static char *put_hex2(char *p, uint8_t v)
{
    static const char h[] = "0123456789ABCDEF";
    *p++ = h[v >> 4];
    *p++ = h[v & 0xFU];
    return p;
}

static void line(uint8_t row, const char *text)
{
    ssd1306_fill_rect(0, (int16_t)(row * 8U), (int16_t)SSD1306_WIDTH, 8, false);
    (void)ssd1306_draw_text(0, (int16_t)(row * 8U), text, 1U);
}

static void flush_or_count(void)
{
    if (ssd1306_flush(I2C_TIMEOUT_MS) != HAL_OK) {
        s_i2c_errors++;
        ssd1306_invalidate();
    }
}

static uint8_t scan_bus(uint8_t *count)
{
    uint8_t first = 0U;
    *count = 0U;
    for (uint8_t a = 0x08U; a <= 0x77U; a++) {
        if (hal_i2c_write(a, NULL, 0U, PROBE_TIMEOUT_MS) == HAL_OK) {
            (*count)++;
            if (first == 0U || a == 0x3CU || a == 0x3DU) { first = a; }
        }
    }
    return first;
}

static void test_task(void *arg)
{
    (void)arg;
    char buf[24];
    char *p;

    vTaskDelay(pdMS_TO_TICKS(150U));          /* OLED cần >= 100 ms sau cấp nguồn */
    const hal_status_t i2c_st = hal_i2c_init();
    const hal_status_t pwm_st = hal_pwm_init();
    buzzer_init();

    uint8_t found = 0U;
    s_oled_addr = (i2c_st == HAL_OK) ? scan_bus(&found) : 0U;
    if (s_oled_addr == 0U) {
        s_blink_ms = 50U;                     /* lỗi: LED nháy nhanh + âm báo nếu PWM ok */
        if (pwm_st == HAL_OK) { (void)buzzer_play(BUZZER_FX_GAME_OVER, now_ms()); }
        for (;;) { (void)buzzer_update(now_ms()); vTaskDelay(pdMS_TO_TICKS(10U)); }
    }

    if (ssd1306_init(s_oled_addr, I2C_TIMEOUT_MS) != HAL_OK) {
        s_blink_ms = 50U;
        for (;;) { vTaskDelay(pdMS_TO_TICKS(1000U)); }
    }

    /* Đo thời gian flush toàn màn (8 page) và 1 page. */
    ssd1306_clear();
    uint32_t t0 = now_ms();
    flush_or_count();
    const uint32_t full_ms = now_ms() - t0;
    line(0, "X");
    t0 = now_ms();
    flush_or_count();
    const uint32_t page_ms = now_ms() - t0;

    /* Test NACK: chọn địa chỉ chắc chắn trống, phải trả IO_ERROR nhanh, không treo. */
    uint8_t absent = 0x50U;
    while (absent == s_oled_addr) { absent++; }
    t0 = now_ms();
    s_nack_result = hal_i2c_write(absent, (const uint8_t *)"\x00", 1U, I2C_TIMEOUT_MS);
    const uint32_t nack_ms = now_ms() - t0;

    line(0, "HAL I2C+PWM TEST");
    p = put_str(buf, "ADDR 0X"); p = put_hex2(p, s_oled_addr);
    p = put_str(p, " DEV "); p = put_dec(p, found, 1U); *p = '\0';
    line(1, buf);
    p = put_str(buf, "PCLK1 "); p = put_dec(p, hal_sys_pclk1_hz() / 1000000UL, 1U);
    p = put_str(p, "MHZ PWM "); p = put_str(p, (pwm_st == HAL_OK) ? "OK" : "ERR"); *p = '\0';
    line(2, buf);
    p = put_str(buf, "FULL "); p = put_dec(p, full_ms, 1U);
    p = put_str(p, "MS PAGE "); p = put_dec(p, page_ms, 1U); p = put_str(p, "MS"); *p = '\0';
    line(3, buf);
    p = put_str(buf, "NACK "); p = put_str(p, (s_nack_result == HAL_IO_ERROR) ? "OK " : "FAIL ");
    p = put_dec(p, nack_ms, 1U); p = put_str(p, "MS"); *p = '\0';
    line(4, buf);
    flush_or_count();

    static const char *const fx_name[BUZZER_FX_COUNT] = { "SHOOT", "HIT", "GAME OVER" };
    TickType_t last = xTaskGetTickCount();
    uint32_t tick10 = 0U;
    uint32_t counter = 0U;
    unsigned fx = 0U;

    for (;;) {
        vTaskDelayUntil(&last, pdMS_TO_TICKS(10U));
        tick10++;
        (void)buzzer_update(now_ms());

        if (tick10 % 150U == 1U) {             /* mỗi 1,5 s đổi hiệu ứng */
            (void)buzzer_play((buzzer_fx_t)fx, now_ms());
            p = put_str(buf, "FX "); p = put_str(p, fx_name[fx]); *p = '\0';
            line(5, buf);
            fx = (fx + 1U) % (unsigned)BUZZER_FX_COUNT;
        }
        if (tick10 % 10U == 0U) {              /* mỗi 100 ms: chỉ page 6, 7 thay đổi */
            counter++;
            p = put_str(buf, "CNT "); p = put_dec(p, counter, 5U);
            p = put_str(p, " ERR "); p = put_dec(p, s_i2c_errors, 1U); *p = '\0';
            line(6, buf);
            const int16_t w = (int16_t)(counter % 21U) * 6;
            ssd1306_fill_rect(0, 57, 126, 6, false);
            ssd1306_fill_rect(0, 57, w, 6, true);
            flush_or_count();
        }
    }
}

static void blink_task(void *arg)
{
    (void)arg;
    RCC_APB2ENR |= (1UL << 4);                                  /* IOPC */
    const uint32_t cs = hal_sys_enter_critical();
    GPIOC_CRH = (GPIOC_CRH & ~(0xFUL << 20)) | (0x2UL << 20);   /* PC13 output 2 MHz */
    hal_sys_exit_critical(cs);
    for (;;) {
        GPIOC_ODR ^= (1UL << 13);
        vTaskDelay(pdMS_TO_TICKS(s_blink_ms));
    }
}

int main(void)
{
    (void)xTaskCreate(test_task, "TEST", 384U, NULL, tskIDLE_PRIORITY + 2U, NULL);
    (void)xTaskCreate(blink_task, "BLINK", 128U, NULL, tskIDLE_PRIORITY + 1U, NULL);
    vTaskStartScheduler();
    for (;;) { }
}
