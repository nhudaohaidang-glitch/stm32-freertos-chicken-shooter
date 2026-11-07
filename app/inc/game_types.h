#ifndef GAME_TYPES_H
#define GAME_TYPES_H
#include <stdint.h>
typedef enum { GAME_MENU, GAME_PLAYING, GAME_PAUSED, GAME_OVER } game_state_t;
typedef enum {
    APP_MOVE_LEFT, APP_MOVE_RIGHT, APP_SHOOT, APP_START_PAUSE,
    APP_START, APP_PAUSE, APP_RESUME, APP_RESTART,
    APP_HIT, APP_LOSE_HP, APP_GAME_OVER
} app_event_kind_t;
typedef struct { app_event_kind_t kind; int16_t value; uint32_t tick; } app_event_t;
typedef enum { SOUND_SHOOT, SOUND_HIT, SOUND_GAME_OVER } app_sound_t;
/* Small initial interface only; entity arrays/dirty rectangles need an API PR.
 * No pointer to mutable game data may be sent to the render task. */
typedef struct {
    game_state_t state;
    uint32_t score;
    uint16_t player_x;
    uint8_t hp;
} game_snapshot_t;
#endif
