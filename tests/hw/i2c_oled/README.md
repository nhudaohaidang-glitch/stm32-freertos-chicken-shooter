# Harness kiểm tra M1: I2C1 + SSD1306

Firmware test riêng cho I2C/OLED (nhánh `feature/hal-i2c-pwm`; buzzer PWM đã bỏ khỏi phạm vi dự án). Không thay `main.c`, không gọi BSP (còn stub), chạy ở clock reset HSI 8 MHz, khớp `configCPU_CLOCK_HZ` hiện tại.

## Nối dây

| Kit | Thiết bị |
|---|---|
| PB6 | OLED SCL |
| PB7 | OLED SDA |
| 3V3 / GND | OLED VCC / GND |
| PC13 | LED có sẵn trên Blue Pill |

Module OLED thường đã có điện trở kéo lên 4,7–10 kΩ trên SCL/SDA. Nếu module không có, phải thêm điện trở kéo lên 3,3 V.

## Build và nạp

```sh
make -C tests/hw/i2c_oled            # tạo build/test_i2c_oled/test_i2c_oled.bin
make -C tests/hw/i2c_oled flash      # cần stlink-tools (st-flash)
```

Trên Windows có thể nạp file `.bin` bằng STM32CubeProgrammer, địa chỉ 0x08000000.

## Kết quả mong đợi

| Hạng mục | Quan sát |
|---|---|
| Scheduler | LED PC13 nháy 2 Hz đều. Nháy nhanh 10 Hz là không thấy thiết bị I2C nào |
| Địa chỉ OLED | Dòng 1 `ADDR 0X3C DEV 1` (hoặc 3D); `DEV` là số thiết bị tìm thấy trên bus |
| Clock | Dòng 2 `PCLK1 8MHZ` |
| Tốc độ I2C | Dòng 3 `FULL xx MS PAGE y MS` (thời gian gửi 8 page / 1 page) |
| NACK | Dòng 4 `NACK OK n MS`: ghi tới địa chỉ trống trả lỗi, không treo |
| Cập nhật page | Dòng 6 bộ đếm tăng mỗi 100 ms, `ERR 0`; thanh dưới cùng chạy |
| Khôi phục bus | Rút dây SDA vài giây rồi cắm lại: `ERR` tăng, sau đó màn tự vẽ lại, LED vẫn nháy |

## Ghi kết quả (điền khi test, đính kèm ảnh/video vào PR)

| Ngày | Commit | Board / module OLED | Mục | Kỳ vọng | Thực tế | PASS/FAIL | Người test |
|---|---|---|---|---|---|---|---|
| | | | Địa chỉ OLED | 0x3C/0x3D | | | |
| | | | FULL / PAGE | ~25 ms / ~3 ms @ 381 kHz | | | |
| | | | NACK | IO_ERROR, < 5 ms | | | |
| | | | Rút/cắm SDA | Tự hồi phục | | | |
| | | | LED sau 10 phút | Vẫn nháy, ERR không tăng | | | |
