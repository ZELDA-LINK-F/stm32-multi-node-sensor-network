#!/bin/bash
# flash.sh — 用 OpenOCD + CMSIS-DAP 烧录 VET6 指南者
# 用法：./flash.sh [build/xxx.elf]

ELF="${1:-build/hello_uart.elf}"

if [ ! -f "$ELF" ]; then
    echo "❌ 找不到 $ELF"
    exit 1
fi

echo "=== 烧录: $ELF ==="
openocd -f interface/cmsis-dap.cfg -f target/stm32f1x.cfg \
    -c "program $ELF verify reset exit" 2>&1 | tail -10
