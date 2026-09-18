# STM32F103 + FreeRTOS 多节点传感网

> 基于 STM32F103VET6 + FreeRTOS + NRF24L01+ 的多节点传感网项目，秋招旗舰项目 B。

## 项目一句话

3 个 STM32F103VET6 节点（采集 DHT11 温湿度 / MPU6050 姿态 / 模拟量），通过 NRF24L01+ 2.4 GHz 接入 ESP32-S3 网关，网关经 Wi-Fi/MQTT 上报到 OneNet 物联网平台。

## 当前进度

| Phase | 名称 | 状态 | 计划起止 |
|---|---|---|---|
| B.0 | 仓库骨架 | 🟡 进行中（B.1 脚手架已就绪） | 2026-09-18 |
| B.1 | 环境搭建 + Hello UART | 🟡 进行中（脚手架 + Makefile） | W1–W2 |
| B.2 | 硬件驱动（DHT11 / MPU6050 / NRF24）| 🟡 进行中（脚手架 + Makefile） | W3–W4 |
| B.3 | FreeRTOS 移植（3 任务并行）| 🟡 进行中（脚手架 + Makefile） | W5–W6 |
| B.4 | 自研二进制协议 + CRC16 | 🟡 进行中（脚手架 + Makefile） | W7–W8 |
| B.5 | 多节点组网 + OneNet 上报 | 🟡 进行中（脚手架 + Makefile） | W9–W11 |
| B.6 | 文档 + 简历素材 | 🟡 进行中（脚手架 + Makefile） | W12 |

## 硬件清单

见 [docs/BOM.md](docs/BOM.md)。

## 决策记录

见 [docs/DECISIONS.md](docs/DECISIONS.md)。

## 完整执行方案

见 [docs/PLAN.md](docs/PLAN.md)（根目录 `2026-09-18-stm32-plan.md` 副本）。

## 仓库结构

```
stm32_sensor_net/
├── README.md           ← 本文件
├── docs/               ← 设计文档
├── firmware/           ← STM32 节点固件（B.1 起填）
├── gateway/            ← ESP32-S3 网关（B.5 起填）
├── tools/              ← 协议测试脚本等（B.4 起填）
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
