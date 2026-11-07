CROSS_COMPILE ?= arm-none-eabi-
CC := $(CROSS_COMPILE)gcc
OBJCOPY := $(CROSS_COMPILE)objcopy
SIZE := $(CROSS_COMPILE)size
RTOS := middlewares/FreeRTOS-Kernel
CPPFLAGS := -Ibsp/inc -Ihal/inc -Iapp/inc -Iconfig -I$(RTOS)/include -I$(RTOS)/portable/GCC/ARM_CM3
CFLAGS := -mcpu=cortex-m3 -mthumb -mfloat-abi=soft -std=c11 -Og -g3 -Wall -Wextra -Werror -ffunction-sections -fdata-sections -ffreestanding -MMD -MP
LDFLAGS := -mcpu=cortex-m3 -mthumb -mfloat-abi=soft -Tlinker/stm32f103c8.ld -nostartfiles --specs=nosys.specs -Wl,--gc-sections,-Map=build/firmware.map -Wl,--print-memory-usage
SOURCES := main.c $(wildcard bsp/src/*.c hal/src/*.c app/src/*.c) $(RTOS)/tasks.c $(RTOS)/queue.c $(RTOS)/list.c $(RTOS)/portable/GCC/ARM_CM3/port.c $(RTOS)/portable/MemMang/heap_4.c
OBJECTS := $(addprefix build/,$(SOURCES:.c=.o))
.PHONY: all clean size check
all: build/firmware.elf build/firmware.bin build/firmware.hex
build/firmware.elf: $(OBJECTS) linker/stm32f103c8.ld
	$(CC) $(LDFLAGS) $(OBJECTS) -Wl,--start-group -lc -lgcc -lnosys -Wl,--end-group -o $@
build/firmware.bin: build/firmware.elf
	$(OBJCOPY) -O binary $< $@
build/firmware.hex: build/firmware.elf
	$(OBJCOPY) -O ihex $< $@
build/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
size: build/firmware.elf
	$(SIZE) $<
check: all size
clean:
	rm -rf build
-include $(OBJECTS:.o=.d)
