#ifndef ILI9341_H
#define ILI9341_H
#include <stdbool.h>
#include "hal_status.h"
/*
 * Driver TFT ILI9341 240x320 qua hal_spi (SPI1) + chân điều khiển:
 *   PA4 CS, PA3 DC (0 = lệnh, 1 = dữ liệu), PB1 RESET.
 * Màu RGB565. Bộ đệm điểm ảnh truyền theo thứ tự byte cao trước (big-endian),
 * dùng ILI9341_BE() khi ghi vào bộ đệm uint16_t.
 *
 * Không có framebuffer: người gọi mở một cửa sổ (begin), đẩy dữ liệu theo dải
 * (push), rồi đóng (end). begin..end giữ khóa SPI và CS thấp.
 * Một chủ sở hữu (TASK_RENDER). Không gọi từ ISR.
 */
#define ILI9341_WIDTH  240U
#define ILI9341_HEIGHT 320U
/* MADCTL: 0x48 = MX | BGR, dọc, gốc trên-trái cho đa số module 2.8".
 * Nếu màu đỏ/xanh dương bị đảo: bỏ bit BGR (0x08). Nếu hình bị lật: thử 0x88 / 0xC8. */
#ifndef ILI9341_MADCTL
#define ILI9341_MADCTL 0x48U
#endif

#define ILI9341_RGB(r, g, b) ((uint16_t)((((r) & 0xF8U) << 8) | (((g) & 0xFCU) << 3) | ((b) >> 3)))
#define ILI9341_BE(c) ((uint16_t)((((c) & 0xFFU) << 8) | ((c) >> 8)))

/* Reset cứng + chuỗi khởi tạo. Gọi từ task (dùng vTaskDelay ~250 ms) hoặc trước scheduler.
 * timeout_ms áp dụng cho từng lời gọi bus; không bao gồm thời gian chờ của panel.
 * timeout_ms=0 -> HAL_TIMEOUT ngay, không reset/gửi lệnh. */
hal_status_t ili9341_init(uint32_t timeout_ms);

/* Mở cửa sổ (x, y, w, h) đã cắt theo màn hình; HAL_INVALID_ARG nếu rỗng.
 * timeout_ms=0 -> HAL_TIMEOUT ngay, không thay CS/DC hay giữ mutex. */
hal_status_t ili9341_begin(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t timeout_ms);
hal_status_t ili9341_push(const void *pixels_be, size_t bytes, uint32_t timeout_ms);
void ili9341_end(void);

/* Tiện ích một lệnh: tô vùng một màu / vẽ ảnh RGB565 big-endian. */
hal_status_t ili9341_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color,
                               uint32_t timeout_ms);
hal_status_t ili9341_blit(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const void *pixels_be,
                          uint32_t timeout_ms);
#endif
