# 05_freertos — FreeRTOS 4 任务工程
TARGET := freertos
BUILD_DIR := build

# === FreeRTOS 路径 ===
FREERTOS_KERNEL := third_party/FreeRTOS-Kernel
FREERTOS_INC := \
    -I$(FREERTOS_KERNEL)/include \
    -I$(FREERTOS_KERNEL)/portable/GCC/ARM_CM3

# === FreeRTOS 源文件（最小集）===
C_SOURCES := \
    Src/main.c \
    $(FREERTOS_KERNEL)/tasks.c \
    $(FREERTOS_KERNEL)/queue.c \
    $(FREERTOS_KERNEL)/list.c \
    $(FREERTOS_KERNEL)/portable/GCC/ARM_CM3/port.c \
    $(FREERTOS_KERNEL)/portable/MemMang/heap_4.c \
    ../common/system_stm32f1xx.c

ASM_SOURCES := \
    Startup/startup_stm32f103vctx.s

C_INCLUDES := \
    -ISrc \
    -I../common \
    $(FREERTOS_INC)

C_DEFS := \
    -DSTM32F103xE \
    -DHSE_VALUE=8000000 \
    -D__NVIC_PRIO_BITS=4 \
    -DconfigKERNEL_INTERRUPT_PRIORITY=15 \
    -DconfigMAX_SYSCALL_INTERRUPT_PRIORITY=5

include ../Makefile.common

size: $(BUILD_DIR)/$(TARGET).elf
	@$(SZ) $<
