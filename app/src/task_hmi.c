#include <stdbool.h>
#include "task_hmi.h"
#include "task.h"
#include "app_cfg.h"
#include "app_ipc.h"
#include "hal_adc.h"
#include "hal_gpio.h"

/*
 * TASK_HMI (ưu tiên 3, chu kỳ 10 ms):
 *   - Joystick (ADC1): hiệu chuẩn tâm lúc khởi động, vùng chết, lượng tử hóa tốc độ 0..4.
 *     Chỉ gửi MOVE khi tốc độ đổi + gửi lại mỗi 250 ms (không làm ngập queue).
 *   - Nút Fire/Start (GPIO + EXTI): ISR chỉ ghi cạnh và đánh thức task bằng
 *     vTaskNotifyGiveFromISR; chống dội bằng N mẫu liên tiếp trong task (không delay trong ISR).
 * Chờ bằng ulTaskNotifyTake(timeout tới mốc chu kỳ kế): vừa định kỳ vừa phản ứng ngay với ngắt.
 */
#define BTN_COUNT 2U

typedef struct {
    uint8_t stable;        /* 1 = đang nhấn (đã chống dội) */
    uint8_t count;         /* số mẫu liên tiếp khác trạng thái stable */
    TickType_t repeat_at;  /* thời điểm auto-fire kế tiếp */
} button_t;

static button_t s_btn[BTN_COUNT];
static TickType_t s_last_sample;
static int16_t s_speed;           /* -4..4: âm = trái */
static TickType_t s_move_sent_at;
static bool s_shoot_pending;
static bool s_start_pending;
static uint16_t s_center_x = 2048U;
static uint32_t s_adc_errors;

static void calibrate_joystick(void)
{
    uint32_t sum = 0U;
    uint32_t n = 0U;
    for (unsigned i = 0; i < 16U; i++) {
        uint16_t x, y;
        if (hal_adc_read_joystick(&x, &y, APP_ADC_TIMEOUT_MS) == HAL_OK) {
            sum += x;
            n++;
        }
        vTaskDelay(pdMS_TO_TICKS(2U));
    }
    /* Tâm hợp lý của KY-023 nằm khoảng giữa; bỏ qua nếu lệch quá xa (đang bị gạt). */
    if (n > 0U) {
        const uint32_t c = sum / n;
        if (c > 1200U && c < 2900U) { s_center_x = (uint16_t)c; }
    }
}

/* Lượng tử hóa độ lệch X -> tốc độ -4..4 với vùng chết. */
static int16_t joystick_speed(uint16_t raw)
{
    int32_t d = (int32_t)raw - (int32_t)s_center_x;
#if APP_JOY_INVERT_X
    d = -d;
#endif
    const int32_t mag = (d < 0) ? -d : d;
    if (mag <= (int32_t)APP_JOY_DEADZONE) { return 0; }
    const int32_t span = ((d < 0) ? (int32_t)s_center_x : (4095 - (int32_t)s_center_x)) - (int32_t)APP_JOY_DEADZONE;
    int32_t v = 1 + ((mag - (int32_t)APP_JOY_DEADZONE) * GAME_PLAYER_MAX_SPEED) / ((span > 1) ? span : 1);
    if (v > GAME_PLAYER_MAX_SPEED) { v = GAME_PLAYER_MAX_SPEED; }
    return (int16_t)((d < 0) ? -v : v);
}

static void send_move(int16_t speed, TickType_t now)
{
    const app_event_t ev = {
        .kind = (speed < 0) ? APP_MOVE_LEFT : APP_MOVE_RIGHT,
        .value = (int16_t)((speed < 0) ? -speed : speed),
        .tick = (uint32_t)now,
    };
    if (app_input_post(&ev) == pdPASS) { s_move_sent_at = now; }
    /* Thất bại: lần sau tốc độ vẫn "khác đã gửi" nên sẽ gửi lại sau chu kỳ refresh. */
}

static void poll_joystick(TickType_t now)
{
    uint16_t x, y;
    if (hal_adc_read_joystick(&x, &y, APP_ADC_TIMEOUT_MS) != HAL_OK) {
        s_adc_errors++;
        return;
    }
    const int16_t sp = joystick_speed(x);
    if (sp != s_speed || (now - s_move_sent_at) >= pdMS_TO_TICKS(APP_MOVE_REFRESH_MS)) {
        s_speed = sp;
        send_move(sp, now);
    }
}

