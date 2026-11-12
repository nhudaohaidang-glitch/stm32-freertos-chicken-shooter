#ifndef APP_IPC_H
#define APP_IPC_H
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "game_types.h"
/*
 * IPC giữa các task:
 *   EXTI --vTaskNotifyGiveFromISR----------------------> HMI
 *   HMI  --g_input_queue (app_event_t, 16)--------------> GAME
 *   GAME --g_render_queue (snapshot, dài 1, overwrite)
 *        + xTaskNotifyGive(g_render_task) ---------------> RENDER
 */
extern QueueHandle_t g_input_queue; /* HMI -> GAME */
extern QueueHandle_t g_render_queue; /* GAME -> RENDER: length 1 snapshot */
extern TaskHandle_t g_render_task;
/* Handle để đo stack high-water mark; mỗi task_xxx_create() tự gán handle của mình. */
extern TaskHandle_t g_hmi_task;
extern TaskHandle_t g_game_task;

/* Bộ đếm chẩn đoán. Mỗi trường chỉ do MỘT task ghi (ghi chú bên cạnh) nên không cần khóa. */
typedef struct {
    volatile uint32_t input_drops;    /* HMI: queue đầy, MOVE bị bỏ (sẽ gửi lại) */
    volatile uint32_t input_retries;  /* HMI: SHOOT/START_PAUSE phải gửi lại chu kỳ sau */
    volatile uint32_t snapshots;      /* GAME: số snapshot đã publish */
    volatile uint32_t game_overruns;  /* GAME: chu kỳ 20 ms bị trễ */
} app_ipc_stats_t;
extern app_ipc_stats_t g_ipc_stats;

/* Init once before task creation. No caller may recreate live IPC objects. */
BaseType_t app_ipc_init(void);
/* HMI: gửi không chờ. pdFAIL khi queue đầy (đã tăng bộ đếm tương ứng);
 * người gọi tự quyết định gửi lại (SHOOT/START_PAUSE) hay bỏ (MOVE). */
BaseType_t app_input_post(const app_event_t *ev);
/* GAME only: copy snapshot then notify; RENDER never writes game state. */
BaseType_t app_render_publish(const game_snapshot_t *snapshot);
#endif
