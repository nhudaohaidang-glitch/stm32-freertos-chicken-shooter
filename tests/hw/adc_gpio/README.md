# Test driver ADC (joystick) + GPIO (nút) trên kit

Chương trình test riêng cho nhánh `feature/hal-adc-gpio`, không thay `main.c` của game. Chạy ở clock 8 MHz (HSI), khớp `configCPU_CLOCK_HZ` hiện tại. Kết quả xem qua LED PC13 và biến `g_test` (đọc bằng gdb).

## Nối dây

| Kit | Thiết bị |
|---|---|
| PA0 | Joystick VRx |
| PA1 | Joystick VRy |
| 3V3 / GND | Joystick +5V / GND (**cấp 3,3 V, không cấp 5 V**) |
| PB12 | Nút Fire, chân còn lại nối GND |
| PB13 | Nút Start/Pause, chân còn lại nối GND |
| PC13 | LED có sẵn trên Blue Pill |

Nếu nút nối lên 3,3 V thay vì GND: thêm `-DHAL_GPIO_BUTTON_ACTIVE_LOW=0` vào `CFLAGS` trong `tests/hw/adc_gpio/Makefile`, chạy `make -C tests/hw/adc_gpio clean` rồi build lại.

## Build, nạp và xem kết quả trên Linux (GCC)

Cài công cụ một lần (Ubuntu/Debian):

```sh
sudo apt-get install -y gcc-arm-none-eabi libnewlib-arm-none-eabi make stlink-tools gdb-multiarch
```

Build và nạp qua ST-Link, chạy từ thư mục gốc repo:

```sh
make -C tests/hw/adc_gpio            # tạo build/test_adc_gpio/test_adc_gpio.bin
make -C tests/hw/adc_gpio flash      # nạp bằng st-flash
```

Chương trình không gọi `SystemInit`, nên chip giữ HSI 8 MHz sau reset, khớp `FreeRTOSConfig.h`; không cần chỉnh clock.

Xem biến `g_test` bằng gdb (hai terminal):

```sh
st-util                                               # terminal 1: GDB server, cổng 4242
gdb-multiarch build/test_adc_gpio/test_adc_gpio.elf   # terminal 2
(gdb) target extended-remote :4242
(gdb) continue          # cho chạy, thao tác joystick/nút
^C                      # dừng lại
(gdb) print g_test
(gdb) continue
```

Nếu `st-flash` báo không thấy ST-Link: rút cắm lại, chạy `st-info --probe`; với ST-Link clone có thể cần giữ nút RESET trên board khi bắt đầu nạp.

## Kết quả mong đợi

| Hạng mục | Quan sát |
|---|---|
| Khởi tạo | LED nháy chậm 1 Hz; `adc_init = 0`, `gpio_init = 0`. LED nháy rất nhanh = init lỗi |
| Joystick | Đẩy lệch xa: LED nháy nhanh 5 Hz; thả tay: 1 Hz |
| Dải ADC | Xoay hết các hướng: `x_min`, `y_min` gần 0; `x_max`, `y_max` gần 4095 |
| Giá trị giữa | Thả tay, ghi lại `x`, `y` (khoảng 2048, mỗi module lệch một ít) |
| Fire | Giữ: LED sáng liên tục, `buttons = 1` |
| Start/Pause | Giữ: LED tắt liên tục, `buttons = 2` |
| Ngắt EXTI | Nhấn mỗi nút 20 lần: `fire_count`, `start_count` khoảng 20. Lớn hơn nhiều nghĩa là nút dội, HMI cần chống dội |
| Đánh thức bằng ngắt | `wakeups` tăng mỗi lần nhấn (ngắt gọi `vTaskNotifyGiveFromISR`) |
| Ổn định | Sau 10 phút `read_errors = 0`, LED vẫn nháy |

## Ghi kết quả (điền khi test, đính kèm ảnh/video vào PR)

| Ngày | Commit | Board / module | Mục | Kỳ vọng | Thực tế | PASS/FAIL | Người test |
|---|---|---|---|---|---|---|---|
| | | | Dải X / Y (min–max) | ~0–4095 | | | |
| | | | Giá trị giữa X / Y | ~2048 | | | |
| | | | Nhiễu khi để yên | ghi biên dao động | | | |
| | | | Fire / Start | buttons = 1 / 2 | | | |
| | | | 20 lần nhấn | count ≈ 20 | | | |
| | | | Sau 10 phút | read_errors = 0 | | | |