/* Một mẫu chống dội cho cả hai nút; trả bit nút vừa chuyển sang "nhấn". */
static uint8_t sample_buttons(TickType_t now)
{
    if ((now - s_last_sample) < pdMS_TO_TICKS(APP_BTN_MIN_SAMPLE_MS)) { return 0U; }
    s_last_sample = now;
    uint8_t raw = 0U;
    if (hal_gpio_read_buttons(&raw) != HAL_OK) { return 0U; }

    uint8_t pressed_edges = 0U;
    for (unsigned b = 0; b < BTN_COUNT; b++) {
        const uint8_t level = (uint8_t)((raw >> b) & 1U);
        button_t *bt = &s_btn[b];
        if (level == bt->stable) {
            bt->count = 0U;
            continue;
        }
        if (++bt->count >= APP_BTN_DEBOUNCE_SAMPLES) {
            bt->stable = level;
            bt->count = 0U;
            if (level != 0U) {
                pressed_edges |= (uint8_t)(1U << b);
                bt->repeat_at = now + pdMS_TO_TICKS(APP_FIRE_REPEAT_MS);
            }
        }
    }
    return pressed_edges;
}

static void handle_buttons(TickType_t now)
{
    const uint8_t edges = sample_buttons(now);
    if ((edges & HAL_GPIO_BTN_FIRE) != 0U) { s_shoot_pending = true; }
    if ((edges & HAL_GPIO_BTN_START_PAUSE) != 0U) { s_start_pending = true; }

    /* Giữ Fire: bắn tự động (GAME còn cooldown riêng). */
    button_t *fire = &s_btn[0];
    if (fire->stable != 0U && (int32_t)(now - fire->repeat_at) >= 0) {
        fire->repeat_at = now + pdMS_TO_TICKS(APP_FIRE_REPEAT_MS);
        s_shoot_pending = true;
    }

    /* SHOOT/START_PAUSE không bị bỏ âm thầm: queue đầy thì giữ cờ, gửi lại chu kỳ sau. */
    if (s_start_pending) {
        const app_event_t ev = { .kind = APP_START_PAUSE, .value = 0, .tick = (uint32_t)now };
        if (app_input_post(&ev) == pdPASS) { s_start_pending = false; }
    }
    if (s_shoot_pending) {
        const app_event_t ev = { .kind = APP_SHOOT, .value = 0, .tick = (uint32_t)now };
        if (app_input_post(&ev) == pdPASS) { s_shoot_pending = false; }
    }
}

static void task_entry(void *argument)
{
    (void)argument;
    calibrate_joystick();
    (void)hal_gpio_exti_enable(xTaskGetCurrentTaskHandle());

    const TickType_t period = pdMS_TO_TICKS(APP_HMI_PERIOD_MS);
    TickType_t next = xTaskGetTickCount();
    s_move_sent_at = next;
    for (;;) {
        TickType_t now = xTaskGetTickCount();
        const TickType_t wait = ((int32_t)(next - now) > 0) ? (next - now) : 0U;
        /* Ngắt nút đánh thức sớm; hết thời gian = tới mốc chu kỳ. */
        const uint32_t woke_by_isr = ulTaskNotifyTake(pdTRUE, wait);
        now = xTaskGetTickCount();
        (void)hal_gpio_take_edges();   /* cạnh chỉ dùng để đánh thức; trạng thái lấy từ mẫu */

        if (woke_by_isr != 0U && (int32_t)(now - next) < 0) {
            handle_buttons(now);      /* lấy mẫu sớm, chu kỳ không đổi */
            continue;
        }
        next += period;
        if ((int32_t)(now - next) > (int32_t)(4U * period)) { next = now + period; } /* bị trễ lâu: bắt nhịp lại */
        poll_joystick(now);
        handle_buttons(now);
    }
}

BaseType_t task_hmi_create(void)
{
    return xTaskCreate(task_entry, "HMI", APP_HMI_STACK_WORDS, NULL,
                       APP_HMI_PRIO, &g_hmi_task);
}
