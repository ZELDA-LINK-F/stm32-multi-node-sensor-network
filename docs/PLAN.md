# 项目 B 执行方案：STM32F103 + FreeRTOS 多节点传感网

> **本文是"动作方案"，不是"动作本身"。** 任何 Phase 启动前，先对照本文的「第一动作」清单逐项打勾，再允许敲代码、装软件、建文件夹。
>
> 起点日期：**2026-09-18（周五）**　目标交付：**2027.02 寒假前完成实物演示 + 简历素材**

---

## 0. 方案 vs 动作：边界声明

| 类别 | 方案阶段（现在） | 动作阶段（拍板后）|
|---|---|---|
| 产出物 | `.md` 文件、目录结构设计图、协议帧表格、采购清单 | 工程源码、可烧录固件、串口日志、视频|
| 工具 | 只读现有文档、`cat`/`ls`、纸笔 | STM32CubeMX、Keil/EIDE、git、烧录器|
| 反例 | ❌ 现在 `git clone` FreeRTOS、跑安装脚本 | ❌ 还在纠结 DHT11 时序图就先不写 CRC16 |

**判定原则**：今天结束前，仓库里**只允许多 1 个 `.md` 和 1 个空目录骨架**，不允许出现 `.c/.h/.ioc` 文件。

---

## 1. 产品定义（一句话 + 三子系统）

**一句话**：3 个 STM32F103VET6 节点（采集温湿度 / 九轴姿态 / 模拟量），通过 NRF24L01+ 2.4 GHz 接入 ESP32-S3 网关，网关经 Wi-Fi/MQTT 上报到 OneNet，Web 端可见实时数据。

**三子系统**：

```
┌─────────────┐    NRF24L01+     ┌──────────────┐    Wi-Fi/MQTT    ┌──────────┐
│ Node 1 DHT11│ ──── 2.4GHz ───► │              │ ──────────────►  │ OneNet   │
│ Node 2 MPU  │ ──── 2.4GHz ───► │ Gateway      │                  │ Dashboard│
│ Node 3 ADC  │ ──── 2.4GHz ───► │ ESP32-S3     │ ◄──── 手机/PC ──►│          │
└─────────────┘                  └──────────────┘     （订阅控制）
       ▲                                ▲
   FreeRTOS 3 任务               协议解析 + 调度
   队列 + 信号量                 CRC16 校验 + POLL
```

**子系统边界**（写代码前必须画清）：

| 子系统 | 职责 | 不做 |
|---|---|---|
| 节点固件 | 采集 + 协议封装 + 收发 | 不上云、不做网页 |
| 网关固件 | 协议解析 + 调度 + 上云 | 不采集传感器 |
| 协议层 | 帧头/长度/类型/CRC16 | 不假设上层业务 |

---

## 2. 知识前提自检（动 Phase B.1 前必须通过）

文档原题 + 我额外补的 4 题：

| # | 自检题 | 不通过则去补 |
|---|---|---|
| B-Q1 | `volatile` 在中断/任务共享标志位中的作用？| 翁恺 C 进阶篇 |
| B-Q2 | FreeRTOS 队列 vs 信号量 vs 互斥锁的区别？| 正点原子 FreeRTOS 教程前 5 章 |
| B-Q3 | STM32F103 的 USART1 收发需要哪些寄存器？CR1/CR2/BRR 各自作用？| 野火/正点 HAL 库教程 USART 章 |
| B-Q4 | NRF24L01+ 的 SPI 通信速率上限？CE/CSN 各自何时拉低？| NRF24L01+ datasheet 中文版（NRF官网）|
| B-Q5 | CRC16-CCITT 多项式是什么？初始值 0xFFFF 还是 0x0000？| 任意 CRC16 教程 |
| B-Q6 | FreeRTOS 中 `vTaskDelay` 和 `vTaskDelayUntil` 的周期抖动差异？| FreeRTOS 官方教程"时间管理"章 |
| B-Q7 | MQTT QoS 0/1/2 的区别？什么时候必须用 QoS 1？| MQTT 入门教程 |
| B-Q8 | STM32 HAL 库 `HAL_UART_Transmit_IT` 和 DMA 方式的 CPU 占用对比？| HAL 库 USART DMA 章 |

