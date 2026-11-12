#include "ili9341.h"
#include "hal_spi.h"
#include "hal_sys.h"
#include "FreeRTOS.h"
#include "task.h"

#define REG(a) (*(volatile uint32_t *)(a))
#define RCC_APB2ENR  REG(0x40021018UL)
#define GPIOA_CRL    REG(0x40010800UL)
#define GPIOA_BSRR   REG(0x40010810UL)
#define GPIOB_CRL    REG(0x40010C00UL)
#define GPIOB_BSRR   REG(0x40010C10UL)
#define RCC_APB2_IOPA (1UL << 2)
#define RCC_APB2_IOPB (1UL << 3)

#define PIN_DC  3U   /* PA3 */
#define PIN_CS  4U   /* PA4 */
#define PIN_RST 1U   /* PB1 */
#define OUT_PP_50MHZ 0x3UL

#define CMD_SWRESET 0x01U
#define CMD_SLPOUT  0x11U
#define CMD_DISPON  0x29U
#define CMD_CASET   0x2AU
#define CMD_PASET   0x2BU
#define CMD_RAMWR   0x2CU
#define CMD_MADCTL  0x36U
#define CMD_PIXFMT  0x3AU

static bool s_ready;
static bool s_open;

static inline void cs(bool high) { GPIOA_BSRR = high ? (1UL << PIN_CS) : (1UL << (PIN_CS + 16U)); }
static inline void dc(bool data) { GPIOA_BSRR = data ? (1UL << PIN_DC) : (1UL << (PIN_DC + 16U)); }
static inline void rst(bool high) { GPIOB_BSRR = high ? (1UL << PIN_RST) : (1UL << (PIN_RST + 16U)); }

/* Trong task luôn dùng FreeRTOS delay; chỉ chờ bận khi chưa có scheduler. */
static void wait_panel_ms(uint32_t ms)
{
    if (hal_sys_rtos_running()) {
        vTaskDelay(pdMS_TO_TICKS(ms) + 1U);
    } else {
        while (ms-- > 0U) { hal_sys_delay_us(1000U); }
    }
}

static void pins_init(void)
{
    RCC_APB2ENR |= RCC_APB2_IOPA | RCC_APB2_IOPB;
    (void)RCC_APB2ENR;
    cs(true);
    dc(true);
    rst(true);
    const uint32_t st = hal_sys_enter_critical();      /* CRL dùng chung với ADC/SPI/I2C */
    GPIOA_CRL = (GPIOA_CRL & ~((0xFUL << (PIN_DC * 4U)) | (0xFUL << (PIN_CS * 4U))))
              | (OUT_PP_50MHZ << (PIN_DC * 4U)) | (OUT_PP_50MHZ << (PIN_CS * 4U));
    GPIOB_CRL = (GPIOB_CRL & ~(0xFUL << (PIN_RST * 4U))) | (OUT_PP_50MHZ << (PIN_RST * 4U));
    hal_sys_exit_critical(st);
}

/* Gửi một lệnh + tham số trong khi đã giữ khóa và CS thấp. */
static hal_status_t write_cmd(uint8_t cmd, const uint8_t *args, size_t n, uint32_t timeout_ms)
{
    dc(false);
    hal_status_t st = hal_spi_write(&cmd, 1U, timeout_ms);
    dc(true);
    if (st == HAL_OK && n > 0U) { st = hal_spi_write(args, n, timeout_ms); }
    return st;
}

/* Một lệnh độc lập (khóa + CS quanh lệnh). */
static hal_status_t command(uint8_t cmd, const uint8_t *args, size_t n, uint32_t timeout_ms)
{
    hal_status_t st = hal_spi_lock(timeout_ms);
    if (st != HAL_OK) { return st; }
    cs(false);
    st = write_cmd(cmd, args, n, timeout_ms);
    cs(true);
    hal_spi_unlock();
    return st;
}

