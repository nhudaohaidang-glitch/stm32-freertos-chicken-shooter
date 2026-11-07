#include "FreeRTOS.h"
#include "task.h"
#include "bsp.h"
void app_assert_failed(const char *file, int line)
{
    (void)file; (void)line;
    bsp_panic();
}
void vApplicationMallocFailedHook(void) { bsp_panic(); }
void vApplicationStackOverflowHook(TaskHandle_t task, char *name)
{
    (void)task; (void)name;
    bsp_panic();
}