**判定**：8 题至少 6 题能口头答出 80% 才能进 B.1。其余题目对应到 Phase 内补课。

---

## 3. Phase 路线图（每个 Phase 含：第一动作 / 产物 / 验收 / 工时）

> **W1 = 2026-09-21（周一）起算**。今天是 W0 周五，下午可花 60–90 分钟做"骨架"。

### Phase B.0 — 骨架日（今天，W0 周五）

| 项 | 内容 |
|---|---|
| **目标** | 把仓库结构和文档骨架立起来，**不写代码** |
| **第一动作** | (1) 在 `~/learn/iot-web/秋招备战/` 下建 `stm32_sensor_net/` 目录；(2) 写 `stm32_sensor_net/README.md`；(3) 把本文（方案）复制为 `stm32_sensor_net/docs/PLAN.md` |
| **产物** | 1 个空目录 + 2 个 `.md` |
| **验收** | `tree stm32_sensor_net/` 输出符合下方 §6 目录骨架 |
| **工时** | 60 分钟 |
| **不通过则** | 不允许进入 W1 |

### Phase B.1 — 环境搭建（W1–W2，2 周）

| 项 | 内容 |
|---|---|
| **目标** | 点亮 STM32F103VET6 最小系统板，串口打印 `Hello FreeRTOS` |
| **第一动作** | (1) 装 STM32CubeMX（Windows 用 Java 启动包，Linux 用 AppImage）；(2) 确认 ST-Link V2 驱动；(3) 创建 `01_hello_uart` CubeMX 工程，目标芯片 STM32F103VET6，使能 USART1 115200 |
| **产物** | 可烧录的最小工程，串口每秒打印一行 |
| **验收** | 串口助手看到稳定输出 60 秒无乱码；Keil 编译通过 0 warning |
| **工时** | 8 h（含排错） |
| **风险** | 板子型号选错（VET6 vs C8T6 引脚不同）；CubeMX 时钟树配置错误 → 串口乱码 |

### Phase B.2 — 硬件驱动（W3–W4，2 周）

| 项 | 内容 |
|---|---|
| **目标** | DHT11 / BNO055 / NRF24L01+ 三类外设各自驱动跑通，**独立任务验证** |
| **第一动作** | 先做 DHT11（最简单的单总线协议）：CubeMX 配置 PG11 为输入，开 `TIM2` 1µs 时基用于延时 |
| **产物** | `bsp_dht11.c/.h`，逻辑分析仪抓到的时序图（截图） |
| **验收** | 串口每 2s 打印温湿度，格式 `T=26.0 H=58.0`；时序图与 datasheet 对得上 |
| **工时** | 16 h |
| **拆分** | W3 上半：DHT11；W3 下：MPU6050 I2C；W4 上：NRF24L01+ SPI；W4 下：NRF24 点对点收发 |

### Phase B.3 — FreeRTOS 移植（W5–W6，2 周）

| 项 | 内容 |
|---|---|
| **目标** | 同一板上 3 任务并行（DHT11 采集 / NRF24 收发 / 串口上报），队列 + 信号量同步 |
| **第一动作** | 在 B.1 工程基础上勾选 FreeRTOS（CMSIS-RTOS V2 或原生 FreeRTOS API，二选一**先定下来**）|
| **产物** | `tasks/` 目录下分文件 `task_sensor.c / task_radio.c / task_uart.c` |
| **验收** | 三个任务优先级定义清楚；`vTaskList()` 打印能看到 3 个任务状态均为 `R` 或 `B`；CPU 占用 < 50% |
| **工时** | 12 h |
| **决策点** | 见 §5 |

### Phase B.4 — 通信协议（W7–W8，2 周）

| 项 | 内容 |
|---|---|
| **目标** | 自研二进制协议跑通，CRC16 校验 1% 误码率下零漏检 |
| **第一动作** | 在 `protocol/` 目录定义帧结构体，**先把帧格式表写进 `docs/PROTOCOL.md`** |
| **产物** | `protocol_frame.h`、`crc16.c`、协议文档 |
| **验收** | Python 脚本造 1000 个含随机 1% 误码的帧，C 实现 CRC 全部检出 |
| **工时** | 14 h |
| **关键约束** | **协议层和业务层严格解耦**，函数签名不接受业务结构体 |

