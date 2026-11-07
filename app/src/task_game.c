#include "task_game.h"
#include "task.h"
#include "app_cfg.h"
#include "app_ipc.h"
/* Placeholder, not a completed task or a playable game. */
static void task_entry(void *argument)
{
    (void)argument;
    TickType_t last = xTaskGetTickCount();
    for (;;) {
        /* TODO: implement task logic on the phase-2 feature branch. */
        vTaskDelayUntil(&last, pdMS_TO_TICKS(APP_GAME_PERIOD_MS));
    }
}
BaseType_t task_game_create(void)
{
    return xTaskCreate(task_entry, "GAME", APP_GAME_STACK_WORDS, NULL,
                       APP_GAME_PRIO, NULL);
}