hal_status_t ili9341_init(uint32_t timeout_ms)
{
    /* Bảng khởi tạo theo datasheet ILI9341 (power, VCOM, gamma). Byte đầu = số tham số. */
    static const uint8_t seq[] = {
        0xCFU, 3, 0x00, 0xC1, 0x30,               /* Power control B */
        0xEDU, 4, 0x64, 0x03, 0x12, 0x81,         /* Power on sequence */
        0xE8U, 3, 0x85, 0x00, 0x78,               /* Driver timing A */
        0xCBU, 5, 0x39, 0x2C, 0x00, 0x34, 0x02,   /* Power control A */
        0xF7U, 1, 0x20,                           /* Pump ratio */
        0xEAU, 2, 0x00, 0x00,                     /* Driver timing B */
        0xC0U, 1, 0x23,                           /* Power control 1: 4.6 V */
        0xC1U, 1, 0x10,                           /* Power control 2 */
        0xC5U, 2, 0x3E, 0x28,                     /* VCOM control 1 */
        0xC7U, 1, 0x86,                           /* VCOM control 2 */
        CMD_MADCTL, 1, ILI9341_MADCTL,            /* Hướng quét + BGR */
        CMD_PIXFMT, 1, 0x55,                      /* 16 bit/pixel */
        0xB1U, 2, 0x00, 0x18,                     /* Frame rate ~79 Hz */
        0xB6U, 3, 0x08, 0x82, 0x27,               /* Display function */
        0xF2U, 1, 0x00,                           /* Tắt 3-gamma */
        0x26U, 1, 0x01,                           /* Gamma curve 1 */
        0xE0U, 15, 0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00,
        0xE1U, 15, 0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F,
    };
    if (hal_spi_clock_hz() == 0U) { return HAL_IO_ERROR; }   /* chưa hal_spi_init() */
    if (timeout_ms == 0U) { return HAL_TIMEOUT; } /* init cần chờ reset/sleep-out */
    s_ready = false;
    s_open = false;
    pins_init();

    /* Reset cứng: thấp >= 10 us, chờ 120 ms trước lệnh đầu tiên. */
    rst(false);
    wait_panel_ms(2U);
    rst(true);
    wait_panel_ms(120U);

    hal_status_t st = command(CMD_SWRESET, NULL, 0U, timeout_ms);
    if (st != HAL_OK) { return st; }
    wait_panel_ms(10U);

    for (size_t i = 0; i < sizeof seq; ) {
        const uint8_t cmd = seq[i];
        const uint8_t n = seq[i + 1U];
        st = command(cmd, &seq[i + 2U], n, timeout_ms);
        if (st != HAL_OK) { return st; }
        i += 2U + n;
    }
    st = command(CMD_SLPOUT, NULL, 0U, timeout_ms);
    if (st != HAL_OK) { return st; }
    wait_panel_ms(120U);                           /* datasheet: 120 ms sau Sleep Out */
    st = command(CMD_DISPON, NULL, 0U, timeout_ms);
    if (st != HAL_OK) { return st; }
    s_ready = true;
    return HAL_OK;
}

hal_status_t ili9341_begin(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t timeout_ms)
{
    if (!s_ready || s_open) { return HAL_IO_ERROR; }
    if (x >= ILI9341_WIDTH || y >= ILI9341_HEIGHT || w == 0U || h == 0U) { return HAL_INVALID_ARG; }
    if (timeout_ms == 0U) { return HAL_TIMEOUT; } /* không phát lệnh khi không được chờ */
    if (w > ILI9341_WIDTH - x) { w = (uint16_t)(ILI9341_WIDTH - x); }
    if (h > ILI9341_HEIGHT - y) { h = (uint16_t)(ILI9341_HEIGHT - y); }

    hal_status_t st = hal_spi_lock(timeout_ms);
    if (st != HAL_OK) { return st; }
    const uint16_t x1 = (uint16_t)(x + w - 1U);
    const uint16_t y1 = (uint16_t)(y + h - 1U);
    const uint8_t ca[4] = { (uint8_t)(x >> 8), (uint8_t)x, (uint8_t)(x1 >> 8), (uint8_t)x1 };
    const uint8_t pa[4] = { (uint8_t)(y >> 8), (uint8_t)y, (uint8_t)(y1 >> 8), (uint8_t)y1 };
    cs(false);
    st = write_cmd(CMD_CASET, ca, sizeof ca, timeout_ms);
    if (st == HAL_OK) { st = write_cmd(CMD_PASET, pa, sizeof pa, timeout_ms); }
    if (st == HAL_OK) { st = write_cmd(CMD_RAMWR, NULL, 0U, timeout_ms); }
    if (st != HAL_OK) {
        cs(true);
        hal_spi_unlock();
        return st;
    }
    s_open = true;                                 /* DC = 1: các byte sau là điểm ảnh */
    return HAL_OK;
}

hal_status_t ili9341_push(const void *pixels_be, size_t bytes, uint32_t timeout_ms)
{
    if (!s_open) { return HAL_IO_ERROR; }
    return hal_spi_write((const uint8_t *)pixels_be, bytes, timeout_ms);
}

void ili9341_end(void)
{
    if (!s_open) { return; }
    cs(true);
    s_open = false;
    hal_spi_unlock();
}

hal_status_t ili9341_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color,
                               uint32_t timeout_ms)
{
    hal_status_t st = ili9341_begin(x, y, w, h, timeout_ms);
    if (st != HAL_OK) { return st; }
    if (w > ILI9341_WIDTH - x) { w = (uint16_t)(ILI9341_WIDTH - x); }
    if (h > ILI9341_HEIGHT - y) { h = (uint16_t)(ILI9341_HEIGHT - y); }
    st = hal_spi_write_repeat16(color, (uint32_t)w * h, timeout_ms);
    ili9341_end();
    return st;
}

hal_status_t ili9341_blit(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const void *pixels_be,
                          uint32_t timeout_ms)
{
    if (pixels_be == NULL) { return HAL_INVALID_ARG; }
    if (x + (uint32_t)w > ILI9341_WIDTH || y + (uint32_t)h > ILI9341_HEIGHT) { return HAL_INVALID_ARG; }
    hal_status_t st = ili9341_begin(x, y, w, h, timeout_ms);
    if (st != HAL_OK) { return st; }
    st = ili9341_push(pixels_be, (size_t)w * h * 2U, timeout_ms);
    ili9341_end();
    return st;
}
