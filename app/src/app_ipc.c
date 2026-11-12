#include "app_ipc.h"
#include "app_cfg.h"
QueueHandle_t g_input_queue;
QueueHandle_t g_render_queue;
TaskHandle_t g_render_task;
TaskHandle_t g_hmi_task;
TaskHandle_t g_game_task;
app_ipc_stats_t g_ipc_stats;

BaseType_t app_ipc_init(void)
{
    if (g_input_queue || g_render_queue) { return pdFAIL; }
    g_input_queue = xQueueCreate(APP_INPUT_QUEUE_LENGTH, sizeof(app_event_t));
    g_render_queue = xQueueCreate(1U, sizeof(game_snapshot_t));
    if (!g_input_queue || !g_render_queue) {
        if (g_input_queue) { vQueueDelete(g_input_queue); g_input_queue = NULL; }
        if (g_render_queue) { vQueueDelete(g_render_queue); g_render_queue = NULL; }
        return pdFAIL;
    }
    return pdPASS;
}

BaseType_t app_input_post(const app_event_t *ev)
{
    if (!ev || !g_input_queue) { return pdFAIL; }
    if (xQueueSend(g_input_queue, ev, 0) == pdPASS) { return pdPASS; }
    if (ev->kind == APP_MOVE_LEFT || ev->kind == APP_MOVE_RIGHT) {
        g_ipc_stats.input_drops++;
    } else {
        g_ipc_stats.input_retries++;
    }
    return pdFAIL;
}

BaseType_t app_render_publish(const game_snapshot_t *snapshot)
{
    if (!snapshot || !g_render_queue || !g_render_task) { return pdFAIL; }
    if (xQueueOverwrite(g_render_queue, snapshot) != pdPASS) { return pdFAIL; }
    g_ipc_stats.snapshots++;
    xTaskNotifyGive(g_render_task);
    return pdPASS;
}
