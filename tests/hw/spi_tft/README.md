# Harness M1: SPI1 + ILI9341

Test riêng cho `feature/hal-spi-tft`; không thay main.c, BSP, clock hoặc cấu hình FreeRTOS chung. BSP hiện là stub, nên harness không gọi bsp_system_init(). Sau hardware reset, HSI/PCLK2 là 8 MHz, SPI1 chọn SCK 4 MHz. Chưa tích hợp TASK_RENDER/game.

## Nối dây

| STM32F103C8T6 | TFT ILI9341 |
|---|---|
| PA5 | SCK |
| PA7 | SDI/MOSI |
| PA4 | CS |
| PA3 | DC/RS |
| PB1 | RESET |
| GND | GND chung |

Logic 3,3 V. Cấp VCC/LED nền theo nhãn và mạch module thật. MISO/SDO không cần cho đường vẽ một chiều. PC13 là LED quan sát trên Blue Pill.

## Build và nạp

```sh
make -j2 check
make -C tests/hw/spi_tft
make -C tests/hw/spi_tft flash
```

Cần ARM GCC, binutils, newlib và make; lệnh flash dùng st-flash. Có thể dùng STM32CubeProgrammer nạp `build/test_spi_tft/test_spi_tft.bin` tại 0x08000000. Reset board sau khi nạp để về HSI 8 MHz. Nếu thay đổi clock, người phụ trách BSP phải đồng bộ clock thực với configCPU_CLOCK_HZ trước khi chạy scheduler.

Không dùng firmware.bin của root để đánh giá driver: main tích hợp hiện dừng ở BSP stub. Dùng đúng firmware của harness.

## Quan sát và số đo

- Sọc trái sang phải: trắng, vàng, cyan, xanh lá, tím, đỏ, xanh dương, đen. Kiểm tra màu/hướng và macro ILI9341_MADCTL.
- Viền đỏ 1 pixel đủ bốn mép kiểm tra CASET/PASET và kích thước 240x320.
- Ô vuông xanh 16x16 chạy ngang; LED PC13 khoảng 1 Hz nếu không trễ chu kỳ. Lỗi driver: LED nháy nhanh khoảng 10 Hz và biến lỗi có thể đọc bằng GDB.
- Dòng FILL/BLIT là thời gian đo theo tick, đơn vị ms. `SPI MHZ 4` là kỳ vọng theo cấu hình reset hiện tại, phải xác minh SCK thật khi có kit.
- Biến quan sát: s_init_status, s_fill_ms, s_blit_ms, s_spi_hz, s_io_errors, s_last_error.
- 153.600 byte ở SCK 4 MHz cần ít nhất khoảng 307,2 ms trên dây, chưa tính phần mềm/lệnh/preemption. Không dùng số 70 ms của cấu hình 18 MHz để nghiệm thu harness này. Timeout harness là 5.000 ms mỗi thao tác bus.

SPI write-only không nhận ACK từ TFT: HAL_OK chỉ chứng minh đường code truyền hoàn tất, không chứng minh dây nối/màu/hướng hiển thị đúng. Đo và quan sát thực tế.

## API và phụ thuộc

- Bốn file driver chính: hal_spi.h/.c và ili9341.h/.c.
- hal_sys.h/.c: đọc RCC, deadline và critical section. font5x7.h/.c dùng cho chữ trên harness. Bốn helper/font này giữ nguyên từng byte từ feature/hal-i2c-pwm để khi ghép nhánh không tạo hai bản khác nhau.
- Gọi init một lần trước khi các task dùng bus. ILI9341 có một chủ sở hữu; begin/push/end giữ mutex cho toàn CS/DC + payload. Hàm SPI write không tự khóa.
- timeout=0 là không chờ: lock thử ngay; truyền có dữ liệu và init/begin TFT trả HAL_TIMEOUT trước khi phát dữ liệu. Một lần write rỗng có thể trả HAL_OK nếu bus đang idle.
- Timeout áp dụng từng lời gọi, không phải deadline chung cho toàn bộ reset/init hoặc frame. Task API không dùng từ ISR, critical section hoặc khi scheduler bị suspend.

## Kết quả phần cứng

**CHƯA CHẠY KIT** trong phiên chuẩn bị code. Điền số thực và ảnh/log vào bảng trước khi đề nghị merge về develop.

| Ngày/commit | Board/module/clock | Tình huống | Kỳ vọng | Thực tế | PASS/FAIL/CHƯA CHẠY |
|---|---|---|---|---|---|
| | | SCK và tick | Khớp clock cấu hình | | CHƯA CHẠY |
| | | Sọc màu/viền | Đúng màu và đủ mép | | CHƯA CHẠY |
| | | FILL/BLIT | Ghi ms đo thật | | CHƯA CHẠY |
| | | Vùng nhỏ | Xóa chỗ cũ, vẽ chỗ mới | | CHƯA CHẠY |
| | | Lỗi/timeout và lần gọi tiếp theo | Trả lỗi hữu hạn, xử lý/khôi phục đúng | | CHƯA CHẠY |
| | | Heap/stack/chạy lâu | Ghi số đo và thời lượng thật | | CHƯA CHẠY |

Chỉ push nhánh feature, không tự merge hoặc đánh dấu M1 đã đạt chỉ nhờ build.

## Kiểm tra đã chạy trước khi push

Ngày 12/11/2026, trên môi trường build (không có kit):

- `make -j2 check`: PASS với ARM GCC 13.2.1, `-Wall -Wextra -Werror`.
- `make -C tests/hw/spi_tft`: PASS, liên kết đầy đủ driver trong harness.
- `make -C tests/host/spi_tft test`: PASS, 53 kiểm tra logic với host GCC và UBSan. Xem [giới hạn mô hình PC](../../host/spi_tft/README.md).

Không có số đo/ảnh phần cứng để ghi PASS cho kit. Không đưa ELF/BIN/HEX/MAP hoặc thư mục build vào commit.
