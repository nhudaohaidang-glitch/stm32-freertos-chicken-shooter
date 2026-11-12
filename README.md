# Game bắn gà STM32 / FreeRTOS

Đề tài: **Hệ thống đồ họa thời gian thực trên vi điều khiển STM32 sử dụng FreeRTOS – ứng dụng game bắn gà**.

**Trạng thái:** đã ghép phần mềm với **3 ngoại vi chính SPI1 (TFT), I2C1 (OLED), ADC1 (joystick)** + GPIO/EXTI cho nút: BSP 72 MHz, ba task HMI/GAME/RENDER, **state machine hướng sự kiện** (chủ đề mở rộng mục 6), render vùng bẩn không framebuffer, OLED hiển thị điểm/máu và thống kê FPS/heap/stack. Đã build và unit test FSM trên PC; **CHƯA TEST trên kit**. **Kết quả trên kit phải được nhóm đo và điền vào [docs/EVALUATION.md](docs/EVALUATION.md) trước khi release `v1.0.0`.**

## Đọc trước khi làm

1. [Quy trình Git của nhóm](docs/GIT_WORKFLOW.md): nhánh, PR, review, tích hợp và release.
2. [Phân công và đầu việc](docs/PLAN.md): ba thành viên, nhánh và điều kiện hoàn thành từng giai đoạn.
3. [Phần cứng và chân kết nối](docs/HARDWARE.md).
4. [Kiến trúc, API và IPC](docs/ARCHITECTURE.md).
5. [Hướng dẫn build](docs/BUILD.md), [kế hoạch kiểm thử](docs/TEST_PLAN.md), [đánh giá FPS/heap/stack](docs/EVALUATION.md).
6. [Nguồn yêu cầu và các giả định](docs/REQUIREMENTS.md), [đối chiếu plan môn học](docs/PLAN_HTN_MAPPING.md).

## Phạm vi đề tài

- STM32F103C8T6 (Flash 64 KiB, SRAM 20 KiB), TFT ILI9341 240×320 qua SPI1.
- OLED SSD1306 128×64 qua I2C1; joystick ADC1; hai nút Fire và Start/Pause.
- Đã bỏ khỏi phạm vi: buzzer PWM, UART debug, DMA (xem [đối chiếu plan](docs/PLAN_HTN_MAPPING.md)).
- Ba task: HMI, GAME, RENDER; GAME cập nhật 20 ms và là nơi duy nhất ghi game state.
- FSM: MENU, PLAYING, PAUSED, GAME_OVER. Queue + Task Notification cho IPC.
- Không dùng framebuffer RGB565 toàn màn hình (153.600 byte). TFT: một buffer dải 1.280 byte; OLED: 1.024 byte pixel và 8 byte điều khiển. Đo FPS thực tế, không cam kết FPS trước khi đo.

![Sơ đồ khối từ đề cương](docs/references/hardware-block-diagram.png)

## Cách chơi

| Điều khiển | Tác dụng |
|---|---|
| Joystick trục X | Di chuyển tàu (càng gạt xa càng nhanh, 1..4 px/khung) |
| Fire (PB12) | Bắn; giữ để bắn tự động mỗi 200 ms |
| Start/Pause (PB13) | MENU -> chơi; đang chơi -> tạm dừng / tiếp tục; GAME OVER -> ván mới |

18 con gà (3 × 6) bay ngang và hạ dần, thả trứng ngẫu nhiên. Trúng gà +10 × đợt; hết gà sang đợt mới nhanh hơn (+50 × đợt). Trúng trứng mất 1/3 tim; hết tim hoặc gà xuống tới tàu là thua. OLED hiển thị trạng thái, điểm, tim, đợt và số đo đánh giá (FPS, thời gian vẽ, heap, stack).

## Cấu trúc

```text
bsp/                 clock 72 MHz, NVIC, DWT, LED, startup (leader)
hal/inc, hal/src     hal_spi + ili9341, hal_i2c + ssd1306, hal_adc, hal_gpio, hal_sys
middlewares/         FreeRTOS V11.1.0, GCC ARM_CM3, heap_4, giấy phép MIT
app/inc, app/src     app_cfg, game_types (snapshot v2), app_ipc, game_engine, render_tft, oled_view, task_hmi/game/render
config/              FreeRTOSConfig.h (chỉ leader thay đổi)
linker/              Flash 64 KiB / RAM 20 KiB, chừa 1 KiB MSP
main.c               điểm tích hợp: BSP -> HAL -> IPC -> task -> scheduler
tests/hw/            harness test từng nhóm driver trên kit (adc_gpio, i2c_oled, spi_tft)
tests/host/          unit test FSM / game engine trên PC
docs/                kiến trúc, phần cứng, quy trình Git, kiểm thử, đánh giá
```

## Bắt đầu

```sh
make -j4 check                 # firmware: build/firmware.bin/.hex/.elf/.map
make -j4 OPT=-O2 check         # bản demo/đo FPS
make -C tests/host test        # unit test FSM trên PC (gcc)
```

Cần `arm-none-eabi-gcc`, Binutils, Newlib, Make (xem [BUILD](docs/BUILD.md)). Nạp: `st-flash --reset write build/firmware.bin 0x08000000` hoặc STM32CubeProgrammer. Số đo FPS/heap/stack hiện trên OLED.

## Lịch sử nhánh

Phát triển theo quy trình của thầy ([GIT_WORKFLOW](docs/GIT_WORKFLOW.md)): `main` (bản ổn định, gắn tag), `develop` (tích hợp), `feature/*` (mỗi người một nhánh, PR về develop, người khác review), `integration/system-v1` (ghép hệ thống), release `develop -> main` + tag `v1.0.0`.
