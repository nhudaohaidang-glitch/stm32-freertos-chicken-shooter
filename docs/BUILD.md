# Build và công cụ

## Linux / môi trường đám mây hiện tại

```sh
export PATH=/workspace/toolchains/arm/usr/bin:$PATH
cd /workspace/stm32-freertos-chicken-shooter
arm-none-eabi-gcc --version
make -j2 check
```

Đã dùng ARM GCC 14.2.1 + Binutils 2.44 + Newlib 4.5 trong phiên này. Không cần các bài tập của kho cũ, không cần submodule và không cần tải dependency để build khi đã có toolchain.

Máy Ubuntu/Debian của thành viên có quyền cài gói:

```sh
sudo apt-get update
sudo apt-get install -y --no-install-recommends make gcc-arm-none-eabi binutils-arm-none-eabi libnewlib-arm-none-eabi libnewlib-dev
make -j2 check
```

Nếu toolchain ở vị trí khác, thêm bin vào PATH hoặc dùng `make CROSS_COMPILE=/duong-dan/bin/arm-none-eabi- check`.

Kết quả: `build/firmware.elf`, `firmware.bin`, `firmware.hex`, `firmware.map`. `make clean` chỉ xóa `build/` của kho này. Không commit output. RAM limit là 20 KiB, Flash 64 KiB; linker assert chừa 1 KiB cho MSP/ISR. Khi đọc memory usage hãy lưu ý vùng stack cố định ở đỉnh RAM (giải thích trong ARCHITECTURE).

## Kiểm tra lại từ đầu

```sh
make clean
make -j2 check
git status --short
```

Tệp build phải nằm trong ignore. So sánh vendor FreeRTOS với commit ghi ở middlewares/README.md khi cập nhật. Giữ xác minh TLS, chữ ký gói và license upstream.

## Windows

Có thể build bằng WSL Ubuntu và cùng các lệnh Make ở trên. Chưa cung cấp project STM32CubeIDE/CubeMX; không tự cho rằng có `.ioc`. Nếu nhóm dùng CubeIDE, leader tạo cấu hình chung và giữ đúng pin map/API/RTOS config, review riêng phần codegen.

## Nạp phần cứng

Chỉ làm sau khi M1/BSP và driver đã được hoàn thiện, build được review và kết nối đúng kit. Khung hiện dừng chủ động tại BSP, chưa có game để nạp demo. Sau này có thể dùng STM32CubeProgrammer hoặc st-flash/ST-Link; công cụ nạp không phải điều kiện để build trong cloud. Chưa chạy lệnh nạp hay kiểm tra phần cứng trong phiên này.
