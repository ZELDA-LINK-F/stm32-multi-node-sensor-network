# 01_hello_uart — 寄存器级 USART1 Hello

> **目标**：从 RM0008 第 25 章 USART 一路看过来，配寄存器实现 115200 8N1 串口打印。
>
> **风格**：纯寄存器操作，**不用 HAL 库，不用 CubeMX**。

## 简历怎么写这一段

> ✅ "基于 STM32F103VET6 实现裸机寄存器级 USART1 串口通信（参考 RM0008），不依赖 HAL 库，编译产物 < 5KB"

## 前置阅读（动手前必读）

1. **RM0008** — STM32F10x Reference Manual
   - 第 7 章 RCC（时钟）：7.3 RCC 寄存器描述，重点 APB2 外设时钟使能
   - 第 25 章 USART：25.3 功能描述 + 25.3.4 波特率计算 + 25.6 寄存器描述
   - **PDF 下载**：https://www.st.com/resource/en/reference_manual/cd00171190-stm32f101xx-stm32f102xx-stm32f103xx-stm32f105xx-and-stm32f107xx-advanced-arm-based-32-bit-mcus-stmicroelectronics.pdf
2. **本仓库** `docs/RM0008_NOTES.md` — 关键章节速查笔记

## 当前状态

| 项 | 状态 |
|---|---|
| 时钟 | ⚠️ **默认 HSI 8MHz**（不是 72MHz！B.1 简化版） |
| USART1 BRR | 按 **72MHz** 算的，实际跑 8MHz → **波特率会偏** |
| 串口 | 115200 8N1，引脚 PA9/PA10 |
| 编译 | ✅ 已通过 Makefile（语法、链接） |
| 烧录 | ⬜ 等硬件 |

> **B.1 故意简化**：时钟配 72MHz 要写 HSE + PLL 那套代码（约 50 行），留到 B.3 FreeRTOS 移植时一起做（因为 FreeRTOS 的 SysTick 也要 72MHz 才有 1ms tick）。  
> 当前 B.1 跑通标准：能看到串口打印但波特率偏低，**接受作为阶段产物**，进 B.3 再修。

## 文件结构

```
01_hello_uart/
├── Makefile                 # 编译入口
├── Src/
│   └── main.c               # 主程序（寄存器级 USART 初始化 + 打印）
├── Startup/
│   └── startup_stm32f103vctx.s  # 启动文件（中断向量 + Reset_Handler）
├── STM32F103VETX_FLASH.ld   # 链接脚本（512KB Flash + 64KB RAM）
├── flash.sh                 # st-flash 一键烧录
└── README.md                # 本文件
```

## 编译步骤

```bash
cd firmware/01_hello_uart
make
# 期望输出：
#   build/hello_uart.elf
#   build/hello_uart.bin
#   build/hello_uart.hex
#   text    data     bss     dec     hex filename
#   1234       0     108    1342     53e build/hello_uart.elf
```

**验证产物大小**：

```bash
make size
# 期望：text < 5KB（寄存器级优势体现）
```

## 烧录步骤（等硬件）

1. 插上 **ST-Link V2** 到 USB
2. 用杜邦线连：
   - ST-Link SWDIO → VET6 PA13
   - ST-Link SWCLK → VET6 PA14
   - ST-Link GND   → VET6 GND
   - ST-Link 3.3V  → VET6 3.3V（**可选**，板子通常已自供电）
3. 串口线：
   - USB-TTL TX → VET6 PA10 (USART1_RX)
   - USB-TTL RX → VET6 PA9 (USART1_TX)
   - GND 共地
4. 烧录：

```bash
bash flash.sh
# 或：make flash
```

5. 打开串口助手（`minicom -D /dev/ttyUSB0 -b 115200` 或 MobaXterm/PuTTY）
6. 期望看到：

```
=== Hello UART! (register-level) ===
MCU: STM32F103VET6 @ 72MHz
USART1: 115200 8N1

Hello UART! count=0
Hello UART! count=1
...
```

> **如果波特率偏**：B.1 简化版时钟是 HSI 8MHz 不是 72MHz，BRR 按 72MHz 算会偏低。看到乱码或字符变形是**预期**。进 B.3 FreeRTOS 移植时一起配时钟。

## 验收清单

- [ ] `make` 0 warning 0 error
- [ ] `make size` 输出 text < 5KB
- [ ] `st-info --probe` 找到 STM32F103VET6
- [ ] `make flash` 烧录成功
- [ ] 串口看到 "Hello UART!" 循环（波特率偏可接受，B.3 修）
- [ ] `git commit -m "phase-B-1: hello uart 寄存器版"`

## 故障排查

| 现象 | 原因 | 解决 |
|---|---|---|
| 串口乱码 | 时钟不是 72MHz | B.1 已知问题，B.3 配时钟 |
| `st-info --probe` 找不到 | ST-Link 没识别 / 驱动问题 | `dmesg \| tail` 看 USB；Ubuntu 自带 stlink 驱动 |
| `make` 报 `_estack` 未定义 | 链接脚本路径错 | Makefile 里 LDSCRIPT 是否找到 |
| `make` 报 `undefined reference to _sidata` 等 | 启动文件没编进 | 检查 ASM_SOURCES 路径 |
| 烧录后没反应 | BOOT0 拨错 | VET6 核心板 BOOT0 应拨到 **0**（运行模式）|
| HardFault | 启动文件 / 链接脚本有 bug | 用 OpenOCD + GDB 看 LR / PC 寄存器 |

## 进阶（可选）

- 把 `delay_loop` 换成 SysTick 精确延时
- 把 USART 改成 **中断方式**（简历再加分）
- 加一个 **printf 重定向到 USART1**（用 gcc -nostdlib 配合 mini printf）
