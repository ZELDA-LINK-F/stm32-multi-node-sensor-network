# 02_dht11 — DHT11 温湿度采集（重构版）
TARGET := dht11
BUILD_DIR := build

# 源文件（应用 + BSP 公共层）
C_SOURCES := \
    Src/main.c \
    Src/dht11.c \
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
	@$(SZ) $<
