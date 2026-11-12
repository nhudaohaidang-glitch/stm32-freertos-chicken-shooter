#include "hal_adc.h"
#include "FreeRTOS.h"   /* configCPU_CLOCK_HZ */
#include "task.h"
#include "semphr.h"     /* mutex */

/* ---------- Thanh ghi (địa chỉ theo RM0008) ---------- */
#define RCC_CFGR     (*(volatile uint32_t *)0x40021004UL)
#define RCC_APB2ENR  (*(volatile uint32_t *)0x40021018UL)
#define GPIOA_CRL    (*(volatile uint32_t *)0x40010800UL)
#define ADC1_SR      (*(volatile uint32_t *)0x40012400UL)
#define ADC1_CR1     (*(volatile uint32_t *)0x40012404UL)
#define ADC1_CR2     (*(volatile uint32_t *)0x40012408UL)
#define ADC1_SMPR2   (*(volatile uint32_t *)0x40012410UL)
#define ADC1_SQR1    (*(volatile uint32_t *)0x4001242CUL)
#define ADC1_SQR3    (*(volatile uint32_t *)0x40012434UL)
#define ADC1_DR      (*(volatile uint32_t *)0x4001244CUL)

/* ---------- Các bit dùng tới ---------- */
#define RCC_APB2ENR_IOPAEN  (1UL << 2)
#define RCC_APB2ENR_ADC1EN  (1UL << 9)
#define ADC_SR_EOC          (1UL << 1)    /* chuyển đổi xong */
#define ADC_CR2_ADON        (1UL << 0)    /* bật ADC */
#define ADC_CR2_CAL         (1UL << 2)    /* hiệu chuẩn */
#define ADC_CR2_RSTCAL      (1UL << 3)    /* reset hiệu chuẩn */
#define ADC_CR2_EXTSEL_SW   (7UL << 17)   /* nguồn kích = SWSTART */
#define ADC_CR2_EXTTRIG     (1UL << 20)
#define ADC_CR2_SWSTART     (1UL << 22)   /* bắt đầu chuyển đổi */

#define CHANNEL_X 0U
#define CHANNEL_Y 1U

/* Số vòng lặp chờ cho 1 ms (xấp xỉ, mỗi vòng >= 4 chu kỳ CPU).
 * Dùng vòng đếm thay cho tick để chạy được cả trước khi scheduler chạy. */
#define LOOPS_PER_MS (configCPU_CLOCK_HZ / 4000U)

static uint8_t adc_ready = 0U;
static SemaphoreHandle_t adc_mutex = NULL;   /* thread-safe: mỗi lúc một task dùng ADC */

/* Mutex chỉ có tác dụng khi scheduler đã chạy; trước đó chỉ có main() nên không cần. */
static int rtos_running(void)
{
    return xTaskGetSchedulerState() == taskSCHEDULER_RUNNING;
}

/* Chờ một bit trong CR2 tự về 0 (dùng cho hiệu chuẩn). */
static hal_status_t wait_cr2_bit_clear(uint32_t bit, uint32_t timeout_ms)
{
    uint32_t loops = timeout_ms * LOOPS_PER_MS;
    while ((ADC1_CR2 & bit) != 0U) {
        if (loops == 0U) {
            return HAL_TIMEOUT;
        }
        loops--;
    }
    return HAL_OK;
}

