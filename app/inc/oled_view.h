#ifndef OLED_VIEW_H
#define OLED_VIEW_H
#include <stdint.h>
#include "hal_status.h"
#include "game_types.h"
/*
 * Màn hình phụ OLED SSD1306 128x64 (I2C): HUD game + số đo đánh giá hệ thống.
 * Bố cục (mỗi page = 8 hàng pixel, 21 ký tự):
 *   page 0    : trạng thái (MENU/PLAYING/PAUSED/GAME OVER) + đợt "W nn"
 *   page 1    : "SCORE"
 *   page 2..3 : điểm, chữ cỡ 2
 *   page 4    : HP dạng trái tim
 *   page 5    : "FPS nn T nnnnnUS"  (FPS thực + thời gian vẽ lâu nhất trong 1 s)
 *   page 6    : "HEAP nnnnn/nnnnn"  (heap trống hiện tại / thấp nhất từng có)
 *   page 7    : "STK nnn/nnn/nnn"   (stack còn trống nhỏ nhất của HMI/GAME/RENDER, word)
 * oled_view_update() vẽ vào bộ đệm SSD1306; driver chỉ gửi các page có pixel đổi
 * (đổi điểm ~2 page ~6 ms ở 400 kHz). Một chủ sở hữu: TASK_RENDER.
 */
typedef struct {
    game_state_t state;
    uint32_t score;
    uint8_t hp;
    uint8_t wave;
    uint16_t fps;
    uint32_t draw_us_max;
    uint32_t heap_free;
    uint32_t heap_min;
    uint16_t stack_free[3];   /* HMI, GAME, RENDER (word) */
} oled_view_info_t;

hal_status_t oled_view_init(uint8_t addr7, uint32_t timeout_ms);
hal_status_t oled_view_update(const oled_view_info_t *info, uint32_t timeout_ms);
/* Chuỗi tên trạng thái. */
const char *oled_view_state_name(game_state_t st);
/* Ghi số thập phân không dấu vào buf (đệm '0' tới width), trả con trỏ sau ký tự cuối. */
char *oled_view_put_u32(char *buf, uint32_t v, unsigned width);
#endif
