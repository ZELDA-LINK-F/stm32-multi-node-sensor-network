# 06_freertos_demo — FreeRTOS 5 任务 Demo（野火指南者 STM32F103VET6）
TARGET := freertos_demo
BUILD_DIR := build

# === FreeRTOS 内核源码（复用 05_freertos 的，不重复 clone）===
FREERTOS_KERNEL := ../05_freertos/third_party/FreeRTOS-Kernel
FREERTOS_INC := \
    -ISrc \
    -I$(FREERTOS_KERNEL)/include \
    -I$(FREERTOS_KERNEL)/portable/GCC/ARM_CM3

# === 应用源文件（按职责分层）===
C_SOURCES := \
    Src/main.c \
    Src/bsp_usart.c \
    Src/bsp_led.c \
    Src/app_tasks.c \
    Src/freertos_hooks.c \
    $(FREERTOS_KERNEL)/tasks.c \
    $(FREERTOS_KERNEL)/queue.c \
    $(FREERTOS_KERNEL)/list.c \
    $(FREERTOS_KERNEL)/portable/GCC/ARM_CM3/port.c \
    $(FREERTOS_KERNEL)/portable/MemMang/heap_4.c \
    ../common/system_stm32f1xx.c

ASM_SOURCES := \
    ../05_freertos/Startup/startup_stm32f103vctx.s

C_INCLUDES := \
    -ISrc \
    -I../common \
    $(FREERTOS_INC)

# 中断优先级宏由 FreeRTOSConfig.h 提供，不再在命令行 -D
C_DEFS := \
    -DSTM32F103xE \
    -DHSE_VALUE=8000000 \
    -D__NVIC_PRIO_BITS=4

# === 继承共用编译规则 ===
include ../Makefile.common

# === 看代码体积 ===
size: $(BUILD_DIR)/$(TARGET).elf
	@$(SZ) $<

# === OpenOCD 烧录 ===
.PHONY: flash-openocd
flash-openocd: $(BUILD_DIR)/$(TARGET).elf
	openocd -f interface/stlink-v2.cfg -f target/stm32f1x.cfg \
	        -c "program $< verify reset exit"
