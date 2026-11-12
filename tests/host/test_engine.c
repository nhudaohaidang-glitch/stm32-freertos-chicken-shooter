/*
 * Unit test game engine trên PC (không cần kit, không cần FreeRTOS).
 *   make -C tests/host test
 * Kiểm tra FSM theo bảng ARCHITECTURE, bỏ qua sự kiện sai trạng thái, pause không
 * làm game chạy tiếp, bắn trúng cộng điểm, mất máu -> GAME_OVER, restart reset ván,
 * hết gà -> đợt mới, gà chạm đáy -> thua, snapshot không rò dữ liệu ô trống.
 */
#include "game_engine.h"
#include <stdio.h>
#include <string.h>

static int s_fail;
static int s_pass;
#define CHECK(cond) do { if (cond) { s_pass++; } else { s_fail++; \
    printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)

static void send(game_t *g, app_event_kind_t k, int16_t v)
{
    const app_event_t ev = { .kind = k, .value = v, .tick = 1234U };
    game_handle_event(g, &ev);
}

static int active_bullets(const game_t *g)
{
    int n = 0;
    for (unsigned i = 0; i < GAME_MAX_BULLETS; i++) { n += g->bullets[i].active; }
    return n;
}

static void test_fsm(void)
{
    printf("test_fsm\n");
    game_t g;
    game_init(&g, 1U);
    CHECK(g.state == GAME_MENU);
    /* MENU bỏ qua SHOOT/MOVE/PAUSE */
    send(&g, APP_SHOOT, 0);
    send(&g, APP_MOVE_LEFT, 3);
    send(&g, APP_PAUSE, 0);
    CHECK(g.state == GAME_MENU);
    CHECK(g.ignored_events == 3U);
    /* MENU -> PLAYING */
    send(&g, APP_START_PAUSE, 0);
    CHECK(g.state == GAME_PLAYING && g.hp == GAME_MAX_HP && g.score == 0U && g.wave == 1U);
    /* PLAYING -> PAUSED -> PLAYING */
    send(&g, APP_START_PAUSE, 0);
    CHECK(g.state == GAME_PAUSED);
    send(&g, APP_SHOOT, 0);              /* bị bỏ qua khi PAUSED */
    CHECK(g.state == GAME_PAUSED);
    send(&g, APP_RESUME, 0);
    CHECK(g.state == GAME_PLAYING);
    send(&g, APP_PAUSE, 0);
    CHECK(g.state == GAME_PAUSED);
    send(&g, APP_START_PAUSE, 0);
    CHECK(g.state == GAME_PLAYING);
    /* GAME_OVER -> RESTART */
    send(&g, APP_GAME_OVER, 0);
    CHECK(g.state == GAME_OVER && g.hp == 0U);
    send(&g, APP_SHOOT, 0);
    CHECK(g.state == GAME_OVER);
    send(&g, APP_RESTART, 0);
    CHECK(g.state == GAME_PLAYING && g.hp == GAME_MAX_HP && g.score == 0U);
    /* MENU chấp nhận APP_START */
    game_init(&g, 2U);
    send(&g, APP_START, 0);
    CHECK(g.state == GAME_PLAYING);
}

/* Bảng chuyển trạng thái đầy đủ: 4 trạng thái x 11 sự kiện = 44 ô, khớp ARCHITECTURE.md.
 * Trạng thái sau mỗi cặp (state, event); "giữ nguyên" = sự kiện bị bỏ qua (hoặc chỉ đổi dữ liệu). */
static void test_fsm_table(void)
{
    printf("test_fsm_table\n");
    static const char *const sn[] = { "MENU", "PLAYING", "PAUSED", "GAME_OVER" };
    static const char *const en[] = { "MOVE_LEFT", "MOVE_RIGHT", "SHOOT", "START_PAUSE", "START",
                                      "PAUSE", "RESUME", "RESTART", "HIT", "LOSE_HP", "GAME_OVER" };
    /* expected[state][event] */
    static const game_state_t expected[4][11] = {
        /* MENU      */ { GAME_MENU, GAME_MENU, GAME_MENU, GAME_PLAYING, GAME_PLAYING,
                          GAME_MENU, GAME_MENU, GAME_MENU, GAME_MENU, GAME_MENU, GAME_MENU },
        /* PLAYING   */ { GAME_PLAYING, GAME_PLAYING, GAME_PLAYING, GAME_PAUSED, GAME_PLAYING,
                          GAME_PAUSED, GAME_PLAYING, GAME_PLAYING, GAME_PLAYING, GAME_PLAYING, GAME_OVER },
        /* PAUSED    */ { GAME_PAUSED, GAME_PAUSED, GAME_PAUSED, GAME_PLAYING, GAME_PAUSED,
                          GAME_PAUSED, GAME_PLAYING, GAME_PAUSED, GAME_PAUSED, GAME_PAUSED, GAME_PAUSED },
        /* GAME_OVER */ { GAME_OVER, GAME_OVER, GAME_OVER, GAME_PLAYING, GAME_OVER,
                          GAME_OVER, GAME_OVER, GAME_PLAYING, GAME_OVER, GAME_OVER, GAME_OVER },
    };
    for (int st = 0; st < 4; st++) {
        for (int ev = 0; ev <= (int)APP_GAME_OVER; ev++) {
            game_t g;
            game_init(&g, 11U);
            if (st >= GAME_PLAYING) { send(&g, APP_START, 0); }
            if (st == GAME_PAUSED) { send(&g, APP_PAUSE, 0); }
            if (st == GAME_OVER) { send(&g, APP_GAME_OVER, 0); }
            if ((int)g.state != st) { CHECK(0); continue; }
            const uint32_t ign = g.ignored_events;
            send(&g, (app_event_kind_t)ev, 1);
            const int ok = g.state == expected[st][ev];
            if (!ok) { printf("  %s + %s -> %s (expected %s)\n", sn[st], en[ev], sn[g.state], sn[expected[st][ev]]); }
            CHECK(ok);
            /* Sự kiện không làm gì ở trạng thái đó phải được đếm là bị bỏ qua. */
            const int handled = !(st == GAME_MENU && (ev == APP_START || ev == APP_START_PAUSE)) &&
                                !(st == GAME_PLAYING && ev != APP_START && ev != APP_RESUME && ev != APP_RESTART) &&
                                !(st == GAME_PAUSED && (ev == APP_RESUME || ev == APP_START_PAUSE)) &&
                                !(st == GAME_OVER && (ev == APP_RESTART || ev == APP_START_PAUSE));
            CHECK((g.ignored_events == ign + 1U) == (handled != 0));
        }
    }
}

