#ifndef HAL_I2C_H
#define HAL_I2C_H
#include "hal_status.h"
/* I2C1 master, PB6 SCL / PB7 SDA, mặc định Fast-mode 400 kHz (polling). */
#ifndef HAL_I2C_CLOCK_HZ
#define HAL_I2C_CLOCK_HZ 400000UL
#endif

/* Gọi một lần trước khi tạo task (hoặc từ một task duy nhất).
 * Tạo mutex bus, khôi phục bus nếu slave giữ SDA, cấu hình I2C1. */
hal_status_t hal_i2c_init(void);

/* addr7 is an unshifted 7-bit address; OLED address must be measured.
 * - size == 0: chỉ gửi địa chỉ (probe), data có thể NULL -> HAL_OK nếu có ACK.
 * - NACK -> HAL_IO_ERROR (bus được nhả bằng STOP).
 * - Bus error / mất arbitration -> HAL_IO_ERROR; quá thời gian -> HAL_TIMEOUT.
 *   Hai trường hợp này driver tự khôi phục bus và reset I2C1.
 * - timeout_ms bao gồm thời gian chờ mutex + truyền; 0 -> HAL_INVALID_ARG
 *   (truyền polling không có chế độ nonblocking).
 * Thread-safe giữa các task (mutex có priority inheritance). Không gọi từ ISR. */
hal_status_t hal_i2c_write(uint8_t addr7, const uint8_t *data, size_t size, uint32_t timeout_ms);
#endif
