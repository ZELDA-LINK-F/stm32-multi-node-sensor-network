# STM32F103 + FreeRTOS 多节点传感网

> 基于 STM32F103VET6 + FreeRTOS + NRF24L01+ 的多节点传感网项目，秋招旗舰项目 B。

## 项目一句话

3 个 STM32F103VET6 节点（采集 DHT11 温湿度 / BNO055 九轴姿态 / 模拟量），通过 NRF24L01+ 2.4 GHz 接入 ESP32-S3 网关，网关经 Wi-Fi/MQTT 上报到 OneNet 物联网平台。

## 当前进度

| Phase | 名称 | 状态 | 计划起止 |
|---|---|---|---|
| B.0 | 仓库骨架 + 决策 | ✅ 已完成 | 2026-09-18 |
| B.1 | Hello UART + 72MHz 时钟 | ✅ 重构用 BSP（1032B），实物验证通过 | W1–W2 |
| B.2 | 硬件驱动（DHT11/BNO055/NRF24）| ✅ 全部 BSP 化（DHT11 1664B/BNO055 1772B/NRF24 1932B）| W3–W4 |
| B.3 | FreeRTOS 4 任务并行 | ✅ VET6 实物跑通（6404B，绿灯 3s 心跳）| W5–W6 |
| B.4 | 自研二进制协议 + CRC16 | ✅ C 实现 + 34 个 PC 单元测试全过 | W7–W8 |
| B.5 | ESP32-S3 网关 | ✅ ESP-IDF 工程编译通过（872KB），NRF24 SPI 驱动完整 | W9–W11 |
| B.6 | 简历素材整合 | ✅ docs/RESUME.md 完成（296 行） | W12 |
| B.7 | 项目规范化 | ✅ ARCHITECTURE.md（4 层架构）+ Doxygen 注释 + 命名规范 | W13 |

## 硬件清单

见 [docs/BOM.md](docs/BOM.md)。

## 决策记录

见 [docs/DECISIONS.md](docs/DECISIONS.md)。

## 完整执行方案

见 [docs/PLAN.md](docs/PLAN.md)（根目录 `2026-09-18-stm32-plan.md` 副本）。

## 仓库结构

```
stm32_sensor_net/
├── README.md               ← 本文件
├── docs/
│   ├── PLAN.md             ← 执行方案
│   ├── DECISIONS.md        ← D1–D6 决策记录
│   ├── BOM.md              ← 硬件清单
│   ├── PROTOCOL.md         ← 协议文档（278 行，B.4）
│   ├── FREERTOS_DESIGN.md  ← FreeRTOS 设计稿（400 行，B.3）
│   ├── ARCHITECTURE.md     ← 项目架构规范（4 层架构，B.7）
│   ├── GATEWAY_DESIGN.md   ← ESP32-S3 网关设计稿（479 行，B.5）
│   └── RESUME.md           ← 秋招简历素材（296 行，B.6）
├── firmware/
│   ├── Makefile.common     ← STM32 GCC 编译共用规则
│   ├── common/              ← 公共 BSP 层（5 个工程共用）
│   │   ├── stm32f1xx.h          ← SYS 寄存器宏 + Doxygen
│   │   ├── system_stm32f1xx.c/.h ← SYS SystemInit（72MHz）
│   │   ├── hal_gpio.c/.h         ← HAL GPIO
│   │   ├── hal_usart.c/.h        ← HAL USART1 115200
│   │   ├── hal_delay.c/.h        ← HAL DWT 精确延时
│   │   └── protocol.c/.h         ← 协议层（CRC16-MODBUS）
│   ├── common/             ← SystemInit 72MHz 时钟（3 个工程共享）
│   ├── 01_hello_uart/      ← B.1 工程（BSP 重构，编译通过 1032B）
│   ├── 02_dht11/           ← B.2.1 工程（BSP 重构，编译通过 1664B）
│   ├── 03_bno055/          ← B.2.2 工程（BSP 重构，编译通过 1772B）
│   └── 04_nrf24/           ← B.2.3 工程（BSP 重构，编译通过 1932B）
│       └── docs/DHT11_DESIGN.md   ← 单总线协议 + 时序分析
├── gateway/                ← ESP32-S3 网关（B.5 起填）
├── tools/
│   ├── install_toolchain.sh ← 工具链一键安装
│   └── install_cubemx.sh    ← CubeMX 安装脚本
└── .gitignore
```

## 简历素材（✅ B.6 完成）

完整简历素材 + 面试 Q&A + 杀手锏话术见 **[docs/RESUME.md](docs/RESUME.md)**（296 行）

快速摘要：

```
项目：基于 STM32F103VET6 的多节点传感网系统
• 异构多节点（2×C8T6 + 1×VET6）+ NRF24L01+ 2.4GHz 组网
  + ESP32-S3 网关经 MQTT 上报 OneNet
• FreeRTOS V10.6.1 移植，4 任务 + 2 队列，BNO055 100Hz 数据零丢失
• 寄存器级外设驱动（USART/I2C/SPI），固件 < 8KB，不依赖 HAL
• 自研二进制协议（帧头+类型+CRC16-MODBUS）+ Python 验证工具
```

## 许可

MIT
