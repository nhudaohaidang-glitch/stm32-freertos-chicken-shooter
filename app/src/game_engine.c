#include "game_engine.h"
#include <string.h>

#define ALL_CHICKENS ((GAME_CHICKEN_COUNT >= 32) ? 0xFFFFFFFFUL : ((1UL << GAME_CHICKEN_COUNT) - 1UL))

static uint32_t rnd(game_t *g)
{
    /* xorshift32: đủ ngẫu nhiên cho game, xác định được khi test. */
    uint32_t x = g->rng;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    g->rng = x;
    return x;
}

static bool overlap(int16_t ax, int16_t ay, int16_t aw, int16_t ah,
                    int16_t bx, int16_t by, int16_t bw, int16_t bh)
{
    return ax < bx + bw && bx < ax + aw && ay < by + bh && by < ay + ah;
}

static void formation_reset(game_t *g)
{
    g->chicken_alive = ALL_CHICKENS;
    g->formation_x_q8 = (int32_t)((GAME_SCREEN_W - GAME_FORMATION_W) / 2) * 256;
    g->formation_y = GAME_FORMATION_TOP;
    g->formation_dir = 1;
    for (unsigned i = 0; i < GAME_MAX_EGGS; i++) { g->eggs[i].active = false; }
}

static void new_round(game_t *g, uint32_t seed_mix)
{
    g->rng ^= seed_mix * 2654435761UL;
    if (g->rng == 0U) { g->rng = 0x1234567U; }
    g->state = GAME_PLAYING;
    g->score = 0U;
    g->hp = GAME_MAX_HP;
    g->wave = 1U;
    g->player_x = (GAME_SCREEN_W - GAME_PLAYER_W) / 2;
    g->player_vx = 0;
    g->shot_cooldown = 0U;
    g->invuln = 0U;
    for (unsigned i = 0; i < GAME_MAX_BULLETS; i++) { g->bullets[i].active = false; }
    for (unsigned i = 0; i < GAME_MAX_BOOMS; i++) { g->booms[i].ttl = 0U; }
    formation_reset(g);
}

void game_init(game_t *g, uint32_t seed)
{
    memset(g, 0, sizeof *g);
    g->rng = (seed != 0U) ? seed : 0x1234567U;
    g->state = GAME_MENU;
    g->hp = GAME_MAX_HP;
    g->wave = 1U;
    g->player_x = (GAME_SCREEN_W - GAME_PLAYER_W) / 2;
    formation_reset(g);
}

static void lose_hp(game_t *g)
{
    if (g->hp > 0U) { g->hp--; }
    if (g->hp == 0U) {
        g->state = GAME_OVER;
        g->player_vx = 0;
        return;
    }
    g->invuln = GAME_INVULN_FRAMES;
}

static void try_shoot(game_t *g)
{
    if (g->shot_cooldown > 0U) { return; }
    for (unsigned i = 0; i < GAME_MAX_BULLETS; i++) {
        if (!g->bullets[i].active) {
            g->bullets[i].active = true;
            g->bullets[i].x = (int16_t)(g->player_x + (GAME_PLAYER_W - GAME_BULLET_W) / 2);
            g->bullets[i].y = (int16_t)(GAME_PLAYER_Y - GAME_BULLET_H);
            g->shot_cooldown = GAME_SHOT_COOLDOWN;
            return;
        }
    }
}

void game_handle_event(game_t *g, const app_event_t *ev)
{
    if (g == NULL || ev == NULL) { return; }
    switch (g->state) {
    case GAME_MENU:
        if (ev->kind == APP_START || ev->kind == APP_START_PAUSE) {
            new_round(g, ev->tick);
            return;
        }
        break;
    case GAME_PLAYING:
        switch (ev->kind) {
        case APP_MOVE_LEFT:
        case APP_MOVE_RIGHT: {
            int16_t v = ev->value;
            if (v < 0) { v = (int16_t)-v; }
            if (v > GAME_PLAYER_MAX_SPEED) { v = GAME_PLAYER_MAX_SPEED; }
            g->player_vx = (int8_t)((ev->kind == APP_MOVE_LEFT) ? -v : v);
            return;
        }
        case APP_SHOOT:
            try_shoot(g);
            return;
        case APP_PAUSE:
        case APP_START_PAUSE:
            g->state = GAME_PAUSED;
            g->player_vx = 0;
            return;
        case APP_HIT:
            g->score += GAME_SCORE_PER_CHICKEN * g->wave;
            return;
        case APP_LOSE_HP:
            lose_hp(g);
            return;
        case APP_GAME_OVER:
            g->hp = 0U;
            g->state = GAME_OVER;
            g->player_vx = 0;
            return;
        default:
            break;
        }
        break;
    case GAME_PAUSED:
        if (ev->kind == APP_RESUME || ev->kind == APP_START_PAUSE) {
            g->state = GAME_PLAYING;
            return;
        }
        break;
    case GAME_OVER:
        if (ev->kind == APP_RESTART || ev->kind == APP_START_PAUSE) {
            new_round(g, ev->tick);
            return;
        }
        break;
    default:
        break;
    }
    g->ignored_events++;
}

