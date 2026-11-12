/*
 * Chương trình test driver ADC + GPIO trên kit. KHÔNG phải main.c của game.
 * Chạy ở clock 8 MHz (HSI), đúng configCPU_CLOCK_HZ hiện tại.
 *
 * Xem kết quả:
 *  - LED PC13 trên Blue Pill:
 *      giữ Fire          -> LED sáng liên tục
 *      giữ Start/Pause   -> LED tắt liên tục
 *      joystick ở giữa   -> nháy chậm (1 Hz)
 *      joystick đẩy lệch -> nháy nhanh (5 Hz)
 *      init lỗi          -> nháy rất nhanh (10 Hz)
 *  - Biến g_test: đọc bằng gdb (print g_test).
 */
#include "FreeRTOS.h"
#include "task.h"
#include "hal_adc.h"
#include "hal_gpio.h"

#define RCC_APB2ENR (*(volatile uint32_t *)0x40021018UL)
#define GPIOC_CRH   (*(volatile uint32_t *)0x40011004UL)
#define GPIOC_ODR   (*(volatile uint32_t *)0x4001100CUL)

#define PERIOD_MS      10U    /* đọc 100 lần mỗi giây */
#define ADC_TIMEOUT_MS 2U
#define FAR_FROM_MID   600U   /* chỉ dùng để đổi tốc độ nháy LED */

typedef struct {
    hal_status_t adc_init;     /* mong đợi 0 (HAL_OK) */
    hal_status_t gpio_init;    /* mong đợi 0 */
    uint16_t x;                /* giá trị mới nhất 0..4095 */
    uint16_t y;
    uint16_t x_min, x_max;     /* xoay joystick hết các hướng để xem dải */
    uint16_t y_min, y_max;
    uint8_t  buttons;          /* bit0 Fire, bit1 Start/Pause */
    uint32_t fire_count;       /* số lần nhấn Fire (theo ngắt) */
    uint32_t start_count;      /* số lần nhấn Start/Pause (theo ngắt) */
    uint32_t read_errors;      /* mong đợi 0 */
    uint32_t wakeups;          /* số lần ngắt nút đánh thức task (FromISR) */
} test_result_t;

volatile test_result_t g_test;

static void led_init(void)
{
    RCC_APB2ENR |= (1UL << 4);            /* bật clock GPIOC */
    GPIOC_CRH &= ~(0xFUL << 20);          /* PC13: bit 23:20 */
    GPIOC_CRH |=  (0x2UL << 20);          /* output push-pull 2 MHz */
    GPIOC_ODR |=  (1UL << 13);            /* LED trên Blue Pill: 1 = tắt */
}

static void led_on(void)     { GPIOC_ODR &= ~(1UL << 13); }
static void led_off(void)    { GPIOC_ODR |=  (1UL << 13); }
static void led_toggle(void) { GPIOC_ODR ^=  (1UL << 13); }

static uint16_t distance_from_mid(uint16_t v)
{
    return (v > 2048U) ? (uint16_t)(v - 2048U) : (uint16_t)(2048U - v);
}

static void test_task(void *arg)
{
    uint32_t blink_timer = 0U;
    uint32_t blink_period = 500U;          /* ms */
    uint16_t x = 2048U;
    uint16_t y = 2048U;
    uint8_t buttons = 0U;
    uint8_t edges;

    (void)arg;
    (void)hal_gpio_exti_enable(xTaskGetCurrentTaskHandle());

    for (;;) {
        /* Chờ tối đa 10 ms; nhấn nút thì ngắt đánh thức task sớm hơn. */
        if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(PERIOD_MS)) > 0U) {
            g_test.wakeups++;
        }

        /* 1. Đọc joystick */
        if (hal_adc_read_joystick(&x, &y, ADC_TIMEOUT_MS) == HAL_OK) {
            g_test.x = x;
            g_test.y = y;
            if (x < g_test.x_min) { g_test.x_min = x; }
            if (x > g_test.x_max) { g_test.x_max = x; }
            if (y < g_test.y_min) { g_test.y_min = y; }
            if (y > g_test.y_max) { g_test.y_max = y; }
        } else {
            g_test.read_errors++;
        }

        /* 2. Đọc nút (mức tức thời) và các lần nhấn ghi bởi ngắt */
        (void)hal_gpio_read_buttons(&buttons);
        g_test.buttons = buttons;
        edges = hal_gpio_take_edges();
        if ((edges & HAL_GPIO_BTN_FIRE) != 0U)        { g_test.fire_count++; }
        if ((edges & HAL_GPIO_BTN_START_PAUSE) != 0U) { g_test.start_count++; }

        /* 3. Điều khiển LED */
        if ((buttons & HAL_GPIO_BTN_FIRE) != 0U) {
            led_on();
        } else if ((buttons & HAL_GPIO_BTN_START_PAUSE) != 0U) {
            led_off();
        } else {
            if (distance_from_mid(x) > FAR_FROM_MID || distance_from_mid(y) > FAR_FROM_MID) {
                blink_period = 100U;
            } else {
                blink_period = 500U;
            }
            blink_timer += PERIOD_MS;   /* xấp xỉ: lần bị đánh thức sớm tính như 10 ms */
            if (blink_timer >= blink_period) {
                blink_timer = 0U;
                led_toggle();
            }
        }
    }
}

static void error_task(void *arg)
{
    (void)arg;
    for (;;) {
        led_toggle();
        vTaskDelay(pdMS_TO_TICKS(50U));
    }
}

int main(void)
{
    BaseType_t ok;

    led_init();
    g_test.x_min = HAL_ADC_MAX_VALUE;
    g_test.y_min = HAL_ADC_MAX_VALUE;

    g_test.adc_init = hal_adc_init();
    g_test.gpio_init = hal_gpio_init();

    if (g_test.adc_init == HAL_OK && g_test.gpio_init == HAL_OK) {
        ok = xTaskCreate(test_task, "TEST", 256U, NULL, 2U, NULL);
    } else {
        ok = xTaskCreate(error_task, "ERR", 128U, NULL, 1U, NULL);
    }

    if (ok == pdPASS) {
        vTaskStartScheduler();
    }
    for (;;) {
        /* chỉ tới đây nếu thiếu heap */
    }
}
