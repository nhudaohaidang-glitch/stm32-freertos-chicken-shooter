#include "buzzer.h"
#include "hal_pwm.h"
#include <stddef.h>

#define DUTY 500U   /* buzzer thụ động to nhất ở 50% */

typedef struct { uint16_t freq_hz; uint16_t dur_ms; } note_t; /* freq 0 = nghỉ */
typedef struct { const note_t *notes; uint8_t count; uint8_t prio; } fx_t;

static const note_t k_shoot[] = { { 1800, 25 }, { 1200, 25 } };
static const note_t k_hit[]   = { { 900, 40 }, { 1400, 40 }, { 2000, 60 } };
static const note_t k_over[]  = { { 784, 150 }, { 659, 150 }, { 523, 150 }, { 0, 50 }, { 392, 350 } };

static const fx_t k_fx[BUZZER_FX_COUNT] = {
    [BUZZER_FX_SHOOT]     = { k_shoot, (uint8_t)(sizeof k_shoot / sizeof k_shoot[0]), 1U },
    [BUZZER_FX_HIT]       = { k_hit,   (uint8_t)(sizeof k_hit / sizeof k_hit[0]),     2U },
    [BUZZER_FX_GAME_OVER] = { k_over,  (uint8_t)(sizeof k_over / sizeof k_over[0]),   3U },
};

static const fx_t *s_cur;
static uint8_t s_idx;
static uint32_t s_note_end;

static hal_status_t start_note(uint32_t now_ms)
{
    const note_t *n = &s_cur->notes[s_idx];
    s_note_end = now_ms + n->dur_ms;
    return hal_pwm_set_buzzer(n->freq_hz, (n->freq_hz != 0U) ? DUTY : 0U);
}

void buzzer_init(void)
{
    s_cur = NULL;
    (void)hal_pwm_set_buzzer(0U, 0U);
}

hal_status_t buzzer_play(buzzer_fx_t fx, uint32_t now_ms)
{
    if ((unsigned)fx >= (unsigned)BUZZER_FX_COUNT) { return HAL_INVALID_ARG; }
    const fx_t *next = &k_fx[fx];
    if (s_cur != NULL && s_cur->prio > next->prio) { return HAL_OK; } /* giữ hiệu ứng quan trọng hơn */
    s_cur = next;
    s_idx = 0U;
    return start_note(now_ms);
}

hal_status_t buzzer_update(uint32_t now_ms)
{
    if (s_cur == NULL) { return HAL_OK; }
    if ((int32_t)(now_ms - s_note_end) < 0) { return HAL_OK; } /* an toàn khi tràn số */
    s_idx++;
    if (s_idx >= s_cur->count) {
        s_cur = NULL;
        return hal_pwm_set_buzzer(0U, 0U);
    }
    return start_note(now_ms);
}

void buzzer_stop(void)
{
    s_cur = NULL;
    (void)hal_pwm_set_buzzer(0U, 0U);
}

uint8_t buzzer_busy(void)
{
    return (s_cur != NULL) ? 1U : 0U;
}
