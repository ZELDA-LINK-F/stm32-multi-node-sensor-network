# STM32F103 + FreeRTOS 多节点传感网

> 基于 STM32F103VET6 + FreeRTOS + NRF24L01+ 的多节点传感网项目，秋招旗舰项目 B。

## 项目一句话

3 个 STM32F103VET6 节点（采集 DHT11 温湿度 / BNO055 九轴姿态 / 模拟量），通过 NRF24L01+ 2.4 GHz 接入 ESP32-S3 网关，网关经 Wi-Fi/MQTT 上报到 OneNet 物联网平台。

## 当前进度

| Phase | 名称 | 状态 | 计划起止 |
|---|---|---|---|
| B.0 | 仓库骨架 + 决策 | ✅ 已完成 | 2026-09-18 |
| B.1 | 环境搭建 + Hello UART | 🟢 编译通过（836B，含 SystemInit 72MHz），等烧录验证 | W1–W2 |
| B.2 | 硬件驱动（DHT11 / BNO055 / NRF24）| 🟢 DHT11 + BNO055 + NRF24 全部编译通过，等硬件实测 | W3–W4 |
| B.3 | FreeRTOS 移植（3 任务并行）| ⬜ 未开始 | W5–W6 |
| B.4 | 自研二进制协议 + CRC16 | ⬜ 未开始 | W7–W8 |
| B.5 | 多节点组网 + OneNet 上报 | ⬜ 未开始 | W9–W11 |
| B.6 | 文档 + 简历素材 | ⬜ 未开始 | W12 |

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
│   └── GATEWAY_DESIGN.md   ← ESP32-S3 网关设计稿（479 行，B.5）
├── firmware/
│   ├── Makefile.common     ← STM32 GCC 编译共用规则
│   ├── common/             ← SystemInit 72MHz 时钟（3 个工程共享）
│   ├── 01_hello_uart/      ← B.1 工程（寄存器级，编译通过 836B）
│   ├── 02_dht11/           ← B.2.1 工程（编译通过 940B）
│   ├── 03_bno055/          ← B.2.2 工程（编译通过 2052B）
│   └── 04_nrf24/           ← B.2.3 工程（编译通过 2104B）
│       └── docs/DHT11_DESIGN.md   ← 单总线协议 + 时序分析
├── gateway/                ← ESP32-S3 网关（B.5 起填）
├── tools/
│   ├── install_toolchain.sh ← 工具链一键安装
│   └── install_cubemx.sh    ← CubeMX 安装脚本
└── .gitignore
```

## 简历素材（占位，B.6 完成后写）

```
项目：基于 STM32F103VET6 的多节点传感网系统
- 设计自研二进制通信协议（帧头+长度+类型+CRC16），CRC 校验保证 1% 误码率下零漏检
- 3 个 STM32 节点通过 NRF24L01+ 2.4GHz 接入 ESP32-S3 网关，网关经 Wi-Fi/MQTT 上报到 OneNet
- FreeRTOS 实现 3 任务并行采集，队列 + 二值信号量同步，任务周期抖动 < 1ms
```

## 许可

MIT
