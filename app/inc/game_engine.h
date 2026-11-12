#ifndef GAME_ENGINE_H
#define GAME_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#include "game_types.h"
/*
 * Game engine thuần C: không gọi FreeRTOS/HAL, không cấp phát động.
 * TASK_GAME là nơi DUY NHẤT gọi các hàm này (sở hữu game_t).
 * Kiểm thử trên PC: tests/host.
 *
 * FSM (bảng trong docs/ARCHITECTURE.md):
 *   MENU      --START | START_PAUSE-->            PLAYING (ván mới)
 *   PLAYING   --PAUSE | START_PAUSE-->            PAUSED
 *   PLAYING   --GAME_OVER | HP về 0 | gà chạm đáy--> GAME_OVER
 *   PAUSED    --RESUME | START_PAUSE-->           PLAYING (không bù thời gian đã dừng)
 *   GAME_OVER --RESTART | START_PAUSE-->          PLAYING (ván mới)
 * Sự kiện không hợp lệ theo trạng thái bị bỏ qua và được đếm (ignored_events).
 */

typedef struct { int16_t x; int16_t y; bool active; } game_obj_t;
typedef struct { int16_t x; int16_t y; uint8_t ttl; } game_boom_t;

typedef struct {
    game_state_t state;
    uint32_t score;
    uint32_t frame;
    uint32_t rng;
    uint32_t ignored_events;
    uint8_t hp;
    uint8_t wave;
    int16_t player_x;
    int8_t player_vx;
    uint8_t shot_cooldown;
    uint8_t invuln;
    int32_t formation_x_q8;   /* fixed-point 24.8 cho tốc độ lẻ pixel */
    int16_t formation_y;
    int8_t formation_dir;
    uint32_t chicken_alive;
    game_obj_t bullets[GAME_MAX_BULLETS];
    game_obj_t eggs[GAME_MAX_EGGS];
    game_boom_t booms[GAME_MAX_BOOMS];
} game_t;

/* Thông số chơi (đổi ở đây để cân bằng độ khó). */
#define GAME_BULLET_SPEED      6
#define GAME_SHOT_COOLDOWN     8      /* khung (160 ms) */
#define GAME_INVULN_FRAMES     60     /* 1,2 s nhấp nháy sau khi trúng trứng */
#define GAME_BOOM_FRAMES       10
#define GAME_FORMATION_STEP_Y  8
#define GAME_SCORE_PER_CHICKEN 10U
#define GAME_SCORE_WAVE_BONUS  50U

void game_init(game_t *g, uint32_t seed);
/* Xử lý một sự kiện theo FSM (event-driven). */
void game_handle_event(game_t *g, const app_event_t *ev);
/* Một bước 20 ms; chỉ cập nhật khi PLAYING (cập nhật định kỳ). */
void game_step(game_t *g);
void game_get_snapshot(const game_t *g, game_snapshot_t *out);

/* Tiện ích hình học dùng chung (engine, render, test). */
static inline int16_t game_chicken_x(int16_t formation_x, unsigned idx)
{
    return (int16_t)(formation_x + (int16_t)((idx % GAME_CHICKEN_COLS) * GAME_CHICKEN_DX));
}
static inline int16_t game_chicken_y(int16_t formation_y, unsigned idx)
{
    return (int16_t)(formation_y + (int16_t)((idx / GAME_CHICKEN_COLS) * GAME_CHICKEN_DY));
}
#endif
