#!/usr/bin/env bash
# STM32 + Linux 工具链一键安装脚本
# 适用：Ubuntu 24.04 / Debian 12
# 跑法：bash tools/install_toolchain.sh

set -euo pipefail

echo "==============================================="
echo "  STM32 + Linux 工具链安装（Ubuntu 24.04）"
echo "==============================================="

# 1. 编译器 + 标准库 + 调试器 + 烧录 + OpenOCD + Java
echo ""
echo "[1/3] 安装 apt 包..."
sudo apt update
sudo apt install -y \
    gcc-arm-none-eabi \
    libnewlib-arm-none-eabi \
    gdb-multiarch \
    openocd \
    stlink-tools \
    make \
    cmake \
    ninja-build \
    default-jre \
    curl \
    unzip

# 2. 验证
echo ""
echo "[2/3] 验证安装..."
TOOLS=(arm-none-eabi-gcc arm-none-eabi-gdb arm-none-eabi-objcopy \
       arm-none-eabi-size openocd st-flash st-info make cmake java)
for t in "${TOOLS[@]}"; do
    if command -v "$t" >/dev/null 2>&1; then
        VER=$("$t" --version 2>&1 | head -1 | cut -c1-60)
        printf "  ✅ %-26s %s\n" "$t" "$VER"
    else
        printf "  ❌ %-26s MISSING\n" "$t"
    fi
done

# 3. 检查 OpenOCD 接口/目标支持
echo ""
echo "[3/3] OpenOCD 接口与目标："
openocd --list-interfaces 2>/dev/null | grep -i stlink && echo "  ✅ stlink 接口 OK" || echo "  ⚠️  stlink 接口未找到"
openocd --list-targets 2>/dev/null | grep -i stm32f1x && echo "  ✅ stm32f1x 目标 OK" || echo "  ⚠️  stm32f1x 目标未找到"

echo ""
echo "==============================================="
echo "  下一步：插上 ST-Link + 板子，跑："
echo "  st-info --probe    # 应输出 Found 1 stlink"
echo "  openocd --version  # 应 ≥ 0.12"
echo "==============================================="
