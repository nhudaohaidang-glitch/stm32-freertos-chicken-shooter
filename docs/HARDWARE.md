# Phần cứng

| Ngoại vi | Chân STM32F103C8T6 | Thiết bị / ghi chú |
|---|---|---|
| SPI1 | PA5 SCK, PA7 MOSI, PA4 CS, PA3 DC, PB1 RESET | TFT ILI9341 2,8 inch, 240×320, RGB565 |
| I2C1 | PB6 SCL, PB7 SDA | OLED SSD1306 128×64 |
| ADC1 | PA0 IN0 VRx, PA1 IN1 VRy | Joystick KY-023 |
| GPIO / EXTI | PB12 Fire, PB13 Start/Pause | Chống dội tại HMI |
| TIM3_CH3 | PB0 | Buzzer thụ động |
| USART1 | PA9 TX, PA10 RX | USB–TTL debug tùy chọn |
| SWD | PA13 SWDIO, PA14 SWCLK | Dành riêng cho ST-Link |

- Logic 3,3 V, GND chung. Joystick và OLED cấp 3,3 V. VCC/LED TFT theo nhãn và mạch module thực tế.
- Chưa chốt mức tích cực nút nhấn; API GPIO trả trạng thái logic `pressed` để HMI không phụ thuộc cực tính.
- OLED: xác nhận địa chỉ 7-bit bằng kiểm tra bus; không mặc định mọi module là 0x3C.
- Trước khi tăng SPI: kiểm tra clock, tốc độ cho phép của màn hình thực tế và dây nối. PA6/MISO chưa cần cho đường vẽ write-only trong đề cương.
- EXTIs 12/13 dùng chung `EXTI15_10_IRQHandler`; ISR kiểm tra/xóa đúng pending bit. Nếu gọi RTOS FromISR thì cấu hình NVIC phải phù hợp ngưỡng trong FreeRTOSConfig.h.
- FreeRTOS dùng SysTick/SVC/PendSV; tránh tạo timebase HAL trùng SysTick mà không thiết kế tích hợp.
- Không thay pin/clock hoặc generate lại cấu hình chung trong các nhánh driver cá nhân.

Điền thêm vào PR bring-up: ảnh dây, loại board, module TFT/OLED, nguồn, clock đo được, cực tính nút, địa chỉ OLED và phiên bản firmware được test.
