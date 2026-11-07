#include "bsp.h"
#include "app_ipc.h"
#include "task_hmi.h"
#include "task_game.h"
#include "task_render.h"
/* Integration owner only. Feature-task PRs must not modify this file. */
int main(void)
{
    /* The BSP deliberately fails until board bring-up is implemented. */
    if (bsp_system_init() != HAL_OK) { bsp_panic(); }
    if (app_ipc_init() != pdPASS) { bsp_panic(); }
    if (task_render_create() != pdPASS || task_hmi_create() != pdPASS ||
        task_game_create() != pdPASS) { bsp_panic(); }
    vTaskStartScheduler();
    bsp_panic();
}
