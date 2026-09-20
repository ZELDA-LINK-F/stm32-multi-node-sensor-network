#!/bin/bash
# stm32.sh — STM32 工程通用工具（编译/烧录/看串口/一键三连）
#
# 用法：
#   ./stm32.sh build 01_hello_uart      ← 编译
#   ./stm32.sh flash 03_bno055          ← 烧录
#   ./stm32.sh serial 04_nrf24          ← 看串口
#   ./stm32.sh all 01_hello_uart        ← 编译+烧录+看串口
#   ./stm32.sh all                      ← 不传参数 = 当前目录
#
# 工程列表：
#   01_hello_uart   - 串口 Hello World（最小，验证时钟）
#   02_dht11        - DHT11 单总线
#   03_bno055       - BNO055 I2C 九轴姿态
#   04_nrf24        - NRF24L01+ SPI 2.4GHz
#   05_freertos     - FreeRTOS 4 任务

set -e

# === 颜色 ===
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; NC='\033[0m'
ok()   { echo -e "${GREEN}✅ $*${NC}"; }
warn() { echo -e "${YELLOW}⚠️  $*${NC}"; }
err()  { echo -e "${RED}❌ $*${NC}"; exit 1; }

# === 找工程根（处理软链情况）===
if [ -L "$0" ]; then
    # 软链：解析真实路径
    REAL_PATH="$(readlink -f "$0")"
    ROOT="$(cd "$(dirname "$REAL_PATH")/.." && pwd)"
else
    ROOT="$(cd "$(dirname "$0")/.." && pwd)"
fi
FW="$ROOT/firmware"

# === 找工程目录 ===
find_project() {
    local proj="$1"
    if [ -z "$proj" ]; then
        # 没传参数：找当前目录是不是工程
        if [ -f "Makefile" ] && [ -d "Src" ]; then
            echo "$(pwd)"
            return 0
        else
            err "用法: $0 <build|flash|serial|all> [工程名]"
        fi
    fi
    local dir="$FW/$proj"
    if [ ! -d "$dir" ]; then
        err "工程不存在: $dir\n可用: 01_hello_uart  02_dht11  03_bno055  04_nrf24  05_freertos"
    fi
    echo "$dir"
}

# === 动作：编译 ===
do_build() {
    local dir="$1"
    cd "$dir"
    ok "编译: $dir"
    make clean > /dev/null
    make 2>&1 | tail -3
    local size=$(arm-none-eabi-size build/*.elf 2>/dev/null | tail -1 | awk '{print $1}')
    ok "编译完成 (text=$size 字节)"
}

# === 动作：烧录 ===
do_flash() {
    local dir="$1"
    cd "$dir"
    if [ ! -f "flash.sh" ]; then
        err "找不到 flash.sh（在 $dir）"
    fi
    ok "烧录: $dir"
    ./flash.sh 2>&1 | grep -E "device id|flash size|Verified|Resetting" || true
    ok "烧录完成"
}

# === 动作：看串口 ===
do_serial() {
    local port="${SERIAL_PORT:-/dev/ttyUSB0}"
    local baud="${SERIAL_BAUD:-115200}"
    ok "读串口 $port @ $baud (3 秒)"
    # 烧录后等 3 秒让 USART buffer 准备好
    sleep 3
    timeout 3 cat "$port" 2>&1 || true
}

# === 主入口 ===
ACTION="${1:-help}"
PROJ="${2:-}"

case "$ACTION" in
    build)
        DIR=$(find_project "$PROJ"); do_build "$DIR" ;;
    flash)
        DIR=$(find_project "$PROJ"); do_flash "$DIR" ;;
    serial)
        do_serial ;;
    all)
        DIR=$(find_project "$PROJ")
        do_build "$DIR"
        do_flash "$DIR"
        echo ""
        do_serial
        ;;
    list)
        echo "可用工程："
        for d in "$FW"/*/; do
            [ -f "$d/Makefile" ] && echo "  $(basename "$d")"
        done
        ;;
    help|*)
        echo "用法：$0 <动作> [工程名]"
        echo ""
        echo "动作："
        echo "  build   编译"
        echo "  flash   烧录"
        echo "  serial  看串口"
        echo "  all     编译+烧录+看串口"
        echo "  list    列出所有工程"
        echo ""
        echo "例子："
        echo "  $0 list"
        echo "  $0 build 01_hello_uart"
        echo "  $0 flash 03_bno055"
        echo "  $0 all 01_hello_uart"
        ;;
esac
