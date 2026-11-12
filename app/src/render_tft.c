#include "render_tft.h"
#include "ili9341.h"
#include "font5x7.h"
#include "app_cfg.h"
#include <stdbool.h>
#include <string.h>

#define MAX_RECTS 64U
#define MAX_ITEMS (GAME_CHICKEN_COUNT + GAME_MAX_BOOMS + GAME_MAX_EGGS + GAME_MAX_BULLETS + 1U)
#define MAX_TEXTS 4U

/* ---------------- Sprite (vẽ bằng ký tự, '.' = trong suốt) ---------------- */
static const char *const k_chicken_a[GAME_CHICKEN_H] = {
    ".......RR.......",
    "......RRRR......",
    ".....WWWWWW.....",
    "....WWKWWKWW....",
    "....WWWYYWWW....",
    "....WWWOOWWW....",
    "W..WWWWRRWWWW..W",
    "WW.WWWWWWWWWW.WW",
    "WWWWWWWWWWWWWWWW",
    ".WWWWWWWWWWWWWW.",
    "..wWWWWWWWWWWw..",
    "....WWWWWWWW....",
    ".....Y....Y.....",
    "....YY....YY....",
};
static const char *const k_chicken_b[GAME_CHICKEN_H] = {
    ".......RR.......",
    "......RRRR......",
    ".....WWWWWW.....",
    "....WWKWWKWW....",
    "....WWWYYWWW....",
    "....WWWOOWWW....",
    "...WWWWRRWWWW...",
    "..WWWWWWWWWWWW..",
    ".WWWWWWWWWWWWWW.",
    "WWWWWWWWWWWWWWWW",
    "WW.wWWWWWWWWw.WW",
    "W...WWWWWWWW...W",
    ".....Y....Y.....",
    "....YY....YY....",
};
static const char *const k_player[GAME_PLAYER_H] = {
    ".......CC.......",
    ".......CC.......",
    "......CWWC......",
    "......BWWB......",
    ".....BBCCBB.....",
    ".....BBCCBB.....",
    "....BBBBBBBB....",
    "R...BBWBBWBB...R",
    "R..BBBBBBBBBB..R",
    "B.BBBBBBBBBBBB.B",
    "BBBBBBBBBBBBBBBB",
    "BBBbBBBBBBBBbBBB",
    "bB..bBBBBBBb..Bb",
    "....bbOOOObb....",
    ".....OYYYYO.....",
    "......O..O......",
};
static const char *const k_bullet[GAME_BULLET_H] = { "WW", "YY", "YY", "YY", "OO", "OO" };
static const char *const k_egg[GAME_EGG_H] = { ".ww.", "wWWw", "WWWW", "WWWW", "wWWw", ".ww." };
static const char *const k_boom[GAME_BOOM_H] = {
    ".......Y........",
    "...Y...O...Y....",
    "....O..R..O.....",
    ".....ORYRO...Y..",
    "..Y..RYWYR..O...",
    "...OORYWWYROO...",
    ".YRRYWWWWWYRRY..",
    "...OORYWWYROO...",
    "..Y..RYWYR..O...",
    ".....ORYRO......",
    "....O..R..O.....",
    "...Y...O...Y....",
    ".......Y........",
    "................",
};

#define C_BG       ILI9341_RGB(4, 6, 28)
#define C_STAR1    ILI9341_RGB(150, 150, 170)
#define C_STAR2    ILI9341_RGB(70, 70, 110)
#define C_TEXT     ILI9341_RGB(255, 220, 40)
#define C_TEXT2    ILI9341_RGB(255, 255, 255)
#define C_BOX      ILI9341_RGB(30, 30, 80)
#define TRANSPARENT 0x10000UL

static uint32_t pal(char c)
{
    switch (c) {
    case 'W': return ILI9341_RGB(255, 255, 255);
    case 'w': return ILI9341_RGB(170, 170, 170);
    case 'R': return ILI9341_RGB(230, 30, 30);
    case 'Y': return ILI9341_RGB(255, 215, 0);
    case 'O': return ILI9341_RGB(255, 120, 0);
    case 'K': return ILI9341_RGB(0, 0, 0);
    case 'B': return ILI9341_RGB(40, 90, 230);
    case 'b': return ILI9341_RGB(20, 40, 120);
    case 'C': return ILI9341_RGB(80, 230, 255);
    default:  return TRANSPARENT;
    }
}

