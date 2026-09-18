#!/usr/bin/env bash
# 一键烧录 hello_uart
# 跑法：bash firmware/01_hello_uart/flash.sh
set -euo pipefail

cd "$(dirname "$0")"

if [[ ! -f build/hello_uart.bin ]]; then
    echo "❌ 没有 build/hello_uart.bin，先 make"
    exit 1
fi

echo "[1/2] 探测 ST-Link..."
if ! st-info --probe >/dev/null 2>&1; then
    echo "❌ ST-Link 未连接"
    echo "   检查：USB 线、ST-Link 灯、dmesg | tail -20"
    exit 1
fi
st-info --probe
echo ""

echo "[2/2] 烧录..."
st-flash --reset --verify write build/hello_uart.bin 0x08000000
echo ""
echo "✅ 烧录完成。打开串口助手（115200 8N1）应该看到 Hello UART 循环打印"
