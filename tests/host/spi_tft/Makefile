ROOT := ../../..
HOSTCC ?= gcc
OUT := $(ROOT)/build/host/spi_tft
SAN ?= -fsanitize=undefined -fno-omit-frame-pointer
CFLAGS := -std=c11 -O1 -g -Wall -Wextra -Werror -Istubs -I$(ROOT)/hal/inc $(SAN)
SOURCES := test_spi_tft.c $(ROOT)/hal/src/hal_spi.c $(ROOT)/hal/src/ili9341.c
HEADERS := $(wildcard stubs/*.h $(ROOT)/hal/inc/*.h)
.PHONY: all test clean
all: $(OUT)/test_spi_tft
$(OUT)/test_spi_tft: $(SOURCES) $(HEADERS) Makefile
	@mkdir -p $(OUT)
	$(HOSTCC) $(CFLAGS) $(SOURCES) -o $@
test: all
	$(OUT)/test_spi_tft
clean:
	rm -rf $(OUT)
