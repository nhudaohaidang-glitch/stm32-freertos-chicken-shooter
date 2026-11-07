#ifndef APP_IPC_H
#define APP_IPC_H
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "game_types.h"
extern QueueHandle_t g_input_queue; /* HMI -> GAME */
extern QueueHandle_t g_sound_queue; /* GAME -> HMI, buzzer owner */
extern QueueHandle_t g_render_queue; /* GAME -> RENDER: length 1 snapshot */
extern TaskHandle_t g_render_task;
/* Init once before task creation. No caller may recreate live IPC objects. */
BaseType_t app_ipc_init(void);
/* GAME only: copy snapshot then notify; RENDER never writes game state. */
BaseType_t app_render_publish(const game_snapshot_t *snapshot);
#endif
