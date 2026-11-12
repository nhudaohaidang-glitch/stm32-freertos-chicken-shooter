# SPI/TFT control-flow regression tests on Linux/WSL

Run `make -C tests/host/spi_tft test` from the repo root. Requires host GCC; UBSan is enabled by default (`SAN=` disables it). This does not compile for or access an STM32.

The tests compile the actual hal_spi.c and ili9341.c with fake FreeRTOS/clock/deadline services. mmap reserves a private process region at the MMIO addresses; it does not access physical hardware. The model does not implement peripheral timing, shift-register behavior or TFT command parsing. It checks argument/timeout/lifecycle behavior, not electrical correctness.

Covered: 8/72 MHz SPI divider selection and preserved GPIO fields; zero-timeout calls without partial transmission; finite mutex waits; TXE/BSY stuck; an elapsed deadline with TXE always ready; ILI9341 init/begin/push/end; lock release after errors and invalid windows.

Hardware reset/clock, real elapsed timeout, full-frame timing, wiring, colors, signal edges and peripheral recovery still require the on-kit harness in tests/hw/spi_tft. No kit PASS is implied by a host PASS.