/* ---------------- Danh sách đối tượng cần vẽ ---------------- */
typedef enum { SPR_CHICKEN_A, SPR_CHICKEN_B, SPR_PLAYER, SPR_BULLET, SPR_EGG, SPR_BOOM } sprite_id_t;
typedef struct { const char *const *rows; uint8_t w; uint8_t h; } sprite_t;
static const sprite_t k_sprites[] = {
    [SPR_CHICKEN_A] = { k_chicken_a, GAME_CHICKEN_W, GAME_CHICKEN_H },
    [SPR_CHICKEN_B] = { k_chicken_b, GAME_CHICKEN_W, GAME_CHICKEN_H },
    [SPR_PLAYER]    = { k_player, GAME_PLAYER_W, GAME_PLAYER_H },
    [SPR_BULLET]    = { k_bullet, GAME_BULLET_W, GAME_BULLET_H },
    [SPR_EGG]       = { k_egg, GAME_EGG_W, GAME_EGG_H },
    [SPR_BOOM]      = { k_boom, GAME_BOOM_W, GAME_BOOM_H },
};

typedef struct { int16_t x; int16_t y; uint8_t spr; } item_t;
typedef struct { int16_t x; int16_t y; uint8_t scale; uint16_t color; const char *s; } text_t;
typedef struct { int16_t x0; int16_t y0; int16_t x1; int16_t y1; } rect_t; /* [x0,x1) x [y0,y1) */

static item_t s_items[MAX_ITEMS];
static unsigned s_item_count;
static text_t s_texts[MAX_TEXTS];
static unsigned s_text_count;
static rect_t s_box;               /* hộp nền cho chữ (rỗng nếu x0 >= x1) */
static rect_t s_rects[MAX_RECTS];
static unsigned s_rect_count;
static char s_score_text[16];
static render_tft_stats_t s_stats;

/* Một bộ đệm dải 320 px x 2 dòng x 2 B = 1.280 B (trong ngân sách 2.560 B của đề cương). */
static uint16_t s_buf[APP_TFT_BUFFER_PIXELS];

static void add_item(int16_t x, int16_t y, sprite_id_t spr)
{
    if (s_item_count < MAX_ITEMS) {
        s_items[s_item_count].x = x;
        s_items[s_item_count].y = y;
        s_items[s_item_count].spr = (uint8_t)spr;
        s_item_count++;
    }
}

static void add_text_centered(int16_t y, uint8_t scale, uint16_t color, const char *s)
{
    if (s_text_count >= MAX_TEXTS) { return; }
    const int16_t w = (int16_t)(strlen(s) * (FONT5X7_WIDTH + 1U) * scale);
    text_t *t = &s_texts[s_text_count++];
    t->x = (int16_t)((GAME_SCREEN_W - w) / 2);
    t->y = y;
    t->scale = scale;
    t->color = color;
    t->s = s;
    /* Hộp nền bao mọi dòng chữ, lề 6 px. */
    const rect_t r = { (int16_t)(t->x - 6), (int16_t)(y - 6), (int16_t)(t->x + w + 6),
                       (int16_t)(y + (int16_t)(8U * scale) + 4) };
    if (s_box.x0 >= s_box.x1) {
        s_box = r;
    } else {
        if (r.x0 < s_box.x0) { s_box.x0 = r.x0; }
        if (r.y0 < s_box.y0) { s_box.y0 = r.y0; }
        if (r.x1 > s_box.x1) { s_box.x1 = r.x1; }
        if (r.y1 > s_box.y1) { s_box.y1 = r.y1; }
    }
}

