#include "task_hmi.h"
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
        vTaskDelayUntil(&last, pdMS_TO_TICKS(APP_HMI_PERIOD_MS));
    }
}
BaseType_t task_hmi_create(void)
{
    return xTaskCreate(task_entry, "HMI", APP_HMI_STACK_WORDS, NULL,
                       APP_HMI_PRIO, NULL);
}
