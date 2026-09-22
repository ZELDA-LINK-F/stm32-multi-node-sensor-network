# 01_hello_uart — 最简单的 USART 测试（重构版）
TARGET := hello_uart
BUILD_DIR := build

C_SOURCES := \
    Src/main.c \
    ../common/hal_delay.c \
    ../common/hal_gpio.c \
    ../common/hal_usart.c \
    ../common/system_stm32f1xx.c

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
	@echo "---- 编译产物大小 ----"
	@$(SZ) $<
