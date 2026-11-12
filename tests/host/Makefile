# Unit test FSM / game engine trên PC (gcc của máy, không cần kit/FreeRTOS).
# Minh chứng cho chủ đề "State machine, event-driven firmware". Chạy từ thư mục gốc repo:
#   make -C tests/host test
ROOT := ../..
HOSTCC ?= gcc
OUT := $(ROOT)/build/host
# Windows/MinGW không có sanitizer: make -C tests/host test SAN=
SAN ?= -fsanitize=address,undefined -fno-omit-frame-pointer
CFLAGS := -std=c11 -O1 -g -Wall -Wextra -Werror -I$(ROOT)/app/inc $(SAN)

.PHONY: all test clean
all: $(OUT)/test_engine

$(OUT)/test_engine: test_engine.c $(ROOT)/app/src/game_engine.c
	@mkdir -p $(OUT)
	$(HOSTCC) $(CFLAGS) $^ -o $@

test: all
	$(OUT)/test_engine

clean:
	rm -rf $(OUT)
