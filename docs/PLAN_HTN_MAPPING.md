# Đối chiếu plan môn Hệ thống nhúng với dự án

Phạm vi đã chốt: **3 ngoại vi chính SPI1 + I2C1 + ADC1** (GPIO/EXTI cho nút và chân TFT), chủ đề mở rộng mục 6: **State machine, event-driven firmware**. Đã bỏ: buzzer PWM (Timer), UART debug, DMA.

Ký hiệu: ✅ dùng trong dự án · ➖ có mặt nhưng không phải trọng tâm / dùng dạng khác · ❌ không dùng.

## 1–2. Chuẩn bị và giới thiệu hệ thống nhúng

| Mục plan | | Thể hiện trong dự án |
|---|---|---|
| Toolchain Linux: Arm GNU Toolchain, arm-none-eabi-gdb, stlink-tools | ✅ | `Makefile` + `arm-none-eabi-gcc`, nạp `st-flash`, debug GDB (`docs/BUILD.md`) |
| Windows: KeilC, Hercules | ➖ | Build qua WSL/Arm GNU Toolchain; không dùng Keil, không cần Hercules (đã bỏ UART) |
| Phần cứng: STM32F1, ST-Link | ✅ | STM32F103C8T6 (Blue Pill), nạp/debug qua SWD |
| USB–TTL | ❌ | Không cần sau khi bỏ UART |
| LED, nút, I2C, SPI, ADC sensor | ✅ | LED PC13, 2 nút, OLED I2C, TFT SPI, joystick ADC |
| Đồng hồ đo dòng | ❌ | Không đo (không làm power management) |
| Biên dịch, nạp code lên board | ✅ | `make check`, `make -C tests/hw/<x> flash` |

## 3. Ngoại vi (Bare-Metal)

### Cơ bản — yêu cầu tối thiểu 3

| Ngoại vi | | Dùng cho | File | Người |
|---|---|---|---|---|
| **SPI** | ✅ ngoại vi chính 1 | TFT ILI9341 240×320, 18 MHz, khóa giao dịch CS/DC | `hal_spi.c`, `ili9341.c` | TV1 |
| **I2C** | ✅ ngoại vi chính 2 | OLED SSD1306: HUD + số đo FPS/heap/stack; timeout, khôi phục bus | `hal_i2c.c`, `ssd1306.c`, `oled_view.c` | TV3 |
| **ADC** | ✅ ngoại vi chính 3 | Joystick 2 trục 12 bit, hiệu chuẩn tâm, vùng chết | `hal_adc.c`, `task_hmi.c` | TV2 |
| GPIO | ✅ hỗ trợ | Nút Fire/Start, CS/DC/RESET của TFT, LED PC13 | `hal_gpio.c`, `ili9341.c`, `bsp.c` | TV2, TV1 |
| Interrupt | ✅ hỗ trợ | EXTI15_10 cho nút, ưu tiên 6 ≥ mức syscall FreeRTOS | `hal_gpio.c` | TV2 |
| UART | ❌ bỏ | Số đo chuyển sang OLED | — | — |
| Timer | ❌ bỏ | Buzzer PWM ngoài phạm vi; SysTick chỉ do FreeRTOS dùng | — | — |

### Nâng cao

| Mục | | Ghi chú |
|---|---|---|
| DMA | ❌ bỏ | SPI polling đủ nhanh (~1,5 ms/khung); đề cương ghi DMA "không bắt buộc" |
| Power management | ❌ | Không làm |
| Lập lịch | ➖ | Dùng bộ lập lịch FreeRTOS (mục 4) thay vì lập lịch bare-metal |
| Watchdog | ❌ | Không làm |
| Kiểm soát bộ nhớ | ✅ | Không framebuffer (150 KiB > 20 KiB SRAM), bộ đệm dải 1.280 B, ngân sách RAM theo map file (~14,8 KB / 20 KB) |

## 4. FreeRTOS

