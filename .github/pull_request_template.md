## Mục tiêu / nhánh

- Nhánh nguồn: `feature/...` hoặc `integration/...`
- Nhánh đích: `develop` (release `develop -> main`)
- Thành viên phụ trách / người review:

## Thay đổi

Mô tả API, ngoại vi hoặc task đã làm; liên kết đầu việc.

## Bằng chứng kiểm tra

- [ ] Build sạch `make clean && make -j2 check`, không warning.
- [ ] Tuân thủ API HAL/IPC, timeout hữu hạn, không deadlock.
- [ ] ISR chỉ dùng `...FromISR`, không delay thô trong task.
- [ ] Feature-task không sửa `main.c`, BSP, clock, cấu hình RTOS chung.
- [ ] Có người khác review.
- [ ] Không có file build hoặc cấu hình IDE cá nhân.
- [ ] Test trên kit: ghi phần cứng, cách test, kết quả và log (hoặc ghi rõ CHƯA TEST).

Với integration/release, bổ sung FPS, heap tối thiểu, stack high-water mark, kết quả FSM và sơ đồ nối dây thực tế. Không đánh dấu đã test nếu chỉ biên dịch.
