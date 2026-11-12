# Đánh giá hệ thống (FPS, heap/stack, kiểm thử chức năng)

Tài liệu tách rõ **số đã có không cần kit** và **số phải đo trên kit**. Không chép số kiểm thử PC sang cột kit.

**Trạng thái: ĐÃ CÓ BÁO CÁO ĐO KIT VÀ XÁC NHẬN 12/12 BƯỚC CHỨC NĂNG; CHỜ REVIEW TRƯỚC RELEASE.** Nguyễn Khắc Lộc báo số đo ngày **12/11/2026**, bản `develop` **`1c21dd8`**, clock **72 MHz**, lúc đang chơi và khi mất máu: FPS hiển thị khoảng **30–50**, T **15561–21xxx µs**, `HEAP 5384/5384`, `STK 148/202/288`. Chủ repo xác nhận trong chat rằng đã thử đạt đủ cả 12 bước ở mục 4. Đây là kết quả nhóm cung cấp; CI/phiên cloud không thực hiện test kit. T vẫn là khoảng xấp xỉ và cách tính FPS vẫn cần review theo mục 1; chưa phát hành `v1.0.0`.

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

Nguồn số đo: [review của Nguyễn Khắc Lộc trên PR #15](https://github.com/nhudaohaidang-glitch/stm32-freertos-chicken-shooter/pull/15#pullrequestreview-5876354053). Review xác nhận ngày đo, commit, clock và tình huống; FPS thấp nhất khoảng **30 khi mất máu**, cao nhất **50**. Bảng dùng số cập nhật này thay cho khoảng 37–50 được báo trước đó.

| Thông tin lần đo | Giá trị nhóm xác nhận |
|---|---|
| Người báo số đo | Nguyễn Khắc Lộc — `bestteemo1324-svg` |
| Ngày đo | 12/11/2026 |
| Commit đã nạp | `1c21dd85bf7a48c1644bf75db43e5e0d2deed5ee` (`develop`) |
| Clock | 72 MHz, theo báo cáo người test |
| Tình huống | PLAYING và khi mất máu |
| Ảnh OLED | Người test cho biết đã gửi qua tin nhắn; chưa đính kèm ảnh/log gốc vào tài liệu này |

| Số liệu được báo | Giá trị | Đơn vị / thứ tự | Mức xác nhận |
|---|---|---|---|
| HEAP free/min | 5384 / 5384 | byte | Người test báo trong review, metadata như bảng trên |
| STK H/G/R | 148 / 202 / 288 | word còn trống nhỏ nhất của HMI / GAME / RENDER | Người test báo trong review, metadata như bảng trên |
| FPS hiển thị | khoảng 30–50 | Số trên dòng FPS của OLED | Khoảng thấp nhất khoảng 30 khi mất máu; chưa xác minh cách chuẩn hóa theo thời gian thực |
| T | 15561–21xxx | µs | Người test báo khoảng xấp xỉ; giữ nguyên `21xxx`, chưa có giới hạn trên chính xác |

T được báo từ 15,561 ms đến khoảng 21,xxx ms; chưa tự chuyển `21xxx` thành một số nguyên chính xác. Khoảng FPS là số hiển thị người test quan sát, chưa phải kết quả đã xác minh theo thời gian thực.

Các số thuộc lần test PLAYING/khi mất máu nêu trên; chưa tách theo từng wave hoặc xác nhận lấy đồng thời. `STK` là mức còn trống nhỏ nhất từng có, không phải lượng stack đã dùng. Giữ riêng các phép đo MENU, tải nhiều trứng, 10 lần pause/resume và chạy 10 phút nếu chưa có số tương ứng. Với bản hiện tại phải review cách tính FPS nêu ở mục 1.

Nhóm bổ sung ảnh/log và số riêng cho từng kịch bản chưa đo bên dưới. Không dùng số minh họa ở mục 1 làm kết quả.

| Ngày | Commit | Clock | Tình huống | FPS | T (µs) | HEAP free/min | STK H/G/R | Ghi chú |
|---|---|---|---|---|---|---|---|---|
| 12/11/2026 | `1c21dd8` | 72 MHz (nhóm báo) | PLAYING và khi mất máu | khoảng 30–50 | 15561–21xxx | 5384/5384 | 148/202/288 | Theo review PR #15; chưa tách theo wave, T xấp xỉ |
| | | CHƯA ĐO | MENU đứng yên | | | | | |
| | | CHƯA ĐO | PLAYING đợt 1, giữ Fire | | | | | |
| | | CHƯA ĐO | PLAYING đợt ≥ 3, nhiều trứng | | | | | |
| | | CHƯA ĐO | Pause/Resume 10 lần | | | | | |
| | | CHƯA ĐO | Chạy liên tục 10 phút | | | | | |

## 4. Kiểm thử chức năng trên kit (nhóm điền)

Chủ repo `nhudaohaidang-glitch` xác nhận trong chat ngày 12/11/2026: **"Đã thử đạt đầy đủ cả 12 bước"**. Bảng ghi PASS theo xác nhận test kit của nhóm cho bản tích hợp đang được báo cáo (`1c21dd8`); không coi đây là kết quả do CI hoặc phiên cloud thực hiện. Chưa có ảnh/log riêng cho từng bước đính kèm trong tài liệu.

| # | Bước | Kỳ vọng | Thực tế | PASS/FAIL |
|---|---|---|---|---|
| 1 | Cấp nguồn | TFT hiện MENU, OLED `MENU`, LED PC13 không nháy nhanh | Nhóm xác nhận đã thử đạt | PASS (nhóm xác nhận) |
| 2 | Nhấn Start | PLAYING, gà di chuyển, OLED điểm 000000, 3 tim | Nhóm xác nhận đã thử đạt | PASS (nhóm xác nhận) |
| 3 | Gạt joystick trái/phải | Tàu đi đúng hướng, đứng yên khi thả | Nhóm xác nhận đã thử đạt | PASS (nhóm xác nhận) |
| 4 | Nhấn / giữ Fire | Đạn bay; giữ thì bắn tự động | Nhóm xác nhận đã thử đạt | PASS (nhóm xác nhận) |
| 5 | Bắn trúng gà | Gà nổ, điểm +10 × đợt | Nhóm xác nhận đã thử đạt | PASS (nhóm xác nhận) |
| 6 | Trúng trứng | Mất 1 tim, tàu nhấp nháy ~1 s | Nhóm xác nhận đã thử đạt | PASS (nhóm xác nhận) |
| 7 | Start khi đang chơi | PAUSED, mọi thứ đứng yên; nhấn lại tiếp tục đúng chỗ | Nhóm xác nhận đã thử đạt | PASS (nhóm xác nhận) |
| 8 | Hết 3 tim / gà xuống đáy | GAME OVER + điểm | Nhóm xác nhận đã thử đạt | PASS (nhóm xác nhận) |
| 9 | Start ở GAME OVER | Ván mới, điểm về 0 | Nhóm xác nhận đã thử đạt | PASS (nhóm xác nhận) |
| 10 | Nhấn Fire ở MENU / PAUSED | Không có tác dụng (FSM bỏ qua sự kiện sai trạng thái) | Nhóm xác nhận đã thử đạt | PASS (nhóm xác nhận) |
| 11 | Rút dây SDA OLED rồi cắm lại | Game không dừng, OLED tự hiện lại | Nhóm xác nhận đã thử đạt | PASS (nhóm xác nhận) |
| 12 | Nhấn nút liên tục rất nhanh | Không treo, mỗi lần nhấn chỉ một sự kiện | Nhóm xác nhận đã thử đạt | PASS (nhóm xác nhận) |

## 5. Điều kiện chốt release v1.0.0

Các ô đã đánh dấu dưới đây chỉ ghi việc đã xác nhận tại mốc `develop` **`a861790`** sau khi PR #16 merge. Các bước review, merge release và tạo tag được giữ riêng, chưa đánh dấu trước khi thực hiện.

- [x] Ghi báo cáo kit ngày 12/11/2026 cho commit `1c21dd8`, clock 72 MHz, PLAYING/khi mất máu, khoảng FPS/T và heap/stack tại mục 3.
- [x] Ghi đủ 12/12 bước chức năng PASS theo xác nhận của nhóm tại mục 4.
- [x] Tài liệu phân công và báo cáo kit đã vào `develop` qua PR #15 (`7c2e1b5`) và #16 (`a861790`).
- [x] [CI firmware #57](https://github.com/nhudaohaidang-glitch/stm32-freertos-chicken-shooter/actions/runs/43243685585) của mốc `a861790`: build firmware, build các harness và host-tests thành công.
- [ ] Bổ sung ảnh/log gốc và chốt số T đầy đủ với người test; các kịch bản chưa có số riêng ở mục 3 vẫn để trống.
- [ ] Review/xử lý cách tính FPS theo thời gian thực ở mục 1 trước khi dùng số FPS để nghiệm thu.
- [ ] PR cập nhật checklist này được thành viên khác review và merge vào `develop`.
- [ ] Kiểm tra lại CI của head cuối trên `develop` và PR release sau mọi cập nhật tiếp theo.
- [ ] PR release #2 (`develop -> main`) được người khác review và merge bằng **Create a merge commit**.
- [ ] Sau merge, leader tạo tag `v1.0.0` trỏ vào commit release trên `main`; không gắn tag vào bootstrap hoặc commit chưa nghiệm thu.
