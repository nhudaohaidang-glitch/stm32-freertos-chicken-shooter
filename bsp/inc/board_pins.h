#ifndef BOARD_PINS_H
#define BOARD_PINS_H
#include <stdint.h>
typedef struct { char port; uint8_t pin; } board_pin_t;
#define BOARD_TFT_SCK   ((board_pin_t){'A', 5})
#define BOARD_TFT_MOSI  ((board_pin_t){'A', 7})
#define BOARD_TFT_CS    ((board_pin_t){'A', 4})
#define BOARD_TFT_DC    ((board_pin_t){'A', 3})
#define BOARD_TFT_RESET ((board_pin_t){'B', 1})
#define BOARD_OLED_SCL  ((board_pin_t){'B', 6})
#define BOARD_OLED_SDA  ((board_pin_t){'B', 7})
#define BOARD_JOY_X    ((board_pin_t){'A', 0})
#define BOARD_JOY_Y    ((board_pin_t){'A', 1})
#define BOARD_FIRE     ((board_pin_t){'B', 12})
#define BOARD_PAUSE    ((board_pin_t){'B', 13})
/* PA13/PA14 reserved for SWD. Logic level: 3.3 V, common GND.
 * Ba ngoại vi chính: SPI1 (TFT), I2C1 (OLED), ADC1 (joystick); GPIO cho nút và chân TFT. */
#endif
