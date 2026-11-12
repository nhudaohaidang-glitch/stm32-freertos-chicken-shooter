# Tham gia dự án

Đọc [quy trình Git](docs/GIT_WORKFLOW.md), [phân công](docs/PLAN.md) và [bàn giao develop](docs/HANDOFF.md) trước khi nhận việc.

- Làm trên feature branch tạo từ develop, PR về develop, có người khác review.
- API HAL/IPC và bảng chân là hợp đồng chung; đổi qua PR có phối hợp.
- Chỉ leader đổi pin/clock/startup/FreeRTOSConfig.h; nhánh task không sửa main.c.
- Build `make -j2 check`, test phần cứng theo TEST_PLAN và ghi rõ phần chưa test.
- Không commit output build, secret, cấu hình IDE cá nhân; không direct commit main sau bootstrap.
- Không dùng HAL_Delay/delay_ms trong task; ISR chỉ gọi API RTOS FromISR.
- Release/tag chỉ sau tích hợp và kiểm tra kit, không tạo lịch sử đóng góp giả.
