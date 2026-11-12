# Bàn giao develop cho nhóm

## Bắt đầu từ đây

1. Leader và ba thành viên điền tên/GitHub username vào bảng phân công trong PLAN.
2. Đưa kho lên remote trống theo GIT_WORKFLOW; bật bảo vệ main/develop.
3. Leader bring-up BSP/clock/FreeRTOS trên kit, kiểm tra SWD và ngắt.
4. TV1/TV2/TV3 làm trên ba feature HAL đã tạo, mở PR về develop. Không viết chồng lên main.c.
5. Sau khi HAL đạt điều kiện M1, tạo các nhánh task từ develop mới nhất.

## Ai sửa tệp nào trong giai đoạn đầu

| Người | Phạm vi chính | Phải phối hợp/review trước khi đổi |
|---|---|---|
| Leader | bsp/, config/, linker/, main.c, CI/build | Clock, pin, IRQ, RAM và RTOS API với cả nhóm |
| TV1 | hal_spi.*, driver ILI9341 mới | API SPI/CS/DC, orientation và snapshot với TV2/TV3 |
| TV2 | hal_adc.*, hal_gpio.*, input contract | EXTI/debounce, kiểu input/FSM với leader |
| TV3 | hal_i2c.*, hal_pwm.*, driver SSD1306/audio mới | OLED API với TV1, sound queue/HMI với TV2 |
| Cả nhóm qua PR | app_cfg.h, game_types.h, app_ipc.h/.c, tài liệu | Không đổi signature/enum/payload âm thầm |

## Kết quả đã kiểm tra

- ARM GCC 14.2.1; build sạch với `-Wall -Wextra -Werror` thành công.
- FreeRTOS Kernel V11.1.0 thật, port GCC Cortex-M3, heap_4; giữ nguyên nguồn/giấy phép upstream.
- Tạo ELF/BIN/HEX/MAP, vector ARM Cortex-M3, Flash đặt tại 0x08000000.
- Khung hiện dùng khoảng 6,5 KiB Flash. `size` báo data 4 B, bss 11.564 B (gồm 1 KiB vùng stack MSP dự trữ). Chưa có buffer màn hình/entity game thật nên đây không phải mức RAM khi hoàn tất game.
- Build outputs bị ignore, không nằm trong commit. Không có test kit, game engine hoặc driver thực để báo đạt.

## Giới hạn hiện tại

- BSP và mọi driver là stub, trả `HAL_NOT_IMPLEMENTED`; main dừng chủ động trước scheduler. Các task chưa có gameplay.
- Snapshot chỉ chứa thông tin cơ bản; cần chốt API entity/dirty-region trước render.
- Chưa có tài khoản thành viên, branch protection, PR online hoặc tag release.
- API GitHub từ phiên cloud trả 403; chưa tạo/push remote. Có helper publish và bản Git bundle/ZIP để bàn giao.

Không đánh dấu mục tiêu M1/M2/M3 đã đạt chỉ vì build xanh. Xem TEST_PLAN để thực hiện và ghi số đo thực.