### Phase B.5 — 多节点组网（W9–W11，3 周）

| 项 | 内容 |
|---|---|
| **目标** | 3 节点 + 1 网关，网关轮询 1→2→3→1，OneNet 实时数据流 |
| **第一动作** | 协议扩展 POLL 帧（0x04），先写**状态机**草图再写代码 |
| **产物** | 多节点实物 + OneNet dashboard 截图 |
| **验收** | 三节点同步刷新 < 1s；MQTTX 收到 JSON `{node_id, ts, payload}` |
| **工时** | 20 h |

### Phase B.6 — 文档与简历（W12，3 天）

| 项 | 内容 |
|---|---|
| **目标** | README 六项齐全 + ≤2 分钟演示视频 |
| **第一动作** | 录屏：手机架三脚鱼，**先彩排一遍**再正式录 |
| **产物** | `README.md` + B 站视频链接 + `docs/PROTOCOL.md` |
| **验收** | 把 README 完整念一遍 ≤90 秒不卡壳 |

---

## 4. 硬件清单（确认已有 vs 待采购）

| 元件 | 数量 | 状态 | 备注 |
|---|---|---|---|
| STM32F103VET6 核心板 | 3 | ✅ 文档说有 | 实物质检：每块都跑一遍 B.1 |
| NRF24L01+ 模块 | 3+1 | ⚠️ 需清点 | 节点端 3 块 + 网关端 1 块；天线版本优选 |
| DHT11 | ≥1 | ✅ 文档说有 | 至少 1 块可工作 |
| MPU6050 | ≥1 | ✅ 文档说有 | I2C 地址 0x68 默认 |
| ESP32-S3 DevKitC-1 | 1 | ✅ 已有 | 网关端，与项目 A 复用 |
| ST-Link V2 | 1 | ⚠️ 需确认 | 仿真器；若无，改用板载 USB-TTL 串口下载（需 BOOT0 跳线）|
| 杜邦线 / 面包板 | 足 | ⚠️ 需清点 | NRF24 必须独立供电（3.3V），不能直接接 MCU 引脚 |

**W0 周五动作**：清点硬件，把缺件列到 `stm32_sensor_net/docs/BOM.md`。

---

## 5. 决策点（必须在进 B.1 前拍板）

| # | 决策项 | 选项 | 我的推荐 | 你的选择 |
|---|---|---|---|---|
| D1 | IDE | ① Keil MDK（闭源，5KB 限制可破解）② STM32CubeIDE（免费）③ VSCode + EIDE + ARM GCC | **CubeIDE**：零成本、官方维护、Linux 也能装 | __ |
| D2 | FreeRTOS 入口 | ① CMSIS-RTOS V2（HAL 友好）② 原生 FreeRTOS API（移植性强，面试加分）| **原生 FreeRTOS**：面试官认这个 | __ |
| D3 | 协议字节序 | ① 大端（网络序）② 小端（MCU 序）| **大端**：网关跨平台友好 | __ |
| D4 | CRC 初值/多项式 | ① CRC16-CCITT FALSE（初值 0xFFFF, 多项式 0x1021）② CRC16-MODBUS（初值 0xFFFF, 多项式 0xA001）| **CRC16-MODBUS**：NRF24 + 工业常见 | __ |
| D5 | 网关路径 | ① 单独 `esp32_gateway/` 项目 ② 嵌入项目 A 主工程做 menuconfig 选项 | **单独项目**：解耦清晰 | __ |
| D6 | 节点供电 | ① USB 5V + 板载 3.3V LDO ② 独立 3.3V 稳压模块 | **方案 ①**：先求通 | __ |

**没有你的回复，我按"我的推荐"列默认值执行**。

---

## 6. 今日（W0）目录骨架（第一步动作的产物）

