#include "oled_view.h"
#include "ssd1306.h"
#include <stdbool.h>

/* Trái tim 7x6, bit0 = hàng trên (cùng quy ước cột với font5x7). */
static const uint8_t k_heart[7] = { 0x0E, 0x1F, 0x3E, 0x7C, 0x3E, 0x1F, 0x0E };

static bool s_ready;

static char *put_text(char *p, const char *s)
{
    while (*s != '\0') { *p++ = *s++; }
    *p = '\0';
    return p;
}

const char *oled_view_state_name(game_state_t st)
{
    switch (st) {
    case GAME_MENU:    return "MENU";
    case GAME_PLAYING: return "PLAYING";
    case GAME_PAUSED:  return "PAUSED";
    case GAME_OVER:    return "GAME OVER";
    default:           return "?";
    }
}

char *oled_view_put_u32(char *buf, uint32_t v, unsigned width)
{
    char t[10];
    unsigned n = 0U;
    do { t[n++] = (char)('0' + v % 10U); v /= 10U; } while (v != 0U && n < sizeof t);
    while (n < width && n < sizeof t) { t[n++] = '0'; }
    while (n > 0U) { *buf++ = t[--n]; }
    *buf = '\0';
    return buf;
}

static void draw_heart(int16_t x, int16_t y, bool full)
{
    for (unsigned cx = 0; cx < sizeof k_heart; cx++) {
        for (unsigned cy = 0; cy < 7U; cy++) {
            bool on = ((k_heart[cx] >> cy) & 1U) != 0U;
            if (!full) {
                /* Tim rỗng: chỉ giữ viền. */
                const bool inner = (cx > 0U && cx < 6U && cy > 0U && cy < 6U) &&
                                   ((k_heart[cx - 1U] >> cy) & 1U) && ((k_heart[cx + 1U] >> cy) & 1U) &&
                                   ((k_heart[cx] >> (cy - 1U)) & 1U) && ((k_heart[cx] >> (cy + 1U)) & 1U);
                if (inner) { on = false; }
            }
            ssd1306_draw_pixel((int16_t)(x + (int16_t)cx), (int16_t)(y + (int16_t)cy), on);
        }
    }
}

hal_status_t oled_view_init(uint8_t addr7, uint32_t timeout_ms)
{
    s_ready = false;
    const hal_status_t st = ssd1306_init(addr7, timeout_ms);
    if (st == HAL_OK) { s_ready = true; }
    return st;
}

hal_status_t oled_view_update(const oled_view_info_t *info, uint32_t timeout_ms)
{
    if (info == NULL) { return HAL_INVALID_ARG; }
    if (!s_ready) { return HAL_IO_ERROR; }
    char buf[24];
    char *p;

    /* Không xóa nền: mọi chuỗi có độ dài cố định và draw_text ghi đè cả ô 6x8,
     * nên pixel chỉ đổi khi nội dung đổi -> driver chỉ gửi page thật sự thay đổi. */

    /* page 0: trạng thái (đệm tới 9 ký tự) + đợt */
    const char *name = oled_view_state_name(info->state);
    p = buf;
    for (unsigned i = 0; i < 9U; i++) { *p++ = (*name != '\0') ? *name++ : ' '; }
    *p = '\0';
    (void)ssd1306_draw_text(0, 0, buf, 1U);
    buf[0] = 'W';
    (void)oled_view_put_u32(&buf[1], (info->wave > 99U) ? 99U : info->wave, 2U);
    (void)ssd1306_draw_text((int16_t)(SSD1306_WIDTH - 6U * 3U), 0, buf, 1U);

    /* page 1: nhãn; page 2..3: điểm cỡ 2 (6 chữ số) */
    (void)ssd1306_draw_text(0, 8, "SCORE", 1U);
    (void)oled_view_put_u32(buf, info->score % 1000000UL, 6U);
    (void)ssd1306_draw_text(0, 16, buf, 2U);

    /* page 4: HP */
    (void)ssd1306_draw_text(0, 32, "HP", 1U);
    for (unsigned i = 0; i < GAME_MAX_HP; i++) {
        draw_heart((int16_t)(16 + (int16_t)(i * 10U)), 32, i < info->hp);
    }

    /* page 5..7: số đo đánh giá (độ dài cố định để không để lại ký tự cũ) */
    p = put_text(buf, "FPS ");
    p = oled_view_put_u32(p, (info->fps > 99U) ? 99U : info->fps, 2U);
    p = put_text(p, " T ");
    p = oled_view_put_u32(p, (info->draw_us_max > 99999U) ? 99999U : info->draw_us_max, 5U);
    (void)put_text(p, "US");
    (void)ssd1306_draw_text(0, 40, buf, 1U);

    p = put_text(buf, "HEAP ");
    p = oled_view_put_u32(p, (info->heap_free > 99999U) ? 99999U : info->heap_free, 5U);
    p = put_text(p, "/");
    (void)oled_view_put_u32(p, (info->heap_min > 99999U) ? 99999U : info->heap_min, 5U);
    (void)ssd1306_draw_text(0, 48, buf, 1U);

    p = put_text(buf, "STK ");
    for (unsigned i = 0; i < 3U; i++) {
        p = oled_view_put_u32(p, (info->stack_free[i] > 999U) ? 999U : info->stack_free[i], 3U);
        if (i < 2U) { p = put_text(p, "/"); }
    }
    (void)ssd1306_draw_text(0, 56, buf, 1U);

    return ssd1306_flush(timeout_ms);
}
