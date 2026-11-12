#ifndef HAL_SPI_H
#define HAL_SPI_H
#include "hal_status.h"
/*
 * SPI1 master chỉ truyền (1-line TX, BIDIMODE): PA5 SCK, PA7 MOSI, mode 0, 8 bit, MSB trước.
 * PA6/MISO không dùng. Driver thiết bị (ILI9341) tự quản lý CS/DC.
 *
 * Quy ước khóa giao dịch: driver thiết bị gọi hal_spi_lock() trước khi kéo CS xuống
 * và hal_spi_unlock() sau khi kéo CS lên, để CS/DC + payload là một khối nguyên tử.
 * Các hàm ghi bên dưới KHÔNG tự khóa; khi scheduler đang chạy, người gọi phải giữ khóa.
 *
 * Truyền bằng polling (không DMA). Tốc độ thực phải đo bằng harness trên kit;
 * không suy ra FPS từ riêng tốc độ SCK.
 */
#ifndef HAL_SPI_MAX_HZ
#define HAL_SPI_MAX_HZ 18000000UL  /* HSI 8 MHz -> 4 MHz; PCLK2 72 MHz -> 18 MHz */
#endif

/* Gọi một lần trước khi tạo task: tạo mutex, cấu hình chân và SPI1. */
hal_status_t hal_spi_init(void);
/* Tần số SCK thực tế đã chọn (Hz), 0 nếu chưa init. */
uint32_t hal_spi_clock_hz(void);

/* Khóa bus có timeout hữu hạn (mutex, priority inheritance).
 * timeout_ms=0: thử lấy khóa ngay, không chờ; khóa bận -> HAL_TIMEOUT.
 * Trước scheduler: HAL_OK sau init (main là chủ sở hữu duy nhất).
 * Không gọi trong ISR, critical section hoặc khi scheduler đang bị suspend. */
hal_status_t hal_spi_lock(uint32_t timeout_ms);
void hal_spi_unlock(void);

/* Polling: gửi size byte, chờ tới khi bus rảnh (BSY=0) mới trả HAL_OK,
 * để người gọi có thể đổi DC/CS sau lần truyền thành công.
 * timeout_ms=0: không gửi byte; size>0 -> HAL_TIMEOUT, không đổi SPI_DR.
 * size=0: chỉ chờ/kiểm tra bus idle; data có thể NULL.
 * Timeout áp dụng riêng cho mỗi lời gọi, không phải toàn bộ frame/init TFT. */
hal_status_t hal_spi_write(const uint8_t *data, size_t size, uint32_t timeout_ms);
/* Gửi count lần giá trị 16 bit (byte cao trước): tô màu không cần bộ đệm.
 * timeout_ms=0 tương tự write: count>0 -> HAL_TIMEOUT, không truyền. */
hal_status_t hal_spi_write_repeat16(uint16_t value, uint32_t count, uint32_t timeout_ms);

#endif
