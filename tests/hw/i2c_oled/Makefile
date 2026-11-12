# Harness test kit I2C1 + OLED (nhánh feature/hal-i2c-pwm). Chạy từ thư mục gốc repo:
#   make -C tests/hw/i2c_oled            # build
#   make -C tests/hw/i2c_oled flash      # nạp qua ST-Link (st-flash)
ROOT := ../../..
CROSS_COMPILE ?= arm-none-eabi-
CC := $(CROSS_COMPILE)gcc
OBJCOPY := $(CROSS_COMPILE)objcopy
SIZE := $(CROSS_COMPILE)size
RTOS := $(ROOT)/middlewares/FreeRTOS-Kernel
BUILD := $(ROOT)/build/test_i2c_oled
CPPFLAGS := -I$(ROOT)/bsp/inc -I$(ROOT)/hal/inc -I$(ROOT)/app/inc -I$(ROOT)/config -I$(RTOS)/include -I$(RTOS)/portable/GCC/ARM_CM3
CFLAGS := -mcpu=cortex-m3 -mthumb -mfloat-abi=soft -std=c11 -Og -g3 -Wall -Wextra -Werror -ffunction-sections -fdata-sections -ffreestanding -MMD -MP
LDFLAGS := -mcpu=cortex-m3 -mthumb -mfloat-abi=soft -T$(ROOT)/linker/stm32f103c8.ld -nostartfiles --specs=nosys.specs -Wl,--gc-sections,-Map=$(BUILD)/test_i2c_oled.map -Wl,--print-memory-usage
SOURCES := test_main.c \
	$(ROOT)/bsp/src/startup.c $(ROOT)/bsp/src/bsp.c $(ROOT)/app/src/rtos_hooks.c \
	$(ROOT)/hal/src/hal_sys.c $(ROOT)/hal/src/hal_i2c.c  \
	$(ROOT)/hal/src/ssd1306.c $(ROOT)/hal/src/font5x7.c \
	$(RTOS)/tasks.c $(RTOS)/queue.c $(RTOS)/list.c \
	$(RTOS)/portable/GCC/ARM_CM3/port.c $(RTOS)/portable/MemMang/heap_4.c
OBJECTS := $(addprefix $(BUILD)/,$(notdir $(SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(SOURCES)))

.PHONY: all clean flash
all: $(BUILD)/test_i2c_oled.bin
$(BUILD)/test_i2c_oled.elf: $(OBJECTS)
	$(CC) $(LDFLAGS) $^ -Wl,--start-group -lc -lgcc -lnosys -Wl,--end-group -o $@
	$(SIZE) $@
$(BUILD)/test_i2c_oled.bin: $(BUILD)/test_i2c_oled.elf
	$(OBJCOPY) -O binary $< $@
$(BUILD)/%.o: %.c
	@mkdir -p $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
flash: $(BUILD)/test_i2c_oled.bin
	st-flash --reset write $< 0x08000000
clean:
	rm -rf $(BUILD)
-include $(OBJECTS:.o=.d)
