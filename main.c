#include "bsp.h"
#include "app_ipc.h"
#include "task_hmi.h"
#include "task_game.h"
#include "task_render.h"
#include "hal_gpio.h"
#include "hal_adc.h"
#include "hal_i2c.h"
#include "hal_spi.h"
/* Integration owner only. Feature-task PRs must not modify this file.
 *
 * Thứ tự khởi động: BSP (clock 72 MHz) -> HAL -> IPC -> task -> scheduler.
 * Ba ngoại vi chính: SPI1 (TFT), I2C1 (OLED), ADC1 (joystick); GPIO cho nút.
 * Thiết bị cần delay dài (ILI9341 reset 120 ms, SSD1306) được khởi tạo trong
 * TASK_RENDER bằng vTaskDelay, không chặn CPU trước scheduler.
 * Lỗi khởi động: bsp_panic() -> LED PC13 nháy nhanh. */
int main(void)
{
    /* 1. Clock + NVIC. PLL lỗi vẫn chạy được ở HSI 8 MHz (game chậm hơn). */
    (void)bsp_system_init();

    /* 2. HAL: các driver bus tạo mutex trong heap FreeRTOS. */
    if (hal_gpio_init() != HAL_OK || hal_adc_init() != HAL_OK ||
        hal_i2c_init() != HAL_OK || hal_spi_init() != HAL_OK) {
        bsp_panic();
    }

    /* 3. IPC: tạo queue trước khi tạo task. */
    if (app_ipc_init() != pdPASS) { bsp_panic(); }

    /* 4. Task: RENDER trước để g_render_task có trước khi GAME publish. */
    if (task_render_create() != pdPASS || task_hmi_create() != pdPASS ||
        task_game_create() != pdPASS) {
        bsp_panic();
    }

    /* 5. Scheduler: không bao giờ trả về nếu đủ heap cho Idle task. */
    vTaskStartScheduler();
    bsp_panic();
}