static void build_scene(const game_snapshot_t *s)
{
    s_item_count = 0U;
    s_text_count = 0U;
    s_box.x0 = s_box.x1 = 0;
    const sprite_id_t chick = ((s->flags & GAME_FLAG_CHICKEN_FRAME) != 0U) ? SPR_CHICKEN_B : SPR_CHICKEN_A;

    /* Thứ tự vẽ: gà -> nổ -> trứng -> đạn -> tàu (sau đè trước). */
    for (unsigned i = 0; i < GAME_CHICKEN_COUNT; i++) {
        if ((s->chicken_alive & (1UL << i)) != 0U) {
            add_item((int16_t)(s->formation_x + (int16_t)((i % GAME_CHICKEN_COLS) * GAME_CHICKEN_DX)),
                     (int16_t)(s->formation_y + (int16_t)((i / GAME_CHICKEN_COLS) * GAME_CHICKEN_DY)), chick);
        }
    }
    for (unsigned i = 0; i < GAME_MAX_BOOMS; i++) {
        if (s->booms[i].x != GAME_POS_NONE) { add_item(s->booms[i].x, s->booms[i].y, SPR_BOOM); }
    }
    for (unsigned i = 0; i < GAME_MAX_EGGS; i++) {
        if (s->eggs[i].x != GAME_POS_NONE) { add_item(s->eggs[i].x, s->eggs[i].y, SPR_EGG); }
    }
    for (unsigned i = 0; i < GAME_MAX_BULLETS; i++) {
        if (s->bullets[i].x != GAME_POS_NONE) { add_item(s->bullets[i].x, s->bullets[i].y, SPR_BULLET); }
    }
    if ((s->flags & GAME_FLAG_PLAYER_VISIBLE) != 0U) {
        add_item((int16_t)s->player_x, GAME_PLAYER_Y, SPR_PLAYER);
    }

    switch (s->state) {
    case GAME_MENU:
        add_text_centered(110, 4, C_TEXT, "CHICKEN");
        add_text_centered(150, 4, C_TEXT, "SHOOTER");
        add_text_centered(200, 2, C_TEXT2, "PRESS START");
        break;
    case GAME_PAUSED:
        add_text_centered(140, 3, C_TEXT, "PAUSED");
        break;
    case GAME_OVER: {
        char *p = s_score_text;
        memcpy(p, "SCORE ", 6U);
        p += 6;
        uint32_t v = s->score % 1000000UL;
        for (int d = 5; d >= 0; d--) { p[d] = (char)('0' + v % 10U); v /= 10U; }
        p[6] = '\0';
        add_text_centered(120, 3, C_TEXT, "GAME OVER");
        add_text_centered(160, 2, C_TEXT2, s_score_text);
        add_text_centered(190, 2, C_TEXT2, "PRESS START");
        break;
    }
    default:
        break;
    }
}

/* Nền: xanh đêm + sao thưa cố định (băm tọa độ, không tốn RAM). */
static inline uint16_t background(int x, int y)
{
    uint32_t h = (uint32_t)x * 73856093UL ^ (uint32_t)y * 19349663UL;
    h ^= h >> 13;
    h *= 0x5bd1e995UL;
    h ^= h >> 15;
    if ((h & 0x3FFU) == 0U) { return C_STAR1; }
    if ((h & 0x3FFU) == 1U) { return C_STAR2; }
    return C_BG;
}

/* Ghép một dải [x0, x0+w) x [y0, y0+lines) vào buf (pixel big-endian). */
static void compose(uint16_t *buf, int16_t x0, int16_t y0, int16_t w, int16_t lines)
{
    const int16_t x1 = (int16_t)(x0 + w);
    const int16_t y1 = (int16_t)(y0 + lines);

    for (int16_t y = y0; y < y1; y++) {
        uint16_t *row = &buf[(y - y0) * w];
        for (int16_t x = x0; x < x1; x++) {
            row[x - x0] = ILI9341_BE(background(x, y));
        }
    }

    for (unsigned k = 0; k < s_item_count; k++) {
        const item_t *it = &s_items[k];
        const sprite_t *sp = &k_sprites[it->spr];
        const int16_t ix0 = (it->x > x0) ? it->x : x0;
        const int16_t iy0 = (it->y > y0) ? it->y : y0;
        const int16_t ix1 = ((int16_t)(it->x + sp->w) < x1) ? (int16_t)(it->x + sp->w) : x1;
        const int16_t iy1 = ((int16_t)(it->y + sp->h) < y1) ? (int16_t)(it->y + sp->h) : y1;
        for (int16_t y = iy0; y < iy1; y++) {
            const char *src = sp->rows[y - it->y];
            uint16_t *row = &buf[(y - y0) * w];
            for (int16_t x = ix0; x < ix1; x++) {
                const uint32_t c = pal(src[x - it->x]);
                if (c != TRANSPARENT) { row[x - x0] = ILI9341_BE((uint16_t)c); }
            }
        }
    }

    /* Hộp chữ đè lên sprite để chữ luôn đọc được. */
    if (s_box.x0 < s_box.x1) {
        const int16_t bx0 = (s_box.x0 > x0) ? s_box.x0 : x0;
        const int16_t bx1 = (s_box.x1 < x1) ? s_box.x1 : x1;
        const int16_t by0 = (s_box.y0 > y0) ? s_box.y0 : y0;
        const int16_t by1 = (s_box.y1 < y1) ? s_box.y1 : y1;
        for (int16_t y = by0; y < by1; y++) {
            for (int16_t x = bx0; x < bx1; x++) { buf[(y - y0) * w + (x - x0)] = ILI9341_BE(C_BOX); }
        }
    }

    for (unsigned k = 0; k < s_text_count; k++) {
        const text_t *t = &s_texts[k];
        const int16_t cw = (int16_t)((FONT5X7_WIDTH + 1U) * t->scale);
        const int16_t ch = (int16_t)(8U * t->scale);
        if (t->y >= y1 || (int16_t)(t->y + ch) <= y0) { continue; }
        int16_t cx = t->x;
        for (const char *s = t->s; *s != '\0'; s++, cx = (int16_t)(cx + cw)) {
            if (cx >= x1 || (int16_t)(cx + cw) <= x0) { continue; }
            const uint8_t *g = font5x7_glyph(*s);
            const int16_t gx0 = (cx > x0) ? cx : x0;
            const int16_t gx1 = ((int16_t)(cx + cw) < x1) ? (int16_t)(cx + cw) : x1;
            const int16_t gy0 = (t->y > y0) ? t->y : y0;
            const int16_t gy1 = ((int16_t)(t->y + ch) < y1) ? (int16_t)(t->y + ch) : y1;
            for (int16_t y = gy0; y < gy1; y++) {
                const unsigned gy = (unsigned)(y - t->y) / t->scale;
                uint16_t *row = &buf[(y - y0) * w];
                for (int16_t x = gx0; x < gx1; x++) {
                    const unsigned gxi = (unsigned)(x - cx) / t->scale;
                    if (gxi < FONT5X7_WIDTH && ((g[gxi] >> gy) & 1U) != 0U) {
                        row[x - x0] = ILI9341_BE(t->color);
                    }
                }
            }
        }
    }
}

