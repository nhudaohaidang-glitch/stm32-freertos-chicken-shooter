#ifndef APP_CFG_H
#define APP_CFG_H
/* Initial values, NOT measured. Stack depths are 32-bit words, not bytes. */
#define APP_HMI_PRIO 3U
#define APP_GAME_PRIO 3U
#define APP_RENDER_PRIO 2U
#define APP_HMI_STACK_WORDS 192U
#define APP_GAME_STACK_WORDS 256U
#define APP_RENDER_STACK_WORDS 384U
#define APP_HMI_PERIOD_MS 10U
#define APP_GAME_PERIOD_MS 20U
#define APP_INPUT_QUEUE_LENGTH 16U
#define APP_SOUND_QUEUE_LENGTH 8U
#define APP_TFT_WIDTH 240U
#define APP_TFT_HEIGHT 320U
#define APP_TFT_MAX_LINE_PIXELS 320U
#define APP_TFT_LINES_PER_BUFFER 2U
#define APP_TFT_BUFFER_COUNT 2U
#define APP_TFT_BUFFER_BYTES (APP_TFT_BUFFER_COUNT * APP_TFT_MAX_LINE_PIXELS * APP_TFT_LINES_PER_BUFFER * 2U)
#define APP_OLED_BUFFER_BYTES 1024U
/* Allocate these buffers only in the future render/driver implementation. */
_Static_assert(APP_TFT_BUFFER_BYTES == 2560U, "proposal buffer budget");
#endif