| Mục plan | | Thể hiện trong dự án |
|---|---|---|
| Khởi tạo task, quản lý trạng thái task | ✅ | 3 task HMI / GAME / RENDER, `task_xxx_create()`, kiểm tra `pdPASS` |
| Định kỳ, chiếm quyền, ưu tiên | ✅ | `xTaskDelayUntil` 10 ms (HMI), 20 ms (GAME); preemptive, ưu tiên 3/3/2 |
| Queue | ✅ | `g_input_queue` (sự kiện), `g_render_queue` (snapshot dài 1, overwrite) |
| Semaphore | ➖ | Không dùng binary semaphore; dùng Task Notification (nhẹ hơn) và mutex |
| ISR–task | ✅ | EXTI -> `vTaskNotifyGiveFromISR` -> HMI, `portYIELD_FROM_ISR` |
| Mutex | ✅ | Khóa bus SPI/I2C/ADC, timeout hữu hạn, priority inheritance |
| Event group, software timer | ❌ | Không dùng (`configUSE_TIMERS 0`) |
| Heap và stack (mở rộng) | ✅ | heap_4 10 KiB; OLED hiện `HEAP free/min`, `STK` high-water mark 3 task; hook stack overflow / malloc fail |
| Phân tích lập lịch, hiệu năng (mở rộng) | ✅ | FPS thực, thời gian vẽ lâu nhất (DWT), bộ đếm chu kỳ GAME bị trễ |

## 5. Bài tập lớn — mục tiêu

| Mục tiêu | | Thể hiện trong dự án |
|---|---|---|
| Sử dụng phối hợp ngoại vi | ✅ | ADC + GPIO/EXTI (input) -> FSM -> SPI (TFT) + I2C (OLED) trong một hệ thống |
| Quản lý luồng thông tin | ✅ | HMI -> queue sự kiện -> GAME -> snapshot + notification -> RENDER; GAME là nơi duy nhất ghi state |
| Đánh giá hoạt động | ✅ (đo trên kit) | Số đo trên OLED; bảng `docs/EVALUATION.md` — **nhóm điền số thật** |
| Xây dựng kiến trúc | ✅ | BSP / HAL / APP, sơ đồ trong `docs/ARCHITECTURE.md` |
| Module hóa phần mềm, thư viện | ✅ | Driver tách file + header API, engine thuần C tách khỏi RTOS |
| Đóng gói và thử nghiệm | ✅ | Quy trình Git main/develop/feature/integration, CI, harness test kit, tag `v1.0.0` |
| Mở rộng tối thiểu 1 chủ đề mục 6 | ✅ | **State machine, event-driven firmware** (xem dưới) |
| Nhiệm vụ: dự án nhúng cơ bản dùng MCU | ✅ | Game bắn gà trên STM32F103 + FreeRTOS |

## 6. Các vấn đề khác — chủ đề được chọn

| Chủ đề | | Ghi chú |
|---|---|---|
| **State machine, event-driven firmware** | ✅ **chủ đề chính** | FSM 4 trạng thái MENU/PLAYING/PAUSED/GAME_OVER; nút -> ngắt -> notification -> sự kiện -> queue -> FSM; sự kiện sai trạng thái bị bỏ qua và đếm; cập nhật định kỳ 20 ms song song. Minh chứng: bảng + sơ đồ trạng thái (`ARCHITECTURE.md`), unit test 44/44 cặp (trạng thái, sự kiện) |
| Debug firmware | ➖ | Có hỗ trợ (LED báo lỗi, hook FreeRTOS, GDB) nhưng không trình bày như chủ đề |
| Testing — Unit Test | ➖ | Chỉ dùng làm minh chứng cho FSM (`tests/host`) |
| Test-Driven Development | ❌ | |
| Coding standard | ➖ | `-Wall -Wextra -Werror`, quy ước commit; không theo chuẩn MISRA |
| Static analysis | ❌ | |
| Bootloader and OTA | ❌ | |

## Cần tầm soát với thầy / cập nhật đề cương

1. **Đề cương mục 2** còn tính năng 4 "Buzzer PWM": cần xóa. **Bảng ngoại vi** còn dòng TIM3_CH3 (PB0) và USART1 (PA9/PA10): cần xóa.
2. **Phân công**: TV3 từ "I2C + PWM" thành "I2C" (OLED HUD + số đo + unit test FSM).
3. **Bộ nhớ**: đề cương ghi bộ đệm TFT 2 × 1.280 B = 2.560 B (cho DMA); dự án dùng 1 × 1.280 B vì truyền polling.
4. GPIO/EXTI vẫn dùng (nút, chân TFT) nhưng không tính vào 3 ngoại vi chính; nếu thầy tính GPIO là ngoại vi thì dự án có 4.
5. "Đánh giá hoạt động" chỉ hoàn tất khi nhóm điền số đo kit vào `docs/EVALUATION.md`.
