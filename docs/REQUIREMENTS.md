# Nguồn yêu cầu và cách đối chiếu

## Tài liệu đã đọc

- [Đề cương DOCX nguyên bản](references/De_cuong_game_ban_ga_STM32_FreeRTOS.docx).
- [Quy trình Git PDF nguyên bản, 6 trang](references/QuyTrinh-Git-2.pdf).
- Ảnh bảng do người dùng gửi trong cuộc trò chuyện: thể hiện main, develop, các nhánh ngoại vi và task tách ra/ghép về. Không tự suy ra những nhãn khó đọc.

Người dùng yêu cầu tạo kho Git cho đề tài mới theo các tài liệu này. Nội dung tài liệu được dùng làm đặc tả dự án/quy trình; các ví dụ Sensor/Display/Comm trong PDF được ánh xạ sang HMI/GAME/RENDER của đề cương, không biến thành tính năng mới.

## Ánh xạ

| Đề cương / quy trình | Vị trí trong kho |
|---|---|
| BSP / HAL / middleware / APP | `bsp/`, `hal/`, `middlewares/`, `app/` |
| Header API trước khi viết driver | `hal/inc/`, `app/inc/` |
| Ba task, ưu tiên 3 / 3 / 2 | `app_cfg.h`, `task_hmi.*`, `task_game.*`, `task_render.*` |
| GAME định kỳ 20 ms | khung `task_game.c`, kế hoạch engine/FSM trong PLAN |
| Queue và Notification | `app_ipc.h/.c`, ARCHITECTURE |
| Pin map không trùng | `board_pins.h`, HARDWARE |
| TFT buffer 2.560 B / OLED 1.024 B | `app_cfg.h`, ngân sách trong ARCHITECTURE |
| `main`, `develop`, `feature/*`, `integration/*` | lịch sử Git thực, GIT_WORKFLOW |
| Không commit build/IDE | `.gitignore` |
| Chỉ một người cấu hình board/RTOS | GIT_WORKFLOW, PLAN |
| Review, đo heap/stack, test kit trước release | mẫu PR, TEST_PLAN |

## Lựa chọn kỹ thuật khởi tạo, chưa phải yêu cầu đã đo

- Make + ARM GCC, không dùng CubeMX trong khung này. Nếu chuyển IDE/codegen, chỉ leader thực hiện và giữ API.
- FreeRTOS V11.1.0 được vendor từ upstream, giữ giấy phép gốc.
- HSI 8 MHz là giả định bootstrap trong `FreeRTOSConfig.h`; BSP chưa triển khai. Chưa chốt PLL/clock SPI, I2C, PWM.
- Heap 10 KiB; stack HMI/GAME/RENDER 192/256/384 words; input queue 16, sound queue 8. Phải đo và hiệu chỉnh, không phải số nghiệm thu.
- Snapshot queue dài 1 kèm notification để tránh render đọc trực tiếp game state; đây là phương án IPC khởi đầu.
- Task HMI sở hữu buzzer; TV3 làm driver/hiệu ứng, phối hợp TV2 tích hợp vào HMI.
- Nhánh ngoại vi được đổi tên theo phân công trong đề cương thay vì sao chép ba nhánh ví dụ của PDF.

Chưa có thông tin về tên thành viên, ngày nộp, phiên bản module TFT thực tế, hướng màn hình, địa chỉ I2C OLED, clock mục tiêu, ảnh nối dây thực tế. Không tự điền các thông tin này thành yêu cầu chắc chắn. License riêng của phần nhóm viết chưa được chọn; không tự gán license MIT của FreeRTOS cho toàn bộ dự án.