static void test_pause_freezes(void)
{
    printf("test_pause_freezes\n");
    game_t g;
    game_init(&g, 3U);
    send(&g, APP_START, 0);
    for (int i = 0; i < 10; i++) { game_step(&g); }
    send(&g, APP_PAUSE, 0);
    game_snapshot_t a, b;
    game_get_snapshot(&g, &a);
    for (int i = 0; i < 500; i++) { game_step(&g); }
    game_get_snapshot(&g, &b);
    CHECK(memcmp(&a, &b, sizeof a) == 0);
    send(&g, APP_RESUME, 0);
    game_step(&g);
    game_get_snapshot(&g, &b);
    CHECK(b.frame == a.frame + 1U);      /* không nhảy cóc sau pause */
}

static void test_move_and_clamp(void)
{
    printf("test_move_and_clamp\n");
    game_t g;
    game_init(&g, 4U);
    send(&g, APP_START, 0);
    const int16_t x0 = g.player_x;
    send(&g, APP_MOVE_RIGHT, 3);
    game_step(&g);
    CHECK(g.player_x == x0 + 3);
    send(&g, APP_MOVE_LEFT, 99);         /* vượt max -> kẹp về 4 */
    game_step(&g);
    CHECK(g.player_x == x0 + 3 - GAME_PLAYER_MAX_SPEED);
    for (int i = 0; i < 200; i++) { game_step(&g); if (g.state != GAME_PLAYING) { break; } }
    CHECK(g.player_x >= 0);
    send(&g, APP_MOVE_RIGHT, 4);
    for (int i = 0; i < 200 && g.state == GAME_PLAYING; i++) { game_step(&g); }
    CHECK(g.player_x <= GAME_SCREEN_W - GAME_PLAYER_W);
    send(&g, APP_MOVE_RIGHT, 0);
    const int16_t x1 = g.player_x;
    if (g.state == GAME_PLAYING) { game_step(&g); CHECK(g.player_x == x1); }
}

static void test_shoot_hits_chicken(void)
{
    printf("test_shoot_hits_chicken\n");
    game_t g;
    game_init(&g, 5U);
    send(&g, APP_START, 0);
    g.rng = 0xFFFFFFFFU;                 /* hạt giống cố định */
    /* Đặt tàu dưới cột 0 của đội hình. */
    g.player_x = (int16_t)((g.formation_x_q8 >> 8) + (GAME_CHICKEN_W - GAME_PLAYER_W) / 2);
    send(&g, APP_SHOOT, 0);
    CHECK(active_bullets(&g) == 1);
    send(&g, APP_SHOOT, 0);
    CHECK(active_bullets(&g) == 1);      /* cooldown: phát thứ hai bị chặn */
    const uint32_t alive0 = g.chicken_alive;
    for (int i = 0; i < 80 && g.chicken_alive == alive0; i++) {
        g.formation_dir = 0;             /* giữ đội hình đứng yên cho phép thử xác định */
        game_step(&g);
        for (unsigned e = 0; e < GAME_MAX_EGGS; e++) { g.eggs[e].active = false; }
    }
    CHECK(g.chicken_alive != alive0);
    CHECK(g.score == GAME_SCORE_PER_CHICKEN);
    /* Con bị trúng là con thấp nhất của cột 0 (hàng cuối). */
    CHECK((g.chicken_alive & (1UL << ((GAME_CHICKEN_ROWS - 1) * GAME_CHICKEN_COLS))) == 0U);
    game_snapshot_t sn;
    game_get_snapshot(&g, &sn);
    int booms = 0;
    for (unsigned i = 0; i < GAME_MAX_BOOMS; i++) { booms += sn.booms[i].x != GAME_POS_NONE; }
    CHECK(booms == 1);
}

