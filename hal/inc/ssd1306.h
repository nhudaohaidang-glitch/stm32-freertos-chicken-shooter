#ifndef SSD1306_H
#define SSD1306_H
#include <stdbool.h>
#include "hal_status.h"
/*
 * Driver OLED SSD1306 128x64 qua hal_i2c. Buffer 1 KiB nằm trong driver.
 * Các hàm vẽ chỉ sửa buffer và đánh dấu page (8 hàng pixel) bị thay đổi;
 * ssd1306_flush() chỉ gửi các page đó -> 1 page ~3 ms thay vì ~25 ms cả màn
 * ở 400 kHz, tránh chặn TASK_RENDER khi chỉ đổi điểm/HP.
 * Một chủ sở hữu (TASK_RENDER). Không gọi từ ISR.
 * Lưu ý: module 1.3" thường là SH1106 (khác SSD1306), driver này không hỗ trợ.
 */
#define SSD1306_WIDTH  128U
#define SSD1306_HEIGHT 64U
#define SSD1306_PAGES  (SSD1306_HEIGHT / 8U)
#define SSD1306_ADDR_DEFAULT 0x3CU /* hoặc 0x3D tùy jumper; phải đo bằng probe */

/* Chờ >= 100 ms sau khi cấp nguồn trước khi gọi (datasheet). */
hal_status_t ssd1306_init(uint8_t addr7, uint32_t timeout_ms);
hal_status_t ssd1306_display_on(bool on, uint32_t timeout_ms);
hal_status_t ssd1306_set_contrast(uint8_t level, uint32_t timeout_ms);

void ssd1306_clear(void);
void ssd1306_draw_pixel(int16_t x, int16_t y, bool on);
void ssd1306_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, bool on);
/* Vẽ chuỗi font 5x7 (bước 6 px/ký tự x scale), cắt ở mép màn. Trả về x kế tiếp. */
int16_t ssd1306_draw_text(int16_t x, int16_t y, const char *s, uint8_t scale);

/* Gửi các page đã đổi. timeout_ms áp dụng cho từng giao dịch I2C. */
hal_status_t ssd1306_flush(uint32_t timeout_ms);
/* Đánh dấu toàn màn bẩn (ví dụ sau lỗi I2C, để lần flush sau vẽ lại hết). */
void ssd1306_invalidate(void);
#endif
