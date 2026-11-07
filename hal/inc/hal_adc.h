#ifndef HAL_ADC_H
#define HAL_ADC_H
#include "hal_status.h"
/*
 * Đọc joystick bằng ADC1:
 *   PA0 = kênh 0 = trục X (VRx)
 *   PA1 = kênh 1 = trục Y (VRy)
 * Mỗi lần đọc: chọn kênh -> bắt đầu chuyển đổi -> chờ cờ EOC -> lấy DR.
 */

#define HAL_ADC_MAX_VALUE 4095U   /* ADC 12 bit: 0..4095 */

/* Gọi 1 lần trong main(), TRƯỚC vTaskStartScheduler(). Tạo mutex, cấu hình ADC.
 * Thiếu heap tạo mutex -> HAL_IO_ERROR. */
hal_status_t hal_adc_init(void);

/* Reads ADC1 channels 0/1; raw 12-bit values, calibration belongs to HMI.
 * Trả về:
 *   HAL_OK          đọc xong, x và y có giá trị 0..4095
 *   HAL_INVALID_ARG x hoặc y là NULL, hoặc timeout_ms = 0
 *   HAL_IO_ERROR    chưa gọi hal_adc_init() thành công
 *   HAL_TIMEOUT     chờ mutex quá timeout_ms, hoặc ADC không báo xong
 *                   trong khoảng timeout_ms (mỗi trục)
 * Thread-safe: có mutex (timeout hữu hạn) nên nhiều task gọi cũng an toàn.
 * Không gọi trong ngắt. */
hal_status_t hal_adc_read_joystick(uint16_t *x, uint16_t *y, uint32_t timeout_ms);
#endif
