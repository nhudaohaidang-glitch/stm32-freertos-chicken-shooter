# Đánh giá hệ thống (FPS, heap/stack, kiểm thử chức năng)

Tài liệu tách rõ **số đã có không cần kit** và **số phải đo trên kit**. Không chép số kiểm thử PC sang cột kit.

**Trạng thái: CHƯA TEST trên kit.** Chưa có số đo FPS, thời gian vẽ, clock thực, heap/stack hoặc kết quả chức năng phần cứng. Giữ bảng mục 3 và 4 chờ nhóm điền sau khi chạy thật.

## 1. Cách lấy số liệu trên kit

1. Build bản demo: `make clean && make -j4 OPT=-O2 check`, nạp `build/firmware.bin` tại 0x08000000.
2. Số đo hiện trên **OLED** (3 dòng dưới), cập nhật theo chu kỳ danh nghĩa 1 s, không cần UART. **Các số dưới đây chỉ minh họa định dạng, không phải kết quả đã đo:**

```text
FPS 50 T 01800US
HEAP 05616/05616
STK 162/198/314
```

| Dòng | Ý nghĩa |
|---|---|
| `FPS nn` | Code hiện chốt số khung TFT có vẽ vùng trong cửa sổ danh nghĩa 1 s; MENU/PAUSED không đổi thường là 00. Nhịp GAME 20 ms không bảo đảm TFT đạt 50 FPS. |
| `T nnnnnUS` | Thời gian `render_tft_draw` lâu nhất trong 1 s (đo bằng DWT, µs) |
| `HEAP a/b` | `xPortGetFreeHeapSize()` / `xPortGetMinimumEverFreeHeapSize()` (byte) |
| `STK h/g/r` | `uxTaskGetStackHighWaterMark()` của HMI / GAME / RENDER: số word **còn trống** nhỏ nhất |

3. Chụp ảnh OLED ở từng tình huống trong bảng mục 3. Bộ đếm phụ (`g_ipc_stats.input_drops`, `game_overruns`, `s_info`) đọc bằng ST-Link + GDB nếu cần.

**Điểm cần review trước khi nghiệm thu FPS:** `update_stats()` đang lấy trực tiếp số frame, chưa chuẩn hóa theo thời gian thực khi cửa sổ bị trễ. Ví dụ 40 frame trong 1,25 s tương đương 32 FPS. Chưa dùng giá trị hiển thị làm FPS chính xác cho tới khi kiểm tra/xử lý điểm này. `T` đo thời gian trôi qua của lời gọi render, có thể gồm thời gian bị task khác chiếm CPU.

## 2. Kết quả đã có không cần kit

| Hạng mục | Công cụ | Kết quả |
|---|---|---|
| FSM đúng bảng chuyển trạng thái | `make -C tests/host test` (gcc + ASan/UBSan) | 44/44 cặp (trạng thái, sự kiện) đúng; tổng 124 kiểm tra PASS |
| Bất biến game | `tests/host/test_engine.c` | 200.000 khung ngẫu nhiên, không vi phạm (HP, vị trí, đạn, bitmask gà) |
| Build | `make check` | Không warning (`-Wall -Wextra -Werror`) |
| RAM tĩnh | map file | `_ebss` + MSP ≈ 14,8 KB / 20 KB |
| Flash | `arm-none-eabi-size` | ~24 KB / 64 KB |

Kiểm tra trong phiên tích hợp ngày 12/11/2026, Arm GNU GCC 13.2.1: build sạch `-Og` và `-O2` đều PASS, không warning; `make -C tests/host test` đạt **124 passed, 0 failed** với ASan/UBSan. Bản `-O2`: Flash **24.164 byte**, `_ebss = 0x20003750`, dự trữ MSP 1.024 byte. Đây là dữ liệu build/map, không phải số đo heap/stack khi chạy.

Lưu lượng SPI ước lượng khi thiết kế: trung bình ~3,4 KB/khung (~1,5 ms ở 18 MHz), vẽ toàn màn 153,6 KB (~68 ms). **FPS, thời gian vẽ, heap/stack thật chỉ có giá trị khi đo trên kit.**

## 3. Bảng đo trên kit (nhóm điền, đọc trên OLED)

| Ngày | Commit | Clock | Tình huống | FPS | T (µs) | HEAP free/min | STK H/G/R | Ghi chú |
|---|---|---|---|---|---|---|---|---|
| | | CHƯA ĐO | MENU đứng yên | | | | | |
| | | CHƯA ĐO | PLAYING đợt 1, giữ Fire | | | | | |
| | | CHƯA ĐO | PLAYING đợt ≥ 3, nhiều trứng | | | | | |
| | | CHƯA ĐO | Pause/Resume 10 lần | | | | | |
| | | CHƯA ĐO | Chạy liên tục 10 phút | | | | | |

## 4. Kiểm thử chức năng trên kit (nhóm điền)

| # | Bước | Kỳ vọng | Thực tế | PASS/FAIL |
|---|---|---|---|---|
| 1 | Cấp nguồn | TFT hiện MENU, OLED `MENU`, LED PC13 không nháy nhanh | | |
| 2 | Nhấn Start | PLAYING, gà di chuyển, OLED điểm 000000, 3 tim | | |
| 3 | Gạt joystick trái/phải | Tàu đi đúng hướng, đứng yên khi thả | | |
| 4 | Nhấn / giữ Fire | Đạn bay; giữ thì bắn tự động | | |
| 5 | Bắn trúng gà | Gà nổ, điểm +10 × đợt | | |
| 6 | Trúng trứng | Mất 1 tim, tàu nhấp nháy ~1 s | | |
| 7 | Start khi đang chơi | PAUSED, mọi thứ đứng yên; nhấn lại tiếp tục đúng chỗ | | |
| 8 | Hết 3 tim / gà xuống đáy | GAME OVER + điểm | | |
| 9 | Start ở GAME OVER | Ván mới, điểm về 0 | | |
| 10 | Nhấn Fire ở MENU / PAUSED | Không có tác dụng (FSM bỏ qua sự kiện sai trạng thái) | | |
| 11 | Rút dây SDA OLED rồi cắm lại | Game không dừng, OLED tự hiện lại | | |
| 12 | Nhấn nút liên tục rất nhanh | Không treo, mỗi lần nhấn chỉ một sự kiện | | |
