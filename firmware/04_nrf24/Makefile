# 04_nrf24 — NRF24L01+ 寄存器级 SPI1 驱动（BSP 重构版）
TARGET := nrf24
BUILD_DIR := build

C_SOURCES := \
    Src/main.c \
    Src/bsp_spi1.c \
    Src/bsp_nrf24.c \
    ../common/system_stm32f1xx.c \
    ../common/bsp_delay.c \
    ../common/bsp_gpio.c \
    ../common/bsp_usart.c

ASM_SOURCES := \
    Startup/startup_stm32f103vctx.s

C_INCLUDES := \
    -ISrc \
    -I../common

C_DEFS := \
    -DSTM32F103xE \
    -DHSE_VALUE=8000000

include ../Makefile.common

size: $(BUILD_DIR)/$(TARGET).elf
	@$(SZ) $<
