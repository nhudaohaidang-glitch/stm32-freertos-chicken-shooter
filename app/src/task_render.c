#include "task_render.h"
#include "task.h"
#include "app_cfg.h"
#include "app_ipc.h"
#include "bsp.h"
#include "ili9341.h"
#include "render_tft.h"
#include "oled_view.h"
#include <stdbool.h>

/*
 * TASK_RENDER (ưu tiên 2 - thấp nhất trong 3 task):
 *   - Chờ Task Notification từ GAME (timeout 100 ms), lấy snapshot mới nhất từ queue dài 1.
 *     Notification được gộp: nếu render chậm, các snapshot trung gian bị bỏ, nhưng
 *     render luôn so với KHUNG ĐÃ VẼ THỰC SỰ (s_drawn) nên không sót vùng cần xóa.
 *   - TFT: chỉ vẽ vùng thay đổi (render_tft). OLED: điểm/HP/trạng thái mỗi 100 ms.
 *   - Đánh giá hoạt động: mỗi 1 s đo FPS thực (số khung TFT vẽ xong), thời gian vẽ lâu
 *     nhất (DWT), heap trống hiện tại/thấp nhất, stack high-water mark từng task;
 *     hiển thị ngay trên OLED (không cần UART).
 * Lỗi màn hình (TFT/OLED) không làm dừng game: thử khởi tạo lại sau 1 s.
 */
static game_snapshot_t s_cur;
static game_snapshot_t s_drawn;
static bool s_have_drawn;
static bool s_tft_ok;
static bool s_oled_ok;

typedef struct {
    uint32_t frames;        /* khung đã vẽ trong cửa sổ 1 s */
    uint32_t draw_us_win;   /* thời gian vẽ lâu nhất trong cửa sổ hiện tại */
    uint32_t tft_errors;
    uint32_t oled_errors;
} render_stats_t;
static render_stats_t s_st;
/* Số đo của cửa sổ 1 s gần nhất: hiển thị trên OLED, đọc được bằng debugger. */
static oled_view_info_t s_info;

/* Chốt số đo mỗi 1 s cho phần "Đánh giá hoạt động". */
static void update_stats(void)
{
    s_info.fps = (uint16_t)s_st.frames;
    s_info.draw_us_max = s_st.draw_us_win;
    s_info.heap_free = (uint32_t)xPortGetFreeHeapSize();
    s_info.heap_min = (uint32_t)xPortGetMinimumEverFreeHeapSize();
    s_info.stack_free[0] = g_hmi_task ? (uint16_t)uxTaskGetStackHighWaterMark(g_hmi_task) : 0U;
    s_info.stack_free[1] = g_game_task ? (uint16_t)uxTaskGetStackHighWaterMark(g_game_task) : 0U;
    s_info.stack_free[2] = (uint16_t)uxTaskGetStackHighWaterMark(NULL);
    s_st.frames = 0U;
    s_st.draw_us_win = 0U;
}

static void try_init_displays(void)
{
    if (!s_tft_ok) {
        s_tft_ok = (ili9341_init(APP_TFT_TIMEOUT_MS) == HAL_OK);
        s_have_drawn = false;                     /* vẽ lại toàn màn sau khi init */
        if (!s_tft_ok) { s_st.tft_errors++; }
    }
    if (!s_oled_ok) {
        s_oled_ok = (oled_view_init(APP_OLED_ADDR, APP_OLED_TIMEOUT_MS) == HAL_OK);
        if (!s_oled_ok) { s_st.oled_errors++; }
    }
}

static void draw_tft(void)
{
    if (!s_tft_ok) { return; }
    const uint32_t t0 = bsp_cycles();
    const hal_status_t st = render_tft_draw(s_have_drawn ? &s_drawn : NULL, &s_cur, APP_TFT_TIMEOUT_MS);
    const uint32_t us = bsp_cycles_to_us(bsp_cycles() - t0);
    if (st != HAL_OK) {
        s_st.tft_errors++;
        s_have_drawn = false;                     /* không chắc màn đang hiển thị gì: vẽ lại hết */
        if (st == HAL_IO_ERROR || st == HAL_TIMEOUT) { s_tft_ok = false; }
        return;
    }
    s_drawn = s_cur;
    s_have_drawn = true;
    if (render_tft_last_stats()->rects > 0U) {
        s_st.frames++;
        if (us > s_st.draw_us_win) { s_st.draw_us_win = us; }
    }
}

static void draw_oled(void)
{
    if (!s_oled_ok) { return; }
    s_info.state = s_cur.state;
    s_info.score = s_cur.score;
    s_info.hp = s_cur.hp;
    s_info.wave = s_cur.wave;
    if (oled_view_update(&s_info, APP_OLED_TIMEOUT_MS) != HAL_OK) {
        s_st.oled_errors++;
        s_oled_ok = false;
    }
}

static void task_entry(void *argument)
{
    (void)argument;
    TickType_t now = xTaskGetTickCount();
    TickType_t next_stats = now + pdMS_TO_TICKS(APP_STATS_PERIOD_MS);
    TickType_t next_oled = now;
    TickType_t next_retry = now;
    bool have_snapshot = false;

    for (;;) {
        now = xTaskGetTickCount();
        if ((!s_tft_ok || !s_oled_ok) && (int32_t)(now - next_retry) >= 0) {
            try_init_displays();
            next_retry = now + pdMS_TO_TICKS(1000U);
        }

        if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(100U)) > 0U &&
            xQueueReceive(g_render_queue, &s_cur, 0) == pdPASS) {
            have_snapshot = true;
            draw_tft();
            bsp_led_toggle();                     /* nháy theo khung: nhìn thấy render còn sống */
        }

        now = xTaskGetTickCount();
        if ((int32_t)(now - next_stats) >= 0) {
            update_stats();
            next_stats += pdMS_TO_TICKS(APP_STATS_PERIOD_MS);
        }
        if (have_snapshot && (int32_t)(now - next_oled) >= 0) {
            draw_oled();
            next_oled = now + pdMS_TO_TICKS(APP_OLED_PERIOD_MS);
        }
    }
}

BaseType_t task_render_create(void)
{
    return xTaskCreate(task_entry, "RENDER", APP_RENDER_STACK_WORDS, NULL,
                       APP_RENDER_PRIO, &g_render_task);
}
