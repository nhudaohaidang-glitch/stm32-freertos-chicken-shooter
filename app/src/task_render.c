#include "task_render.h"
#include "task.h"
#include "app_cfg.h"
#include "app_ipc.h"
/* Placeholder, not a completed task or a playable game. */
static void task_entry(void *argument)
{
    (void)argument;
    game_snapshot_t snapshot;
    for (;;) {
        if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(100U)) > 0U) {
            if (xQueueReceive(g_render_queue, &snapshot, 0) == pdPASS) {
                /* TODO: dirty-region TFT + OLED, measure actual FPS. */
                (void)snapshot;
            }
        }
    }
}
BaseType_t task_render_create(void)
{
    return xTaskCreate(task_entry, "RENDER", APP_RENDER_STACK_WORDS, NULL,
                       APP_RENDER_PRIO, &g_render_task);
}
