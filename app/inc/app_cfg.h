#ifndef APP_CFG_H
#define APP_CFG_H
/* Bảng tài nguyên RTOS thống nhất (Giai đoạn 2 - Bước 1 quy trình).
 * Stack tính bằng WORD 32 bit, không phải byte. Giá trị ước lượng từ -fstack-usage;
 * PHẢI xác nhận bằng uxTaskGetStackHighWaterMark() trên kit (dòng STK trên OLED). */
#define APP_HMI_PRIO 3U
#define APP_GAME_PRIO 3U
#define APP_RENDER_PRIO 2U
#define APP_HMI_STACK_WORDS 192U
#define APP_GAME_STACK_WORDS 256U
#define APP_RENDER_STACK_WORDS 384U
#define APP_HMI_PERIOD_MS 10U
#define APP_GAME_PERIOD_MS 20U
#define APP_INPUT_QUEUE_LENGTH 16U

/* ---- TFT: một bộ đệm dải nhỏ, không framebuffer ----
 * Đề cương dự trù 2 bộ đệm (2.560 B) cho DMA; dự án truyền bằng polling nên chỉ
 * cần 1 bộ đệm 1.280 B (ghép xong một dải rồi gửi). */
#define APP_TFT_WIDTH 240U
#define APP_TFT_HEIGHT 320U
#define APP_TFT_MAX_LINE_PIXELS 320U
#define APP_TFT_LINES_PER_BUFFER 2U
#define APP_TFT_BUFFER_COUNT 1U
#define APP_TFT_BUFFER_PIXELS (APP_TFT_MAX_LINE_PIXELS * APP_TFT_LINES_PER_BUFFER)
#define APP_TFT_BUFFER_BYTES (APP_TFT_BUFFER_COUNT * APP_TFT_BUFFER_PIXELS * 2U)
#define APP_OLED_BUFFER_BYTES 1024U
_Static_assert(APP_TFT_BUFFER_BYTES <= 2560U, "within proposal buffer budget");

/* ---- HMI ---- */
#define APP_ADC_TIMEOUT_MS 2U
#define APP_JOY_DEADZONE 350U        /* mã ADC quanh tâm được coi là đứng yên */
#define APP_JOY_INVERT_X 0           /* 1 nếu gạt trái lại đi phải (tùy cách lắp) */
#define APP_BTN_DEBOUNCE_SAMPLES 3U  /* số mẫu liên tiếp giống nhau (>= 5 ms/mẫu) */
#define APP_BTN_MIN_SAMPLE_MS 5U
#define APP_FIRE_REPEAT_MS 200U      /* giữ Fire: bắn tự động */
#define APP_MOVE_REFRESH_MS 250U     /* gửi lại tốc độ hiện tại, chống mất gói */

/* ---- RENDER ---- */
#define APP_TFT_TIMEOUT_MS 200U
#define APP_OLED_ADDR 0x3CU          /* đo bằng harness I2C; 0x3D nếu jumper khác */
#define APP_OLED_TIMEOUT_MS 20U
#define APP_OLED_PERIOD_MS 100U
#define APP_STATS_PERIOD_MS 1000U      /* cập nhật số đo FPS/heap/stack trên OLED */
#endif