hal_status_t hal_adc_init(void)
{
    adc_ready = 0U;

    /* 0. Tạo mutex một lần (cấp phát từ heap FreeRTOS). */
    if (adc_mutex == NULL) {
        adc_mutex = xSemaphoreCreateMutex();
        if (adc_mutex == NULL) {
            return HAL_IO_ERROR;
        }
    }

    /* 1. Clock ADC tối đa 14 MHz: 8 MHz / 2 = 4 MHz, 72 MHz / 6 = 12 MHz.
     *    ADCPRE nằm ở bit 15:14 của RCC_CFGR (00 = /2, 10 = /6). */
    RCC_CFGR &= ~(3UL << 14);
    if (configCPU_CLOCK_HZ > 28000000UL) {
        RCC_CFGR |= (2UL << 14);
    }

    /* 2. Bật clock cho GPIOA và ADC1. */
    RCC_APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_ADC1EN;

    /* 3. PA0, PA1 ở chế độ analog: mỗi chân 4 bit trong CRL, 0000 = analog. */
    GPIOA_CRL &= ~(0xFFUL);

    /* 4. Cấu hình ADC1: một kênh mỗi lần, kích bằng phần mềm, căn phải. */
    ADC1_CR1 = 0U;
    ADC1_CR2 = ADC_CR2_EXTSEL_SW | ADC_CR2_EXTTRIG;
    ADC1_SQR1 = 0U;                         /* chuỗi chỉ có 1 lần chuyển đổi */

    /* 5. Thời gian lấy mẫu dài nhất (mã 7 = 239.5 chu kỳ) cho kênh 0 và 1,
     *    vì biến trở joystick ~10 kOhm cần thời gian nạp. */
    ADC1_SMPR2 = (7UL << 0) | (7UL << 3);

    /* 6. Bật ADC, chờ ổn định một chút rồi hiệu chuẩn. */
    ADC1_CR2 |= ADC_CR2_ADON;
    for (volatile uint32_t i = 0U; i < 1000U; i++) {
        /* chờ > 1 us */
    }
    ADC1_CR2 |= ADC_CR2_RSTCAL;
    if (wait_cr2_bit_clear(ADC_CR2_RSTCAL, 10U) != HAL_OK) {
        return HAL_TIMEOUT;
    }
    ADC1_CR2 |= ADC_CR2_CAL;
    if (wait_cr2_bit_clear(ADC_CR2_CAL, 10U) != HAL_OK) {
        return HAL_TIMEOUT;
    }

    adc_ready = 1U;
    return HAL_OK;
}

/* Đọc 1 kênh: chọn kênh, bắt đầu, chờ EOC, lấy kết quả. */
static hal_status_t read_channel(uint32_t channel, uint16_t *value, uint32_t timeout_ms)
{
    uint32_t loops = timeout_ms * LOOPS_PER_MS;

    ADC1_SQR3 = channel;                    /* kênh cần đọc */
    ADC1_SR = 0U;                           /* xóa cờ cũ */
    ADC1_CR2 |= ADC_CR2_SWSTART;            /* bắt đầu chuyển đổi */

    while ((ADC1_SR & ADC_SR_EOC) == 0U) {
        if (loops == 0U) {
            return HAL_TIMEOUT;
        }
        loops--;
    }
    *value = (uint16_t)(ADC1_DR & 0x0FFFU); /* đọc DR cũng tự xóa EOC */
    return HAL_OK;
}

hal_status_t hal_adc_read_joystick(uint16_t *x, uint16_t *y, uint32_t timeout_ms)
{
    uint16_t vx = 0U;
    uint16_t vy = 0U;
    hal_status_t status;

    if (x == NULL || y == NULL || timeout_ms == 0U) {
        return HAL_INVALID_ARG;
    }
    if (adc_ready == 0U) {
        return HAL_IO_ERROR;
    }

    /* Chờ mutex có giới hạn: không bao giờ block vĩnh viễn. */
    if (rtos_running() && xSemaphoreTake(adc_mutex, pdMS_TO_TICKS(timeout_ms)) != pdTRUE) {
        return HAL_TIMEOUT;
    }

    status = read_channel(CHANNEL_X, &vx, timeout_ms);
    if (status == HAL_OK) {
        status = read_channel(CHANNEL_Y, &vy, timeout_ms);
    }

    if (rtos_running()) {
        (void)xSemaphoreGive(adc_mutex);
    }

    if (status == HAL_OK) {
        *x = vx;
        *y = vy;
    }
    return status;
}
