#include "app_ipc.h"
#include "app_cfg.h"
QueueHandle_t g_input_queue;
QueueHandle_t g_sound_queue;
QueueHandle_t g_render_queue;
TaskHandle_t g_render_task;
BaseType_t app_ipc_init(void)
{
    if (g_input_queue || g_sound_queue || g_render_queue) { return pdFAIL; }
    g_input_queue = xQueueCreate(APP_INPUT_QUEUE_LENGTH, sizeof(app_event_t));
    g_sound_queue = xQueueCreate(APP_SOUND_QUEUE_LENGTH, sizeof(app_sound_t));
    g_render_queue = xQueueCreate(1U, sizeof(game_snapshot_t));
    if (!g_input_queue || !g_sound_queue || !g_render_queue) {
        if (g_input_queue) { vQueueDelete(g_input_queue); g_input_queue = NULL; }
        if (g_sound_queue) { vQueueDelete(g_sound_queue); g_sound_queue = NULL; }
        if (g_render_queue) { vQueueDelete(g_render_queue); g_render_queue = NULL; }
        return pdFAIL;
    }
    return pdPASS;
}
BaseType_t app_render_publish(const game_snapshot_t *snapshot)
{
    if (!snapshot || !g_render_queue || !g_render_task) { return pdFAIL; }
    if (xQueueOverwrite(g_render_queue, snapshot) != pdPASS) { return pdFAIL; }
    xTaskNotifyGive(g_render_task);
    return pdPASS;
}
