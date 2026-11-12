#ifndef FONT5X7_H
#define FONT5X7_H
#include <stdint.h>
/* Font 5x7 ASCII 32..90 (space..Z); chữ thường được in hoa.
 * Mỗi glyph 5 cột, bit0 = hàng trên cùng. Ký tự ngoài dải hiển thị '?'. */
#define FONT5X7_FIRST 32U
#define FONT5X7_LAST  90U
#define FONT5X7_WIDTH 5U
#define FONT5X7_HEIGHT 7U
const uint8_t *font5x7_glyph(char c);
#endif
