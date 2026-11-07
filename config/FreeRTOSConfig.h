#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H
/* Bootstrap assumption: HSI 8 MHz. BSP owner must validate or update this. */
#define configCPU_CLOCK_HZ 8000000UL
#define configTICK_RATE_HZ 1000U
#define configUSE_PREEMPTION 1
#define configUSE_TIME_SLICING 1
#define configMAX_PRIORITIES 5
#define configMINIMAL_STACK_SIZE 128U
#define configTOTAL_HEAP_SIZE (10U * 1024U)
#define configMAX_TASK_NAME_LEN 12
#define configUSE_16_BIT_TICKS 0
#define configIDLE_SHOULD_YIELD 1
#define configUSE_MUTEXES 1
#define configUSE_RECURSIVE_MUTEXES 0
#define configUSE_COUNTING_SEMAPHORES 0
#define configUSE_TASK_NOTIFICATIONS 1
#define configUSE_TIMERS 0
#define configUSE_IDLE_HOOK 0
#define configUSE_TICK_HOOK 0
#define configCHECK_FOR_STACK_OVERFLOW 2
#define configUSE_MALLOC_FAILED_HOOK 1
#define configSUPPORT_DYNAMIC_ALLOCATION 1
#define configSUPPORT_STATIC_ALLOCATION 0
#define configUSE_NEWLIB_REENTRANT 0
#define configPRIO_BITS 4
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY 15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5
#define configKERNEL_INTERRUPT_PRIORITY (15U << (8U - configPRIO_BITS))
#define configMAX_SYSCALL_INTERRUPT_PRIORITY (5U << (8U - configPRIO_BITS))
#define INCLUDE_vTaskDelay 1
#define INCLUDE_xTaskDelayUntil 1
#define INCLUDE_uxTaskGetStackHighWaterMark 1
#define INCLUDE_xTaskGetSchedulerState 1
#define INCLUDE_vTaskDelete 1
void app_assert_failed(const char *file, int line);
#define configASSERT(x) do { if (!(x)) app_assert_failed(__FILE__, __LINE__); } while (0)
#define vPortSVCHandler SVC_Handler
#define xPortPendSVHandler PendSV_Handler
#define xPortSysTickHandler SysTick_Handler
#endif
