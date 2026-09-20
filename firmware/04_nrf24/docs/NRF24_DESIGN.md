# NRF24L01+ 寄存器级驱动设计稿（B.2.3）

> 本稿是"动作方案"，不是"动作本身"。代码已写好（B.2.3 完成），等硬件实测。

## 1. 为什么选 NRF24

| 无线 | 距离 | 功耗 | 价格 | 适用 |
|---|---|---|---|---|
| **NRF24L01+** | 100m | 极低 | ¥6 | ✅ 传感网 |
| WiFi (ESP32) | 50m | 高 | ¥25 | 视频 |
| BLE | 30m | 低 | ¥10 | 手机 |
| Zigbee | 100m | 低 | ¥15 | 网状 |

3 节点每秒几十字节 → NRF24 性价比 + 功耗都最优。

## 2. 引脚分配（VET6 指南者）

```
MCU        NRF24L01+
────       ──────────
PA5 (SCK)  ──► SCK
PA6 (MISO) ◄── MISO
PA7 (MOSI) ──► MOSI
PB0        ──► CE
PB1        ──► CSN
3.3V       ──► VCC（**10µF + 100nF 去耦**）
GND        ──► GND
```

⚠️ NRF24 发射瞬间电流 ~115mA，VET6 板载 LDO 可能压降导致通信失败。
**强烈建议加 10µF 电解 + 100nF 陶瓷到 NRF24 VCC 引脚**。

## 3. SPI1 配置（RM0008 第 23 章）

| 参数 | 值 | 说明 |
|---|---|---|
| 模式 | Master | STM32 主动 |
| 时钟 | fPCLK/8 = 9MHz | NRF24 上限 10MHz，留余量 |
| CPOL/CPHA | 0/0 | NRF24 模式 0 |
| 数据位 | 8 位 | |
| MSB/LSB | MSB | NRF24 默认 |
| NSS | 软件模式 | SSM=1 SSI=1，避免硬件 NSS 受多从机影响 |

## 4. NRF24 关键配置（datasheet 第 6 章）

| 寄存器 | 值 | 含义 |
|---|---|---|
| CONFIG | 0x0A (PTX) / 0x0B (PRX) | PWR_UP + 2 字节 CRC |
| EN_AA | 0x00 | 关自动应答（B.4 加回来）|
| EN_RXADDR | 0x01 | 只开 pipe 0 |
| SETUP_AW | 0x03 | 5 字节地址 |
| SETUP_RETR | 0x00 | 不重传（无 auto-ack）|
| RF_CH | 76 | 2400 + 76 = 2476 MHz（避开 WiFi）|
| RF_SETUP | 0x0E | 2 Mbps + 0 dBm |
| TX_ADDR | E7E7E7E7E7 | 目标地址（节点 1）|
| RX_ADDR_P0 | 同 TX_ADDR | P0 用于 auto-ack 对齐地址 |
| RX_PW_P0 | 32 | 接收载荷宽度 |

## 5. SPI 命令字（datasheet 表 16）

| 命令 | 值 | 用途 |
|---|---|---|
| R_REGISTER | 0x00+addr | 读寄存器 |
| W_REGISTER | 0x20+addr | 写寄存器 |
| R_RX_PAYLOAD | 0x61 | 读 FIFO 数据 |
| W_TX_PAYLOAD | 0xA0 | 写要发数据 |
| FLUSH_TX | 0xE1 | 清 TX FIFO |
| FLUSH_RX | 0xE2 | 清 RX FIFO |
| NOP | 0xFF | 读 STATUS 寄存器用 |

## 6. 测试方案

### 6.1 单板 PTX 测试（当前 main.c）

- 1 块 VET6 + 1 块 NRF24
- 配置 PTX 模式，channel 76
- 每秒发送 "HELLO #N"
- 观察 STATUS 寄存器的 TX_DS（0x20）位

**预期**：
- 串口打印 `[N] TX OK  status=0x0E`（TX_DS 置位后清零）
- ⚠️ 即使没有接收端，TX_DS 也会置位（因为没有 auto-ack）

### 6.2 双板 ping-pong（等 C8T6 到货后做）

- VET6 当 PTX，C8T6 当 PRX
- C8T6 收到后回 "PONG"
- VET6 看是否收到 PONG

## 7. 简历可写

> ✅ "基于 STM32F103C8T6 寄存器级实现 SPI1 主模式通信（fPCLK/8 = 9MHz，模式 0），驱动 NRF24L01+ 2.4GHz 无线模块，支持 PTX / PRX 双模式自动切换"
>
> ✅ "深入阅读 NRF24L01+ datasheet 第 6/8/9 章，自行实现 7 条 SPI 命令字驱动、5 字节地址过滤、STATUS 标志位轮询"
>
> ✅ "理解 2.4GHz ISM 频段信道分配（0~125，避开 WiFi 1/6/11 信道）"

## 8. 风险

| 风险 | 缓解 |
|---|---|
| VET6 LDO 供电不足 | 加 10µF + 100nF 去耦 |
| SPI 速率过高 | /8 分频（9MHz）有 1MHz 余量 |
| 地址冲突 | 用 E7 开头（不是 0x00/0xFF）|
| 接收端没开 | PTX 测试只看 TX_DS 即可 |