static void test_lose_hp_to_game_over(void)
{
    printf("test_lose_hp_to_game_over\n");
    game_t g;
    game_init(&g, 6U);
    send(&g, APP_START, 0);
    send(&g, APP_LOSE_HP, 0);
    CHECK(g.hp == GAME_MAX_HP - 1 && g.state == GAME_PLAYING);
    send(&g, APP_LOSE_HP, 0);
    send(&g, APP_LOSE_HP, 0);
    CHECK(g.hp == 0U && g.state == GAME_OVER);

    /* Trứng rơi trúng tàu: mất 1 HP rồi bất tử tạm thời */
    game_init(&g, 7U);
    send(&g, APP_START, 0);
    g.eggs[0].active = true;
    g.eggs[0].x = (int16_t)(g.player_x + 6);
    g.eggs[0].y = GAME_PLAYER_Y;
    game_step(&g);
    CHECK(g.hp == GAME_MAX_HP - 1 && g.invuln > 0U);
    g.eggs[1].active = true;
    g.eggs[1].x = (int16_t)(g.player_x + 6);
    g.eggs[1].y = GAME_PLAYER_Y;
    game_step(&g);
    CHECK(g.hp == GAME_MAX_HP - 1);      /* đang bất tử */
}

static void test_wave_and_invasion(void)
{
    printf("test_wave_and_invasion\n");
    game_t g;
    game_init(&g, 8U);
    send(&g, APP_START, 0);
    g.chicken_alive = 1U;                /* còn một con */
    g.bullets[0].active = true;
    g.bullets[0].x = (int16_t)((g.formation_x_q8 >> 8) + 7);
    g.bullets[0].y = (int16_t)(g.formation_y + 8);
    game_step(&g);
    CHECK(g.wave == 2U);
    CHECK(g.chicken_alive == (1UL << GAME_CHICKEN_COUNT) - 1UL);
    CHECK(g.formation_y == GAME_FORMATION_TOP);
    CHECK(g.score == GAME_SCORE_PER_CHICKEN + GAME_SCORE_WAVE_BONUS);

    /* Gà xuống tới tầng người chơi -> GAME_OVER */
    game_init(&g, 9U);
    send(&g, APP_START, 0);
    int steps = 0;
    while (g.state == GAME_PLAYING && steps < 100000) {
        g.invuln = 255U;                 /* bỏ qua trứng để chỉ thử xâm lược */
        game_step(&g);
        steps++;
    }
    CHECK(g.state == GAME_OVER && g.hp == 0U);
    printf("  formation reached player after %d frames (%.1f s)\n", steps, steps * 0.02);
}

static void test_long_random_run(void)
{
    printf("test_long_random_run\n");
    /* Chạy ngẫu nhiên 200.000 khung: bất biến luôn đúng, không treo. */
    game_t g;
    game_init(&g, 42U);
    uint32_t r = 99U;
    unsigned games = 0U, max_wave = 0U;
    for (int i = 0; i < 200000; i++) {
        r = r * 1103515245U + 12345U;
        const unsigned k = (r >> 16) % 16U;
        if (k < 4U) { send(&g, (k & 1U) ? APP_MOVE_LEFT : APP_MOVE_RIGHT, (int16_t)(k + 1U)); }
        else if (k < 10U) { send(&g, APP_SHOOT, 0); }
        else if (k == 10U && (r & 0x3F00U) == 0U) { send(&g, APP_START_PAUSE, 0); }
        if (g.state != GAME_PLAYING && g.state != GAME_PAUSED) { send(&g, APP_START_PAUSE, 0); games++; }
        if (g.state == GAME_PAUSED && (r & 0x700U) == 0U) { send(&g, APP_RESUME, 0); }
        game_step(&g);
        if (g.wave > max_wave) { max_wave = g.wave; }
        if (!(g.hp <= GAME_MAX_HP)) { CHECK(0); break; }
        if (!(g.player_x >= 0 && g.player_x <= GAME_SCREEN_W - GAME_PLAYER_W)) { CHECK(0); break; }
        if ((g.chicken_alive >> GAME_CHICKEN_COUNT) != 0U) { CHECK(0); break; }
        for (unsigned b = 0; b < GAME_MAX_BULLETS; b++) {
            if (g.bullets[b].active && g.bullets[b].y + GAME_BULLET_H <= 0) { CHECK(0); }
        }
    }
    CHECK(games > 0U);
    printf("  %u games, max wave %u\n", games, max_wave);
}

int main(void)
{
    test_fsm();
    test_fsm_table();
    test_pause_freezes();
    test_move_and_clamp();
    test_shoot_hits_chicken();
    test_lose_hp_to_game_over();
    test_wave_and_invasion();
    test_long_random_run();
    printf("engine tests: %d passed, %d failed\n", s_pass, s_fail);
    return s_fail == 0 ? 0 : 1;
}