/* ---------------- Vùng bẩn ---------------- */
static bool clip(rect_t *r)
{
    if (r->x0 < 0) { r->x0 = 0; }
    if (r->y0 < 0) { r->y0 = 0; }
    if (r->x1 > GAME_SCREEN_W) { r->x1 = GAME_SCREEN_W; }
    if (r->y1 > GAME_SCREEN_H) { r->y1 = GAME_SCREEN_H; }
    return r->x0 < r->x1 && r->y0 < r->y1;
}

static bool add_rect(rect_t r)
{
    if (!clip(&r)) { return true; }
    if (s_rect_count >= MAX_RECTS) { return false; }
    s_rects[s_rect_count++] = r;
    return true;
}

static rect_t rect_of(int16_t x, int16_t y, int16_t w, int16_t h)
{
    const rect_t r = { x, y, (int16_t)(x + w), (int16_t)(y + h) };
    return r;
}

/* Vị trí cũ/mới của cùng một đối tượng: gần nhau -> một vùng hợp, xa -> hai vùng. */
static bool add_move(bool had, int16_t ox, int16_t oy, bool has, int16_t nx, int16_t ny,
                     int16_t w, int16_t h)
{
    if (had && has && ox == nx && oy == ny) { return true; }
    if (had && has) {
        const int16_t dx = (int16_t)((ox > nx) ? ox - nx : nx - ox);
        const int16_t dy = (int16_t)((oy > ny) ? oy - ny : ny - oy);
        if (dx < w && dy < h) {
            const rect_t u = { (ox < nx) ? ox : nx, (oy < ny) ? oy : ny,
                               (int16_t)(((ox > nx) ? ox : nx) + w), (int16_t)(((oy > ny) ? oy : ny) + h) };
            return add_rect(u);
        }
    }
    bool ok = true;
    if (had) { ok = add_rect(rect_of(ox, oy, w, h)); }
    if (has && ok) { ok = add_rect(rect_of(nx, ny, w, h)); }
    return ok;
}

static bool add_slots(const game_pos_t *a, const game_pos_t *b, unsigned n, int16_t w, int16_t h)
{
    for (unsigned i = 0; i < n; i++) {
        if (!add_move(a[i].x != GAME_POS_NONE, a[i].x, a[i].y,
                      b[i].x != GAME_POS_NONE, b[i].x, b[i].y, w, h)) { return false; }
    }
    return true;
}

