# 02_dht11 — DHT11 寄存器级 Makefile
TARGET := dht11
BUILD_DIR := build

C_SOURCES := \
    Src/main.c \
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
	@$(SZ) $<
