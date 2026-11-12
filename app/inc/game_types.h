#ifndef GAME_TYPES_H
#define GAME_TYPES_H
#include <stdint.h>
/*
 * Kiểu dữ liệu dùng chung HMI / GAME / RENDER (API v2).
 * Chỉ gồm stdint: game engine và render có thể biên dịch/kiểm thử trên PC.
 */
typedef enum { GAME_MENU, GAME_PLAYING, GAME_PAUSED, GAME_OVER } game_state_t;
typedef enum {
    APP_MOVE_LEFT, APP_MOVE_RIGHT, APP_SHOOT, APP_START_PAUSE,
    APP_START, APP_PAUSE, APP_RESUME, APP_RESTART,
    APP_HIT, APP_LOSE_HP, APP_GAME_OVER
} app_event_kind_t;
/* MOVE_LEFT/MOVE_RIGHT: value = tốc độ 0..GAME_PLAYER_MAX_SPEED px/khung (0 = đứng yên).
 * HMI chỉ gửi khi tốc độ đổi và gửi lại định kỳ (gộp được, mất một gói không sao).
 * tick = xTaskGetTickCount() lúc tạo sự kiện. */
typedef struct { app_event_kind_t kind; int16_t value; uint32_t tick; } app_event_t;

/* ---- Hình học sân chơi (dọc 240x320, gốc trên-trái, đơn vị pixel) ---- */
#define GAME_SCREEN_W        240
#define GAME_SCREEN_H        320
#define GAME_PLAYER_W        16
#define GAME_PLAYER_H        16
#define GAME_PLAYER_Y        (GAME_SCREEN_H - GAME_PLAYER_H - 6)
#define GAME_PLAYER_MAX_SPEED 4
#define GAME_CHICKEN_W       16
#define GAME_CHICKEN_H       14
#define GAME_CHICKEN_ROWS    3
#define GAME_CHICKEN_COLS    6
#define GAME_CHICKEN_COUNT   (GAME_CHICKEN_ROWS * GAME_CHICKEN_COLS)
#define GAME_CHICKEN_DX      28   /* khoảng cách cột */
#define GAME_CHICKEN_DY      22   /* khoảng cách hàng */
#define GAME_FORMATION_W     ((GAME_CHICKEN_COLS - 1) * GAME_CHICKEN_DX + GAME_CHICKEN_W)
#define GAME_FORMATION_TOP   20
#define GAME_BULLET_W        2
#define GAME_BULLET_H        6
#define GAME_MAX_BULLETS     6
#define GAME_EGG_W           4
#define GAME_EGG_H           6
#define GAME_MAX_EGGS        6
#define GAME_BOOM_W          16
#define GAME_BOOM_H          14
#define GAME_MAX_BOOMS       4
#define GAME_MAX_HP          3

#define GAME_POS_NONE        INT16_MIN   /* x = GAME_POS_NONE: ô trống */
typedef struct { int16_t x; int16_t y; } game_pos_t;

#define GAME_FLAG_PLAYER_VISIBLE 0x01U   /* tắt khi nhấp nháy sau khi trúng trứng */
#define GAME_FLAG_CHICKEN_FRAME  0x02U   /* khung vỗ cánh */

/* Bản sao trạng thái gửi GAME -> RENDER qua queue dài 1 (xQueueOverwrite).
 * Không chứa con trỏ tới dữ liệu game; RENDER không bao giờ ghi vào game state.
 * Vị trí gà i (hàng r = i / COLS, cột c = i % COLS):
 *   x = formation_x + c * GAME_CHICKEN_DX, y = formation_y + r * GAME_CHICKEN_DY. */
typedef struct {
    game_state_t state;
    uint32_t score;
    uint32_t frame;          /* số khung GAME đã chạy (20 ms/khung khi PLAYING) */
    uint16_t player_x;
    uint8_t hp;
    uint8_t wave;
    uint8_t flags;
    int16_t formation_x;
    int16_t formation_y;
    uint32_t chicken_alive;  /* bit i = gà i còn sống */
    game_pos_t bullets[GAME_MAX_BULLETS];
    game_pos_t eggs[GAME_MAX_EGGS];
    game_pos_t booms[GAME_MAX_BOOMS];
} game_snapshot_t;

_Static_assert(GAME_CHICKEN_COUNT <= 32, "chicken_alive is a 32-bit mask");
_Static_assert(GAME_FORMATION_W < GAME_SCREEN_W, "formation must fit the screen");
#endif
