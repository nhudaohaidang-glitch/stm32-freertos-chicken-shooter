# Phân công và kế hoạch làm việc

Chưa có tên thật; nhóm điền TV1/TV2/TV3 và chọn một người làm leader. Không tự giả định người đang dùng tài khoản GitHub là leader.

| Vai trò | Ngoại vi theo đề cương | Giai đoạn HAL | Giai đoạn APP |
|---|---|---|---|
| TV1 | SPI | `feature/hal-spi-tft`: SPI1, ILI9341, CS/DC, vùng vẽ | `feature/task-render`: TFT + OLED, dirty region, FPS |
| TV2 | ADC + GPIO/EXTI | `feature/hal-adc-gpio`: joystick, nút, chống dội contract | `feature/task-hmi` rồi `feature/task-game`: input, engine, va chạm, FSM |
| TV3 | I2C + PWM | `feature/hal-i2c-pwm`: SSD1306, buzzer; UART debug nếu được giao | `feature/ipc-audio-tests`: hoàn thiện IPC/audio, OLED API và kiểm thử phối hợp TV1/TV2 |
| Leader | Cấu hình chung | BSP/clock/startup/FreeRTOSConfig.h, thống nhất API | Review/merge, quản lý main.c, integration và release |

HMI chứa output âm thanh: TV3 cung cấp driver/hiệu ứng; TV2 tích hợp qua API. TV3 hỗ trợ OLED trong RENDER của TV1, tránh hai người cùng sửa task_render.c không thống nhất. Đây là cách triển khai phân công trong đề cương, không thêm task thứ tư.

## M0 — khởi tạo (đã chuẩn bị cục bộ)

- [x] Cấu trúc BSP/HAL/FreeRTOS/APP, Makefile, linker, pin map.
- [x] Git ignore, khung API HAL/IPC/task, cấu hình ưu tiên 3/3/2.
- [x] Vendor FreeRTOS, build được .elf/.bin/.hex/.map, mẫu CI/PR.
- [x] main bootstrap, develop và ba nhánh HAL từ develop.
- [ ] Kho GitHub remote và push.
- [ ] Thêm thành viên, leader và quyền review.
- [ ] Áp dụng branch protection trên GitHub.

Build được khung không có nghĩa M1 hoàn thành. CI được chuẩn bị nhưng chưa chạy trên GitHub khi chưa có remote.

## M1 — bring-up và HAL

- Leader: validate HSI/PLL, timebase, SWD, vector/ISR priority, RAM budget. Thay BSP stub bằng cấu hình đã test.
- TV1: TFT init, pixel/rect/text hoặc sprite cơ bản, clipping, test pattern và timeout.
- TV2: joystick 2 trục, calibration/dead-zone, nút/EXTI, chống dội.
- TV3: OLED đúng địa chỉ, chữ số/trạng thái, PWM đúng tần số, sound API không block.
- Mỗi feature có test riêng trên kit, PR -> develop, người khác review; leader merge lần lượt.

Điều kiện sang M2: HAL đã merge, API thống nhất, RTOS đã chạy trên kit, clock và ngắt được xác nhận; không chỉ dựa vào build xanh.

## M2 — task độc lập

Tạo các nhánh task **mới từ develop cập nhật tại thời điểm đó**, không tạo sớm từ khung M0. Triển khai ba task, engine/FSM, snapshot và âm thanh; dùng harness/mock data độc lập. Mỗi task file cung cấp đúng `task_xxx_create()`, không sửa main.c. TV2 có thể làm HMI rồi GAME; nhóm có thể điều chỉnh người làm GAME nhưng giữ quyền sở hữu file rõ ràng.

PR yêu cầu build không warning, task tạo thành công, stack/heap không lỗi, bằng chứng kiểm tra trên kit. Các mock/harness không tự trở thành logic game thật.

## M3 — tích hợp

Sau khi ba task đã merge -> tạo `integration/system-v1` từ develop. Đại diện tích hợp hoàn thiện app_ipc.c và main.c; init HAL/driver -> IPC -> task -> scheduler. Test toàn hệ thống theo TEST_PLAN; ghi commit tương ứng với log đo.

Sửa lỗi tích hợp trên integration branch; review/merge integration -> develop. Khi kit đạt tiêu chí mới PR develop -> main và tạo tag v1.0.0. Không phát hành từ skeleton, không tạo giả PR/test/tag để có sơ đồ đẹp.

Nhóm tự điền ngày thực tế và buổi demo; đề cương không cung cấp deadline nên không gán lịch tưởng tượng.
