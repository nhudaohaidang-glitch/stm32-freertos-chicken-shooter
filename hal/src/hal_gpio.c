#include "hal_gpio.h"
#include "FreeRTOS.h"
#include "task.h"       /* taskENTER_CRITICAL, vTaskNotifyGiveFromISR */

/* ---------- Thanh ghi (địa chỉ theo RM0008 và tài liệu Cortex-M3) ---------- */
#define RCC_APB2ENR   (*(volatile uint32_t *)0x40021018UL)
#define AFIO_EXTICR4  (*(volatile uint32_t *)0x40010014UL)
#define GPIOB_CRH     (*(volatile uint32_t *)0x40010C04UL)
#define GPIOB_IDR     (*(volatile uint32_t *)0x40010C08UL)
#define GPIOB_ODR     (*(volatile uint32_t *)0x40010C0CUL)
#define EXTI_IMR      (*(volatile uint32_t *)0x40010400UL)
#define EXTI_RTSR     (*(volatile uint32_t *)0x40010408UL)
#define EXTI_FTSR     (*(volatile uint32_t *)0x4001040CUL)
#define EXTI_PR       (*(volatile uint32_t *)0x40010414UL)
#define NVIC_ISER1    (*(volatile uint32_t *)0xE000E104UL)
#define NVIC_IPR_EXTI15_10 (*(volatile uint8_t *)(0xE000E400UL + 40U))

#define RCC_APB2ENR_AFIOEN (1UL << 0)
#define RCC_APB2ENR_IOPBEN (1UL << 3)

#define PIN_FIRE  12U
#define PIN_START 13U
#define PIN_MASK  ((1UL << PIN_FIRE) | (1UL << PIN_START))

/* Ưu tiên ngắt EXTI15_10 = 6 (số càng lớn ưu tiên càng thấp).
 * Ngắt gọi hàm ...FromISR nên BẮT BUỘC số này >= 5
 * (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY trong FreeRTOSConfig.h). */
#define EXTI_PRIORITY 6U
#if EXTI_PRIORITY < configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY
#error "EXTI_PRIORITY phai >= configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY"
#endif

static uint8_t gpio_ready = 0U;
static volatile uint8_t edges = 0U;   /* ngắt ghi, task đọc */
static TaskHandle_t wake_task = NULL;  /* task được đánh thức khi có nhấn */

hal_status_t hal_gpio_init(void)
{
    /* 1. Bật clock GPIOB và AFIO (AFIO cần cho EXTI). */
    RCC_APB2ENR |= RCC_APB2ENR_IOPBEN | RCC_APB2ENR_AFIOEN;

    /* 2. PB12, PB13 là input có điện trở kéo: mỗi chân 4 bit trong CRH,
     *    1000 = input pull-up/pull-down. PB12 ở bit 19:16, PB13 ở bit 23:20. */
    GPIOB_CRH &= ~(0xFFUL << 16);
    GPIOB_CRH |=  (0x88UL << 16);

    /* 3. Chọn kéo lên hay kéo xuống bằng ODR: 1 = kéo lên, 0 = kéo xuống. */
#if HAL_GPIO_BUTTON_ACTIVE_LOW
    GPIOB_ODR |= PIN_MASK;
#else
    GPIOB_ODR &= ~PIN_MASK;
#endif

    gpio_ready = 1U;
    return HAL_OK;
}

hal_status_t hal_gpio_read_buttons(uint8_t *pressed)
{
    uint32_t pins;
    uint8_t result = 0U;

    if (pressed == NULL) {
        return HAL_INVALID_ARG;
    }
    if (gpio_ready == 0U) {
        return HAL_IO_ERROR;
    }

    pins = GPIOB_IDR;
#if HAL_GPIO_BUTTON_ACTIVE_LOW
    pins = ~pins;                /* nhấn = 0 -> đảo lại để nhấn = 1 */
#endif
    if ((pins & (1UL << PIN_FIRE)) != 0U) {
        result |= HAL_GPIO_BTN_FIRE;
    }
    if ((pins & (1UL << PIN_START)) != 0U) {
        result |= HAL_GPIO_BTN_START_PAUSE;
    }
    *pressed = result;
    return HAL_OK;
}

hal_status_t hal_gpio_exti_enable(TaskHandle_t notify_task)
{
    if (gpio_ready == 0U) {
        return HAL_IO_ERROR;
    }
    wake_task = notify_task;     /* gán trước khi bật ngắt */

    /* 1. Nối EXTI12, EXTI13 vào cổng B (mã cổng B = 0001).
     *    EXTI12 ở bit 3:0, EXTI13 ở bit 7:4 của AFIO_EXTICR4. */
    AFIO_EXTICR4 &= ~0xFFUL;
    AFIO_EXTICR4 |=  0x11UL;

    /* 2. Ngắt ở cạnh nhấn: nút kéo lên thì nhấn là cạnh xuống. */
#if HAL_GPIO_BUTTON_ACTIVE_LOW
    EXTI_FTSR |= PIN_MASK;
#else
    EXTI_RTSR |= PIN_MASK;
#endif

    /* 3. Xóa cờ cũ (ghi 1 để xóa) rồi cho phép ngắt hai đường 12, 13. */
    EXTI_PR = PIN_MASK;
    EXTI_IMR |= PIN_MASK;

    /* 4. NVIC: EXTI15_10 là IRQ số 40 -> thanh ghi ISER1, bit 8.
     *    STM32F1 dùng 4 bit cao của byte ưu tiên. */
    NVIC_IPR_EXTI15_10 = (uint8_t)(EXTI_PRIORITY << 4);
    NVIC_ISER1 = (1UL << 8);
    return HAL_OK;
}

uint8_t hal_gpio_take_edges(void)
{
    uint8_t result;

    /* Khóa ngắt ngắn để ngắt không ghi đè giữa lúc đọc và xóa. */
    taskENTER_CRITICAL();
    result = edges;
    edges = 0U;
    taskEXIT_CRITICAL();
    return result;
}

/* Hàm ngắt: tên phải đúng như trong file startup . */
void EXTI15_10_IRQHandler(void)
{
    uint32_t pending = EXTI_PR & PIN_MASK;

    EXTI_PR = pending;           /* xóa cờ của các chân vừa ngắt */

    if ((pending & (1UL << PIN_FIRE)) != 0U) {
        edges |= HAL_GPIO_BTN_FIRE;
    }
    if ((pending & (1UL << PIN_START)) != 0U) {
        edges |= HAL_GPIO_BTN_START_PAUSE;
    }

    /* Đánh thức task: trong ngắt chỉ dùng hàm có đuôi FromISR. */
    if (pending != 0U && wake_task != NULL) {
        BaseType_t higher_task_woken = pdFALSE;
        vTaskNotifyGiveFromISR(wake_task, &higher_task_woken);
        portYIELD_FROM_ISR(higher_task_woken);   /* chuyển ngay sang task nếu nó ưu tiên cao hơn */
    }
}
