# Phân công và kế hoạch làm việc

Phạm vi: **3 ngoại vi chính SPI1 + I2C1 + ADC1** (GPIO/EXTI cho nút), chủ đề mở rộng mục 6: **State machine, event-driven firmware**. Bỏ buzzer PWM, UART, DMA.

| Vai trò | GitHub | Ngoại vi | Nhánh HAL (M1) | Nhánh APP (M2) |
|---|---|---|---|---|
| Leader + TV1 | `nhudaohaidang-glitch` | SPI1 (TFT) + cấu hình chung | `feature/bsp-clock`, `feature/hal-spi-tft` | `feature/app-api`, `feature/task-render`, `integration/system-v1` |
| TV2 | `hoangdat06022005-commits` | ADC1 (joystick) + GPIO/EXTI (nút) | `feature/hal-adc-gpio` | `feature/task-hmi`, `feature/task-game` (FSM) |
| TV3 | Nguyễn Khắc Lộc | I2C1 (OLED) | `feature/hal-i2c-pwm` (phần PWM đã gỡ) | `feature/oled-view-tests` (OLED HUD + số đo, unit test FSM, CI) |

Nhóm sửa bảng nếu phân vai khác thực tế. Mỗi người commit/push bằng tài khoản của mình; người khác review PR. TV3 viết `oled_view.c` để TV1 gọi trong RENDER, tránh hai người cùng sửa `task_render.c`. Không thêm task thứ tư.

## M0 — khởi tạo (đã chuẩn bị cục bộ)

- [x] Cấu trúc BSP/HAL/FreeRTOS/APP, Makefile, linker, pin map.
- [x] Git ignore, khung API HAL/IPC/task, cấu hình ưu tiên 3/3/2.
- [x] Vendor FreeRTOS, build được .elf/.bin/.hex/.map, mẫu CI/PR.
- [x] main bootstrap, develop và ba nhánh HAL từ develop.
- [x] Kho GitHub remote và push.
- [ ] Thêm thành viên, leader và quyền review (leader kiểm tra trong Settings -> Collaborators).
- [ ] Áp dụng branch protection trên GitHub.

Build được khung không có nghĩa M1 hoàn thành. CI được chuẩn bị nhưng chưa chạy trên GitHub khi chưa có remote.

## M1 — bring-up và HAL

| PR -> develop | Người làm | Nội dung | Test kit |
|---|---|---|---|
| `feature/hal-adc-gpio` | TV2 | ADC1 joystick, nút + EXTI, harness | `tests/hw/adc_gpio` |
| `feature/hal-i2c-pwm` | TV3 | I2C1, SSD1306, harness; gỡ PWM/buzzer khỏi phạm vi | `tests/hw/i2c_oled` |
| `feature/bsp-clock` | Leader | 72 MHz, NVIC, DWT, LED, `SystemCoreClock` | qua harness SPI (đo thời gian) |
| `feature/hal-spi-tft` | TV1 | SPI1 polling, ILI9341, harness | `tests/hw/spi_tft` |

Điều kiện sang M2: HAL đã merge, ba harness chạy đúng trên kit, clock và ngắt được xác nhận.

## M2 — task độc lập

| PR -> develop | Người làm | Nội dung |
|---|---|---|
| `feature/app-api` | Leader | Bảng tài nguyên `app_cfg.h`, snapshot v2 `game_types.h`, helper IPC, ARCHITECTURE |
| `feature/task-hmi` | TV2 | `task_hmi.c`: joystick, chống dội, EXTI -> notification, auto-fire, sự kiện |
| `feature/task-game` | TV2 | `game_engine.c` + `task_game.c`: **FSM**, va chạm, điểm, HP, đợt |
| `feature/oled-view-tests` | TV3 | `oled_view.c` (HUD + FPS/heap/stack), unit test FSM 44 ô, CI |
| `feature/task-render` | TV1 | `render_tft.c` + `task_render.c`: vùng bẩn, FPS, số đo |

Mỗi nhánh tạo từ develop mới nhất tại thời điểm bắt đầu; không sửa `main.c`.

## M3 — tích hợp và release

`integration/system-v1` (leader): `main.c` (BSP -> HAL -> IPC -> task -> scheduler), cập nhật tài liệu, đo trên kit theo `docs/EVALUATION.md`. PR integration -> develop (merge commit), rồi PR develop -> main, tag `v1.0.0`.

Không phát hành khi chưa có số đo kit thực; không tạo PR/test/tag giả.