/* Cột trái/phải và hàng dưới cùng còn gà, để đội hình dùng hết bề ngang khi cột chết. */
static void alive_bounds(uint32_t alive, unsigned *cmin, unsigned *cmax, unsigned *rmax)
{
    *cmin = GAME_CHICKEN_COLS;
    *cmax = 0U;
    *rmax = 0U;
    for (unsigned i = 0; i < GAME_CHICKEN_COUNT; i++) {
        if ((alive & (1UL << i)) == 0U) { continue; }
        const unsigned c = i % GAME_CHICKEN_COLS;
        const unsigned r = i / GAME_CHICKEN_COLS;
        if (c < *cmin) { *cmin = c; }
        if (c > *cmax) { *cmax = c; }
        if (r > *rmax) { *rmax = r; }
    }
}

static void move_formation(game_t *g)
{
    unsigned cmin, cmax, rmax;
    alive_bounds(g->chicken_alive, &cmin, &cmax, &rmax);
    if (cmin > cmax) { return; }
    /* Tốc độ 0,5 px/khung, +0,25 mỗi đợt, tối đa 2 px/khung. */
    int32_t speed_q8 = 128 + 64 * (int32_t)(g->wave - 1U);
    if (speed_q8 > 512) { speed_q8 = 512; }
    g->formation_x_q8 += g->formation_dir * speed_q8;

    const int32_t left_min_q8 = -(int32_t)(cmin * GAME_CHICKEN_DX) * 256;
    const int32_t right_max_q8 =
        (int32_t)(GAME_SCREEN_W - GAME_CHICKEN_W - (int32_t)(cmax * GAME_CHICKEN_DX)) * 256;
    if (g->formation_x_q8 <= left_min_q8 || g->formation_x_q8 >= right_max_q8) {
        g->formation_x_q8 = (g->formation_x_q8 <= left_min_q8) ? left_min_q8 : right_max_q8;
        g->formation_dir = (int8_t)-g->formation_dir;
        g->formation_y = (int16_t)(g->formation_y + GAME_FORMATION_STEP_Y);
    }
}

static void spawn_boom(game_t *g, int16_t x, int16_t y)
{
    unsigned best = 0U;
    for (unsigned i = 0; i < GAME_MAX_BOOMS; i++) {
        if (g->booms[i].ttl == 0U) { best = i; break; }
        if (g->booms[i].ttl < g->booms[best].ttl) { best = i; }
    }
    g->booms[best].x = x;
    g->booms[best].y = y;
    g->booms[best].ttl = GAME_BOOM_FRAMES;
}

static void bullets_vs_chickens(game_t *g)
{
    const int16_t fx = (int16_t)(g->formation_x_q8 >> 8);
    for (unsigned b = 0; b < GAME_MAX_BULLETS; b++) {
        game_obj_t *bu = &g->bullets[b];
        if (!bu->active) { continue; }
        for (unsigned i = 0; i < GAME_CHICKEN_COUNT; i++) {
            if ((g->chicken_alive & (1UL << i)) == 0U) { continue; }
            const int16_t cx = game_chicken_x(fx, i);
            const int16_t cy = game_chicken_y(g->formation_y, i);
            if (overlap(bu->x, bu->y, GAME_BULLET_W, GAME_BULLET_H,
                        cx, cy, GAME_CHICKEN_W, GAME_CHICKEN_H)) {
                g->chicken_alive &= ~(1UL << i);
                bu->active = false;
                g->score += GAME_SCORE_PER_CHICKEN * g->wave;
                spawn_boom(g, cx, cy);
                break;
            }
        }
    }
}

static void drop_egg(game_t *g)
{
    /* Xác suất mỗi khung (phần nghìn): 12 + 6 * đợt, tối đa 60. */
    uint32_t rate = 12U + 6U * g->wave;
    if (rate > 60U) { rate = 60U; }
    if (rnd(g) % 1000U >= rate) { return; }

    unsigned slot = GAME_MAX_EGGS;
    for (unsigned i = 0; i < GAME_MAX_EGGS; i++) {
        if (!g->eggs[i].active) { slot = i; break; }
    }
    if (slot == GAME_MAX_EGGS) { return; }

    /* Chọn một cột ngẫu nhiên còn gà; con thấp nhất cột đó đẻ trứng. */
    const unsigned col0 = rnd(g) % GAME_CHICKEN_COLS;
    for (unsigned k = 0; k < GAME_CHICKEN_COLS; k++) {
        const unsigned c = (col0 + k) % GAME_CHICKEN_COLS;
        for (int r = GAME_CHICKEN_ROWS - 1; r >= 0; r--) {
            const unsigned i = (unsigned)r * GAME_CHICKEN_COLS + c;
            if ((g->chicken_alive & (1UL << i)) != 0U) {
                const int16_t fx = (int16_t)(g->formation_x_q8 >> 8);
                g->eggs[slot].active = true;
                g->eggs[slot].x = (int16_t)(game_chicken_x(fx, i) + (GAME_CHICKEN_W - GAME_EGG_W) / 2);
                g->eggs[slot].y = (int16_t)(game_chicken_y(g->formation_y, i) + GAME_CHICKEN_H);
                return;
            }
        }
    }
}

