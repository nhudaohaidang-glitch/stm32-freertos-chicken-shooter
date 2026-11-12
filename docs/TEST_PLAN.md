# Kế hoạch kiểm tra và nghiệm thu

## Phân biệt kết quả

**Đã chạy không cần kit** (xem số liệu trong `docs/EVALUATION.md`):

| Lệnh | Kiểm tra |
|---|---|
| `make clean && make -j4 check` | Build firmware tích hợp với `-Wall -Wextra -Werror`, không warning |
| `make -C tests/hw/<adc_gpio\|i2c_oled\|spi_tft>` | Build ba harness test kit |
| `make -C tests/host test` | Unit test FSM: đủ 44 cặp (trạng thái, sự kiện), pause, va chạm, HP, đợt, 200.000 khung ngẫu nhiên; ASan/UBSan |

**Chưa chạy trong phiên phát triển:** mọi mục cần kit bên dưới. Build/test PC xanh không chứng minh nối dây, timing ngắt, tốc độ SPI thực hay FPS thật.

## M1 — driver trên kit (chưa chạy)

| Hạng mục | Kiểm tra cần thực hiện |
|---|---|
| BSP/FreeRTOS | Clock/tick đúng, context switch, SWD, hook assert/malloc/stack |
| SPI/TFT | Test pattern, RGB565, clipping, vùng vẽ, CS/DC đúng, timeout |
| ADC | Hai trục đủ dải, trung tâm/dead-zone, không vượt 3,3 V |
| GPIO/EXTI | Fire/Pause đúng cực tính, một lần nhấn không lặp do dội; ISR RTOS priority đúng |
| I2C/OLED | Địa chỉ thật, cập nhật điểm/HP/state, NACK/timeout không treo task |

## M2 — task/FSM (chưa chạy)

- HMI phát đúng MOVE/SHOOT/START_PAUSE; kiểm tra queue đầy, drop counter và nhấn nhanh.
- GAME giữ nhịp 20 ms dù không có input; chuyển state hợp lệ, bỏ event sai state, va chạm/điểm/HP, restart reset đúng.
- PAUSED giữ nguyên tiến trình game; resume không nhảy vị trí do khoảng pause.
- RENDER chỉ đọc bản sao; coalesced notification không mất vùng cần xóa của frame cuối; không vệt/sót vùng khi khung bị gộp.
- Kiểm tra task_create thất bại và IPC cấp phát thiếu: fail rõ ràng, không khởi động dang dở.

## M3 — demo tích hợp (chưa chạy)

1. Bật nguồn -> MENU; Start -> PLAYING.
2. Joystick di chuyển, Fire sinh đạn; gà di chuyển và va chạm đổi điểm/HP.
3. Pause/Resume; HP về 0 -> GAME_OVER; Restart tạo ván mới.
4. OLED phản ánh đúng state/điểm/HP; dòng FPS/HEAP/STK cập nhật mỗi 1 s.
5. Đo FPS trên màn hình thật, heap hiện tại/tối thiểu, stack high-water mark mỗi task, RAM từ map. Đo thời gian GAME có trễ chu kỳ hay không.
6. Thử input dồn dập, pause nhiều lần, nhiều đạn/gà và chạy lâu; ghi thời lượng thực tế, lỗi/drop/malloc/stack hook.

Không đặt mục tiêu FPS hoặc thời lượng soak giả khi nhóm chưa thống nhất. Hãy ghi số đo thay vì đánh dấu đạt chung chung.

## Mẫu báo cáo test

| Ngày | Commit / nhánh | Board/module/clock | Tình huống | Kỳ vọng | Thực tế / số đo | PASS/FAIL/CHƯA CHẠY | Người test |
|---|---|---|---|---|---|---|---|
| Điền lúc test | SHA thật | Phần cứng thật | ... | ... | Đính kèm log/ảnh | ... | ... |

Chỉ release main sau khi các kiểm tra bắt buộc có bằng chứng và review. Việc code compile được không chứng minh wiring, timing, stack hoặc game hoạt động.
