#ifndef BUZZER_H
#define BUZZER_H
#include <stdint.h>
#include "hal_status.h"
/*
 * Trình phát hiệu ứng âm thanh không blocking trên hal_pwm.
 * TASK_HMI gọi buzzer_play() khi nhận app_sound_t từ g_sound_queue và gọi
 * buzzer_update() mỗi chu kỳ (10 ms). Không có delay/busy-wait bên trong.
 * Enum riêng để lớp HAL không phụ thuộc app/; HMI ánh xạ 1-1:
 *   SOUND_SHOOT -> BUZZER_FX_SHOOT, SOUND_HIT -> BUZZER_FX_HIT,
 *   SOUND_GAME_OVER -> BUZZER_FX_GAME_OVER.
 * Ưu tiên: GAME_OVER > HIT > SHOOT; hiệu ứng thấp hơn không cắt hiệu ứng cao
 * đang phát. Một chủ sở hữu (TASK_HMI).
 */
typedef enum {
    BUZZER_FX_SHOOT,
    BUZZER_FX_HIT,
    BUZZER_FX_GAME_OVER,
    BUZZER_FX_COUNT
} buzzer_fx_t;

/* Gọi sau hal_pwm_init(). */
void buzzer_init(void);
/* now_ms: thời gian hiện tại (ví dụ xTaskGetTickCount() * portTICK_PERIOD_MS). */
hal_status_t buzzer_play(buzzer_fx_t fx, uint32_t now_ms);
hal_status_t buzzer_update(uint32_t now_ms);
void buzzer_stop(void);
/* 1 nếu đang phát. */
uint8_t buzzer_busy(void);
#endif
