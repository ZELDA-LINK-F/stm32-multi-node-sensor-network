#!/usr/bin/env bash
# STM32CubeMX 下载安装脚本（Linux）
# 官方下载页：https://www.st.com/en/development-tools/stm32cubemx.html
# 安装包：en.stm32cubemx-lin-vxxx.zip （约 1.7GB）

set -euo pipefail

CUBEMX_VERSION="${CUBEMX_VERSION:-6.12.0}"
INSTALL_DIR="$HOME/STM32CubeMX"
TMP_DIR="$(mktemp -d)"

echo "==============================================="
echo "  STM32CubeMX ${CUBEMX_VERSION} 下载安装"
echo "==============================================="
echo ""
echo "⚠️  你需要先去 ST 官网下载（需登录 + 接受协议）："
echo "    https://www.st.com/en/development-tools/stm32cubemx.html"
echo "    找到 'Get Software'，下 Linux 版（en.stm32cubemx-lin-v${CUBEMX_VERSION}.zip）"
echo ""

read -p "下载完成了吗？把 zip 拖到终端或输入绝对路径: " ZIP_PATH

if [[ ! -f "$ZIP_PATH" ]]; then
    echo "❌ 文件不存在：$ZIP_PATH"
    exit 1
fi

echo ""
echo "[1/3] 解压到 ${INSTALL_DIR}..."
mkdir -p "$INSTALL_DIR"
unzip -q "$ZIP_PATH" -d "$TMP_DIR"
# CubeMX zip 解压后是个 SetupSTM32CubeMX-xxx_linux 文件
SETUP_BIN=$(find "$TMP_DIR" -maxdepth 2 -name "SetupSTM32CubeMX*linux*" | head -1)
chmod +x "$SETUP_BIN"

echo ""
echo "[2/3] 启动安装程序（图形界面）..."
echo "    图形界面会问：安装路径、要不要下载 HAL 库（建议下，选 STM32F1）"
"$SETUP_BIN"

echo ""
echo "[3/3] 添加到 PATH..."
cat >> ~/.bashrc << 'PATH_EOF'

# STM32CubeMX
export PATH="$HOME/STM32CubeMX/bin:$PATH"
PATH_EOF

if [[ -n "${ZSH_VERSION:-}" ]]; then
    cat >> ~/.zshrc << 'PATH_EOF'

# STM32CubeMX
export PATH="$HOME/STM32CubeMX/bin:$PATH"
PATH_EOF
fi

echo ""
echo "✅ 完成。下次终端打开后命令："
echo "    STM32CubeMX"
echo ""
echo "（或直接：${INSTALL_DIR}/bin/STM32CubeMX）"
