# Hello UART 工程 Makefile
# 用法：make            # 编译
#       make flash      # 烧录
#       make clean      # 清理
#       make check-tools

# 1. 跑 CubeMX 生成代码后，把下列路径填好
# 2. CubeMX Project Manager -> Code Generator 勾上
#    "Generate peripheral initialization as a pair of .c/.h files per peripheral"

TARGET := hello_uart
BUILD_DIR := build

# === CubeMX 输出源文件 ===
C_SOURCES := \
    Src/main.c \
    Src/stm32f1xx_it.c \
    Src/usart.c \
    Src/gpio.c \
    Src/stm32f1xx_hal_msp.c \
    Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_uart.c \
    Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal.c \
    Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_rcc.c \
    Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_gpio.c \
    Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_cortex.c \
    Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_pwr.c \
    Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_dma.c \
    Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_flash.c

ASM_SOURCES := \
    Startup/startup_stm32f103vctx.s

C_INCLUDES := \
    -ISrc \
    -IInc

C_DEFS := \
    -DSTM32F103xE \
    -DUSE_HAL_DRIVER \
    -DHSE_VALUE=8000000

include ../Makefile.common
