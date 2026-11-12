#include "ssd1306.h"
#include "hal_i2c.h"
#include "font5x7.h"
#include <string.h>

#define CTRL_CMD  0x00U
#define CTRL_DATA 0x40U

/* Mỗi page: [0x40][128 byte] để gửi thẳng qua I2C, không cần copy.
 * Tổng 8 x 129 = 1032 byte (ngân sách OLED 1024 + 8 byte control). */
static uint8_t s_buf[SSD1306_PAGES][1U + SSD1306_WIDTH];
static uint8_t s_dirty;  /* bit p = page p cần gửi */
static uint8_t s_addr;
static bool s_ready;

static hal_status_t cmd(const uint8_t *c, size_t n, uint32_t timeout_ms)
{
    uint8_t tx[8];
    if (n + 1U > sizeof tx) { return HAL_INVALID_ARG; }
    tx[0] = CTRL_CMD;
    memcpy(&tx[1], c, n);
    return hal_i2c_write(s_addr, tx, n + 1U, timeout_ms);
}

hal_status_t ssd1306_init(uint8_t addr7, uint32_t timeout_ms)
{
    /* Chuỗi khởi tạo 128x64, charge pump nội, horizontal addressing. */
    static const uint8_t seq[][3] = {
        { 1, 0xAE },        /* display off */
        { 2, 0xD5, 0x80 },  /* clock divide */
        { 2, 0xA8, 0x3F },  /* multiplex 64 */
        { 2, 0xD3, 0x00 },  /* display offset */
        { 1, 0x40 },        /* start line 0 */
        { 2, 0x8D, 0x14 },  /* charge pump on */
        { 2, 0x20, 0x00 },  /* horizontal addressing */
        { 1, 0xA1 },        /* segment remap: cột 127 -> SEG0 */
        { 1, 0xC8 },        /* COM scan ngược: gốc tọa độ góc trên trái */
        { 2, 0xDA, 0x12 },  /* COM pins alternative */
        { 2, 0x81, 0xCF },  /* contrast */
        { 2, 0xD9, 0xF1 },  /* pre-charge */
        { 2, 0xDB, 0x40 },  /* VCOMH */
        { 1, 0xA4 },        /* hiển thị theo RAM */
        { 1, 0xA6 },        /* không đảo màu */
        { 1, 0x2E },        /* tắt scroll */
    };
    if (addr7 > 0x7FU) { return HAL_INVALID_ARG; }
    s_addr = addr7;
    s_ready = false;
    for (size_t i = 0; i < sizeof seq / sizeof seq[0]; i++) {
        hal_status_t st = cmd(&seq[i][1], seq[i][0], timeout_ms);
        if (st != HAL_OK) { return st; }
    }
    for (unsigned p = 0; p < SSD1306_PAGES; p++) { s_buf[p][0] = CTRL_DATA; }
    s_ready = true;
    ssd1306_clear();
    hal_status_t st = ssd1306_flush(timeout_ms);  /* xóa RAM rác trước khi bật */
    if (st != HAL_OK) { return st; }
    return ssd1306_display_on(true, timeout_ms);
}

hal_status_t ssd1306_display_on(bool on, uint32_t timeout_ms)
{
    const uint8_t c = on ? 0xAFU : 0xAEU;
    return s_ready ? cmd(&c, 1U, timeout_ms) : HAL_IO_ERROR;
}

hal_status_t ssd1306_set_contrast(uint8_t level, uint32_t timeout_ms)
{
    const uint8_t c[2] = { 0x81U, level };
    return s_ready ? cmd(c, 2U, timeout_ms) : HAL_IO_ERROR;
}

void ssd1306_clear(void)
{
    for (unsigned p = 0; p < SSD1306_PAGES; p++) {
        memset(&s_buf[p][1], 0, SSD1306_WIDTH);
    }
    s_dirty = 0xFFU;
}

void ssd1306_invalidate(void)
{
    s_dirty = 0xFFU;
}

void ssd1306_draw_pixel(int16_t x, int16_t y, bool on)
{
    if (x < 0 || y < 0 || x >= (int16_t)SSD1306_WIDTH || y >= (int16_t)SSD1306_HEIGHT) { return; }
    const unsigned p = (unsigned)y >> 3;
    const uint8_t bit = (uint8_t)(1U << ((unsigned)y & 7U));
    uint8_t *b = &s_buf[p][1U + (unsigned)x];
    const uint8_t old = *b;
    *b = on ? (uint8_t)(old | bit) : (uint8_t)(old & (uint8_t)~bit);
    if (*b != old) { s_dirty |= (uint8_t)(1U << p); }
}

void ssd1306_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, bool on)
{
    for (int16_t j = 0; j < h; j++) {
        for (int16_t i = 0; i < w; i++) {
            ssd1306_draw_pixel((int16_t)(x + i), (int16_t)(y + j), on);
        }
    }
}

int16_t ssd1306_draw_text(int16_t x, int16_t y, const char *s, uint8_t scale)
{
    if (s == NULL) { return x; }
    if (scale == 0U) { scale = 1U; }
    for (; *s != '\0'; s++) {
        const uint8_t *g = font5x7_glyph(*s);
        /* Vẽ cả cột trống thứ 6 và hàng thứ 8 để ghi đè chữ cũ khi cập nhật số. */
        for (unsigned cx = 0; cx < FONT5X7_WIDTH + 1U; cx++) {
            const uint8_t col = (cx < FONT5X7_WIDTH) ? g[cx] : 0U;
            for (unsigned cy = 0; cy < 8U; cy++) {
                ssd1306_fill_rect((int16_t)(x + (int16_t)(cx * scale)),
                                  (int16_t)(y + (int16_t)(cy * scale)),
                                  (int16_t)scale, (int16_t)scale,
                                  ((col >> cy) & 1U) != 0U);
            }
        }
        x = (int16_t)(x + (int16_t)((FONT5X7_WIDTH + 1U) * scale));
        if (x >= (int16_t)SSD1306_WIDTH) { break; }
    }
    return x;
}

hal_status_t ssd1306_flush(uint32_t timeout_ms)
{
    if (!s_ready) { return HAL_IO_ERROR; }
    for (unsigned p = 0; p < SSD1306_PAGES; p++) {
        if ((s_dirty & (1U << p)) == 0U) { continue; }
        const uint8_t win[6] = { 0x21U, 0x00U, (uint8_t)(SSD1306_WIDTH - 1U),  /* cột 0..127 */
                                 0x22U, (uint8_t)p, (uint8_t)p };               /* page p */
        hal_status_t st = cmd(win, sizeof win, timeout_ms);
        if (st == HAL_OK) {
            st = hal_i2c_write(s_addr, s_buf[p], sizeof s_buf[p], timeout_ms);
        }
        if (st != HAL_OK) { return st; }  /* page vẫn bẩn, lần sau gửi lại */
        s_dirty &= (uint8_t)~(1U << p);
    }
    return HAL_OK;
}