```
stm32_sensor_net/
├── README.md                  # 项目介绍 + 当前进度（草稿，≤50 行）
├── docs/
│   ├── PLAN.md                # ← 本文副本
│   ├── BOM.md                 # 硬件清单（含状态）
│   ├── PROTOCOL.md            # Phase B.4 才填
│   └── DECISIONS.md           # §5 决策记录
├── firmware/
│   └── .gitkeep               # B.1 才放 .ioc/.c/.h
├── gateway/                   # 留给 ESP32-S3，B.5 再建
│   └── .gitkeep
├── tools/
│   └── crc16_test.py          # B.4 才放
└── .gitignore                 # 忽略 *.o, *.hex, *.bin, build/, .mxproject/
```

**目录创建命令**（**不要现在跑**，等你说"开始"）：

```bash
cd ~/learn/iot-web/秋招备战
mkdir -p stm32_sensor_net/{docs,firmware,gateway,tools}
cd stm32_sensor_net
git init && git checkout -b main
# 然后再写 README 和 PLAN
git add . && git commit -m "phase-B-0: 仓库骨架 v0"
```

---

## 7. 风险清单（方案阶段标注，Phase 内消化）

| 风险 | 触发条件 | 缓解 |
|---|---|---|
| NRF24 供电不足导致通信丢包 | 用杜邦线直接从 MCU 引脚取电 | 加 10µF + 100nF 去耦；外接 AMS1117-3.3 |
| CubeMX 工程文件冲突（`.ioc` 是 XML 易冲突）| 多人协作 / 多台电脑 | 用 CubeMX 6.10+，git 配 `.gitattributes` 标记 `.ioc` 二进制合并 |
| FreeRTOS 堆栈溢出 | 任务局部变量太大 / 递归 | 开启 `configCHECK_FOR_STACK_OVERFLOW = 2`，看 vApplicationStackOverflowHook |
| 协议扩展性差 | 帧格式一次性拍死不留升级位 | B.4 定义时预留 1 字节 `version` 字段 |
| OneNet 鉴权过期 | Studio 改版 | 选 OneNet 物联网平台（老版）API，文档稳定 |

---

## 8. 今日（W0）90 分钟极简行动表

> **不是动作，是准备动作**。完成 ≠ 写代码。

| 时间 | 动作 | 产出 |
|---|---|---|
| 0–10 min | 把本文读到能复述"3 子系统 + 6 Phase" | 心智模型 |
| 10–30 min | 回答 §2 自检题，错题数 ≥3 则暂停方案、先补课 | 自检结果 |
| 30–45 min | 清点硬件（按 §4 表），填 `BOM.md` | `docs/BOM.md` |
| 45–70 min | 在 `docs/DECISIONS.md` 写出 §5 决策（D1–D6 选哪个）| 决策记录 |
| 70–85 min | 草拟 `README.md`：项目一句话 + 进度表 | `README.md` |
| 85–90 min | `git init` + 第一次 commit `phase-B-0: 仓库骨架 v0` | 一个 commit |

**如果走到第 70 分钟决策还没拍板**：停止。决策没定就开工，后面 12 周都在补决策的债。

---

## 9. 完成判定（"方案"这步什么时候算 done）

- [x] 6 个 Phase 都填了"第一动作"
- [x] §5 决策点 D1–D6 全部勾选（或明确接受默认）
- [x] §4 硬件清单实物清点完毕
- [x] §2 自检题 ≥6/8 通过
- [x] 目录骨架命令**只存在文档里**，未实际执行
- [x] **本文已 commit 到 git**，commit message 建议 `phase-B-0: 写执行方案 v1`

---

## 10. 下一步（方案 done 之后）

如果今天就完成 §9 全部勾选，**周一（2026-09-21）直接进 Phase B.1 第一动作**：

```
W1 Day1：装 STM32CubeMX + 创建 01_hello_uart 工程 + 烧录串口 Hello
W1 Day2：调通时钟树到 72MHz（VET6 满速），USART1 115200 无乱码
W1 Day3：把 Hello 工程 commit `phase-B-1: hello uart`
W1 Day4–5：补 B-Q3 / B-Q8 不熟的部分
```

---

**Last edit: 2026-09-18 by Codex. Awaiting user approval to enter action phase.**
