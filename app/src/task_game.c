#include "task_game.h"
#include "task.h"
#include "app_cfg.h"
#include "app_ipc.h"
#include "game_engine.h"

/*
 * TASK_GAME (ưu tiên 3, chu kỳ 20 ms bằng xTaskDelayUntil):
 *   1. Lấy hết sự kiện trong g_input_queue (không chờ) -> FSM.
 *   2. game_step(): chuyển động gà/đạn/trứng, va chạm, điểm, HP.
 *   3. Snapshot -> RENDER (queue dài 1 + notification).
 * Event-driven (FSM) kết hợp cập nhật định kỳ: gà vẫn bay khi không có input.
 * Đây là nơi DUY NHẤT ghi game state; s_game là static, chỉ task này truy cập.
 */
static game_t s_game;

static void task_entry(void *argument)
{
    (void)argument;
    game_snapshot_t snap;
    app_event_t ev;
    game_state_t last_state = GAME_MENU;
    uint32_t idle_publish = 0U;

    game_init(&s_game, (uint32_t)xTaskGetTickCount() ^ 0xC0FFEE11UL);
    game_get_snapshot(&s_game, &snap);
    (void)app_render_publish(&snap);

    TickType_t last = xTaskGetTickCount();
    for (;;) {
        while (xQueueReceive(g_input_queue, &ev, 0) == pdPASS) {
            game_handle_event(&s_game, &ev);     /* event-driven: FSM */
        }
        game_step(&s_game);                      /* định kỳ: chuyển động, va chạm */

        /* PLAYING: publish mỗi khung. Trạng thái tĩnh (MENU/PAUSED/GAME_OVER):
         * publish khi đổi trạng thái và nhắc lại mỗi 500 ms (RENDER có thể vừa khởi động). */
        const bool changed = (s_game.state != last_state);
        if (s_game.state == GAME_PLAYING || changed || ++idle_publish >= 25U) {
            idle_publish = 0U;
            game_get_snapshot(&s_game, &snap);
            (void)app_render_publish(&snap);
        }
        last_state = s_game.state;

        if (xTaskDelayUntil(&last, pdMS_TO_TICKS(APP_GAME_PERIOD_MS)) == pdFALSE) {
            g_ipc_stats.game_overruns++;   /* chu kỳ trước chạy quá 20 ms */
        }
    }
}

BaseType_t task_game_create(void)
{
    return xTaskCreate(task_entry, "GAME", APP_GAME_STACK_WORDS, NULL,
                       APP_GAME_PRIO, &g_game_task);
}
