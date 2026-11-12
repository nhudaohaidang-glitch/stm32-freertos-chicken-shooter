#ifndef RENDER_TFT_H
#define RENDER_TFT_H
#include <stdint.h>
#include "hal_status.h"
#include "game_types.h"
/*
 * Vẽ sân chơi lên ILI9341 không cần framebuffer (153.600 B > 20 KiB SRAM):
 *  1. So sánh snapshot đã vẽ (prev) với snapshot mới (cur) -> danh sách vùng bẩn
 *     (hợp vị trí cũ + mới của từng gà/đạn/trứng/nổ/tàu).
 *  2. Với mỗi vùng: mở cửa sổ ILI9341, ghép từng dải vào bộ đệm nhỏ
 *     (nền sao + mọi sprite/chữ giao dải, theo thứ tự vẽ) rồi đẩy qua SPI.
 *     Mỗi pixel chỉ ghi một lần với màu cuối -> không nhấp nháy, không vệt.
 *  3. Một bộ đệm 640 pixel (1.280 B): ghép xong một dải thì gửi bằng SPI polling.
 * Đổi trạng thái/đợt hoặc quá nhiều vùng -> vẽ lại toàn màn (~70 ms ở 18 MHz).
 * Thuần C trên API ili9341: kiểm thử trên PC bằng ILI9341 giả (tests/host).
 */
typedef struct {
    uint16_t rects;        /* số vùng đã vẽ ở lần gọi gần nhất */
    uint32_t pixels;       /* số pixel đã gửi */
    uint8_t full_redraw;   /* 1 nếu vẽ lại toàn màn */
} render_tft_stats_t;

/* prev = NULL: vẽ toàn màn. Trả lỗi HAL đầu tiên gặp phải (vùng còn lại bị bỏ,
 * người gọi nên vẽ toàn màn ở lần sau). */
hal_status_t render_tft_draw(const game_snapshot_t *prev, const game_snapshot_t *cur,
                             uint32_t timeout_ms);
const render_tft_stats_t *render_tft_last_stats(void);
#endif
