# 03_bno055 — BNO055 寄存器级 Makefile（BSP 重构版）
TARGET := bno055
BUILD_DIR := build

C_SOURCES := \
    Src/main.c \
    Src/bsp_i2c1.c \
    Src/bsp_bno055.c \
    ../common/system_stm32f1xx.c \
    ../common/hal_delay.c \
    ../common/hal_gpio.c \
    ../common/hal_usart.c

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
