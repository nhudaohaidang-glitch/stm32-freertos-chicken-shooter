# FreeRTOS Kernel

Vendored từ https://github.com/FreeRTOS/FreeRTOS-Kernel, tag **V11.1.0**, commit `dbf70559b27d39c1fdb68dfb9a32140b6a6777a0`.

Chỉ giữ các tệp cần cho dynamic tasks, queue, notification, mutex: `tasks.c`, `queue.c`, `list.c`, headers, port GCC ARM_CM3, `heap_4.c` và giấy phép MIT gốc. Không sửa nguồn upstream. Không bật software timers khi chưa bổ sung `timers.c` và ngân sách RAM. Không phụ thuộc submodule hoặc thư mục bài tập cũ.