void game_step(game_t *g)
{
    if (g == NULL || g->state != GAME_PLAYING) { return; }
    g->frame++;

    /* 1. Người chơi */
    int16_t px = (int16_t)(g->player_x + g->player_vx);
    if (px < 0) { px = 0; }
    if (px > GAME_SCREEN_W - GAME_PLAYER_W) { px = GAME_SCREEN_W - GAME_PLAYER_W; }
    g->player_x = px;
    if (g->shot_cooldown > 0U) { g->shot_cooldown--; }
    if (g->invuln > 0U) { g->invuln--; }

    /* 2. Đạn */
    for (unsigned i = 0; i < GAME_MAX_BULLETS; i++) {
        game_obj_t *b = &g->bullets[i];
        if (!b->active) { continue; }
        b->y = (int16_t)(b->y - GAME_BULLET_SPEED);
        if (b->y + GAME_BULLET_H <= 0) { b->active = false; }
    }

    /* 3. Đội hình gà + va chạm đạn */
    move_formation(g);
    bullets_vs_chickens(g);

    /* 4. Hiệu ứng nổ */
    for (unsigned i = 0; i < GAME_MAX_BOOMS; i++) {
        if (g->booms[i].ttl > 0U) { g->booms[i].ttl--; }
    }

    /* 5. Hết gà: đợt mới nhanh hơn */
    if (g->chicken_alive == 0U) {
        g->score += GAME_SCORE_WAVE_BONUS * g->wave;
        if (g->wave < 99U) { g->wave++; }
        formation_reset(g);
        return;
    }

    /* 6. Gà chạm tầng người chơi: thua ngay */
    unsigned cmin, cmax, rmax;
    alive_bounds(g->chicken_alive, &cmin, &cmax, &rmax);
    const int16_t bottom = (int16_t)(g->formation_y + (int16_t)(rmax * GAME_CHICKEN_DY) + GAME_CHICKEN_H);
    if (bottom >= GAME_PLAYER_Y) {
        g->hp = 0U;
        g->state = GAME_OVER;
        g->player_vx = 0;
        return;
    }

    /* 7. Trứng rơi + va chạm người chơi */
    drop_egg(g);
    const int16_t egg_speed = (int16_t)(2 + (g->wave - 1U) / 3U);
    for (unsigned i = 0; i < GAME_MAX_EGGS; i++) {
        game_obj_t *e = &g->eggs[i];
        if (!e->active) { continue; }
        e->y = (int16_t)(e->y + (egg_speed > 4 ? 4 : egg_speed));
        if (e->y >= GAME_SCREEN_H) { e->active = false; continue; }
        if (g->invuln == 0U &&
            overlap(e->x, e->y, GAME_EGG_W, GAME_EGG_H,
                    (int16_t)(g->player_x + 2), (int16_t)(GAME_PLAYER_Y + 4),
                    GAME_PLAYER_W - 4, GAME_PLAYER_H - 4)) {
            e->active = false;
            lose_hp(g);
            if (g->state == GAME_OVER) { break; }
        }
    }
}

void game_get_snapshot(const game_t *g, game_snapshot_t *out)
{
    memset(out, 0, sizeof *out);
    out->state = g->state;
    out->score = g->score;
    out->frame = g->frame;
    out->player_x = (uint16_t)g->player_x;
    out->hp = g->hp;
    out->wave = g->wave;
    out->flags = 0U;
    if (g->invuln == 0U || ((g->invuln >> 2) & 1U) == 0U) { out->flags |= GAME_FLAG_PLAYER_VISIBLE; }
    if (((g->frame >> 3) & 1U) != 0U) { out->flags |= GAME_FLAG_CHICKEN_FRAME; }
    out->formation_x = (int16_t)(g->formation_x_q8 >> 8);
    out->formation_y = g->formation_y;
    out->chicken_alive = g->chicken_alive;
    for (unsigned i = 0; i < GAME_MAX_BULLETS; i++) {
        out->bullets[i].x = g->bullets[i].active ? g->bullets[i].x : GAME_POS_NONE;
        out->bullets[i].y = g->bullets[i].active ? g->bullets[i].y : 0;
    }
    for (unsigned i = 0; i < GAME_MAX_EGGS; i++) {
        out->eggs[i].x = g->eggs[i].active ? g->eggs[i].x : GAME_POS_NONE;
        out->eggs[i].y = g->eggs[i].active ? g->eggs[i].y : 0;
    }
    for (unsigned i = 0; i < GAME_MAX_BOOMS; i++) {
        out->booms[i].x = (g->booms[i].ttl > 0U) ? g->booms[i].x : GAME_POS_NONE;
        out->booms[i].y = (g->booms[i].ttl > 0U) ? g->booms[i].y : 0;
    }
}