static bool build_dirty(const game_snapshot_t *p, const game_snapshot_t *c)
{
    s_rect_count = 0U;
    const bool same_formation = p->formation_x == c->formation_x && p->formation_y == c->formation_y &&
        ((p->flags ^ c->flags) & GAME_FLAG_CHICKEN_FRAME) == 0U;
    for (unsigned i = 0; i < GAME_CHICKEN_COUNT; i++) {
        const bool had = (p->chicken_alive & (1UL << i)) != 0U;
        const bool has = (c->chicken_alive & (1UL << i)) != 0U;
        if (same_formation && had == has) { continue; }
        const int16_t ox = (int16_t)(p->formation_x + (int16_t)((i % GAME_CHICKEN_COLS) * GAME_CHICKEN_DX));
        const int16_t oy = (int16_t)(p->formation_y + (int16_t)((i / GAME_CHICKEN_COLS) * GAME_CHICKEN_DY));
        const int16_t nx = (int16_t)(c->formation_x + (int16_t)((i % GAME_CHICKEN_COLS) * GAME_CHICKEN_DX));
        const int16_t ny = (int16_t)(c->formation_y + (int16_t)((i / GAME_CHICKEN_COLS) * GAME_CHICKEN_DY));
        if (had && has && ox == nx && oy == ny) {
            if (!add_rect(rect_of(nx, ny, GAME_CHICKEN_W, GAME_CHICKEN_H))) { return false; } /* đổi khung cánh */
        } else if (!add_move(had, ox, oy, has, nx, ny, GAME_CHICKEN_W, GAME_CHICKEN_H)) {
            return false;
        }
    }
    if (!add_slots(p->booms, c->booms, GAME_MAX_BOOMS, GAME_BOOM_W, GAME_BOOM_H)) { return false; }
    if (!add_slots(p->eggs, c->eggs, GAME_MAX_EGGS, GAME_EGG_W, GAME_EGG_H)) { return false; }
    if (!add_slots(p->bullets, c->bullets, GAME_MAX_BULLETS, GAME_BULLET_W, GAME_BULLET_H)) { return false; }
    const bool pv = (p->flags & GAME_FLAG_PLAYER_VISIBLE) != 0U;
    const bool cv = (c->flags & GAME_FLAG_PLAYER_VISIBLE) != 0U;
    if (pv != cv || p->player_x != c->player_x) {
        return add_move(pv, (int16_t)p->player_x, GAME_PLAYER_Y, cv, (int16_t)c->player_x, GAME_PLAYER_Y,
                        GAME_PLAYER_W, GAME_PLAYER_H);
    }
    return true;
}

/* ---------------- Đẩy một vùng ra màn hình ---------------- */
static hal_status_t draw_rect(const rect_t *r, uint32_t timeout_ms)
{
    const int16_t w = (int16_t)(r->x1 - r->x0);
    const int16_t h = (int16_t)(r->y1 - r->y0);
    int16_t lines_per = (int16_t)(APP_TFT_BUFFER_PIXELS / (unsigned)w);
    if (lines_per < 1) { lines_per = 1; }

    hal_status_t st = ili9341_begin((uint16_t)r->x0, (uint16_t)r->y0, (uint16_t)w, (uint16_t)h, timeout_ms);
    if (st != HAL_OK) { return st; }
    for (int16_t y = r->y0; y < r->y1 && st == HAL_OK; y = (int16_t)(y + lines_per)) {
        const int16_t lines = ((int16_t)(r->y1 - y) < lines_per) ? (int16_t)(r->y1 - y) : lines_per;
        compose(s_buf, r->x0, y, w, lines);             /* ghép dải trong RAM */
        st = ili9341_push(s_buf, (size_t)w * (size_t)lines * 2U, timeout_ms);  /* gửi polling */
    }
    ili9341_end();
    s_stats.pixels += (uint32_t)w * (uint32_t)h;
    s_stats.rects++;
    return st;
}

hal_status_t render_tft_draw(const game_snapshot_t *prev, const game_snapshot_t *cur, uint32_t timeout_ms)
{
    if (cur == NULL) { return HAL_INVALID_ARG; }
    s_stats.rects = 0U;
    s_stats.pixels = 0U;
    s_stats.full_redraw = 0U;
    build_scene(cur);

    bool full = (prev == NULL) || prev->state != cur->state || prev->wave != cur->wave ||
                cur->state != GAME_PLAYING;
    if (!full && !build_dirty(prev, cur)) { full = true; }   /* quá nhiều vùng */
    if (full) {
        if (prev != NULL && cur->state != GAME_PLAYING && prev->state == cur->state &&
            prev->wave == cur->wave && prev->score == cur->score) {
            return HAL_OK;                                    /* màn tĩnh không đổi: bỏ qua */
        }
        s_stats.full_redraw = 1U;
        s_rect_count = 1U;
        s_rects[0] = rect_of(0, 0, GAME_SCREEN_W, GAME_SCREEN_H);
    }
    for (unsigned i = 0; i < s_rect_count; i++) {
        const hal_status_t st = draw_rect(&s_rects[i], timeout_ms);
        if (st != HAL_OK) { return st; }
    }
    return HAL_OK;
}

const render_tft_stats_t *render_tft_last_stats(void)
{
    return &s_stats;
}
