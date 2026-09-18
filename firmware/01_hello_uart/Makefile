# 01_hello_uart — 寄存器级 Makefile
# 用法：
#   make            编译
#   make size       查看编译产物大小
#   make flash      烧录（需 ST-Link）
#   make clean      清理

TARGET := hello_uart
BUILD_DIR := build

# === 源文件 ===
C_SOURCES := \
    Src/main.c

ASM_SOURCES := \
    Startup/startup_stm32f103vctx.s

C_INCLUDES := \
    -ISrc

C_DEFS := \
    -DSTM32F103xE \
    -DHSE_VALUE=8000000

include ../Makefile.common

# === 额外目标 ===
size: $(BUILD_DIR)/$(TARGET).elf
	@echo "---- 编译产物大小 ----"
	@$(SZ) $<
