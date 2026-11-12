#ifndef HAL_GPIO_H
#define HAL_GPIO_H
#include "hal_status.h"
#include "FreeRTOS.h"
#include "task.h"
/*
 * Hai nút nhấn:
 *   PB12 = Fire        -> bit 0
 *   PB13 = Start/Pause -> bit 1
 * Mặc định nút nối xuống GND, dùng điện trở kéo lên bên trong chip,
 * nên nhấn = mức 0. Nếu nút nối lên 3,3 V thì đổi HAL_GPIO_BUTTON_ACTIVE_LOW = 0.
 */
#ifndef HAL_GPIO_BUTTON_ACTIVE_LOW
#define HAL_GPIO_BUTTON_ACTIVE_LOW 1
#endif

#define HAL_GPIO_BTN_FIRE        0x01U
#define HAL_GPIO_BTN_START_PAUSE 0x02U

/* Gọi 1 lần trong main(), TRƯỚC vTaskStartScheduler(). */
hal_status_t hal_gpio_init(void);

/* bits 0/1 = Fire / Start-Pause; logical pressed, independent of polarity.
 * Trạng thái tức thời, CHƯA chống dội. pressed = NULL -> HAL_INVALID_ARG. */
hal_status_t hal_gpio_read_buttons(uint8_t *pressed);

/* Debounce in HMI; EXTI, if used, only records edges via ...FromISR APIs.
 * Bật ngắt EXTI12/EXTI13 tại cạnh nhấn. Trong ngắt:
 *   - ghi lại "nút nào vừa được nhấn" (lấy bằng hal_gpio_take_edges)
 *   - nếu notify_task khác NULL: đánh thức task đó bằng vTaskNotifyGiveFromISR.
 * Task chờ bằng ulTaskNotifyTake(pdTRUE, timeout). Truyền NULL nếu không cần.
 * Gọi sau hal_gpio_init(). */
hal_status_t hal_gpio_exti_enable(TaskHandle_t notify_task);

/* Lấy các nút đã có cạnh nhấn kể từ lần gọi trước, rồi xóa.
 * Giúp không bỏ lỡ lần nhấn rất ngắn giữa hai chu kỳ đọc. Chỉ gọi từ task. */
uint8_t hal_gpio_take_edges(void);
#endif
