# 04_nrf24 — NRF24L01+ 寄存器级 SPI1 驱动（B.2.3）

> **状态**：🟡 编译通过（2104B），等硬件实测 + 双板 ping-pong
>
> 简历可写：
> ✅ "STM32F103 SPI1 寄存器级驱动（9MHz，模式 0）+ NRF24L01+ 2.4GHz 收发"
> ✅ "深入阅读 NRF24 datasheet 第 6/8/9 章"

## 文件结构

```
04_nrf24/
├── Makefile                  ← 编译
├── STM32F103VETX_FLASH.ld
├── Startup/startup_stm32f103vctx.s
├── Src/
│   ├── main.c                ← PTX 测试（每秒发 HELLO）
│   ├── bsp_spi1.c/.h         ← SPI1 寄存器驱动（RM0008 Ch.23）
│   └── bsp_nrf24.c/.h        ← NRF24 应用层（datasheet Ch.6/8/9）
└── docs/NRF24_DESIGN.md      ← 设计稿
```

## 编译

```bash
make          # 期望 build/nrf24.elf ~2.1KB
make size     # 详细大小
make flash    # 烧录到 VET6
```

## 接线（VET6 指南者）

| VET6 | NRF24 |
|---|---|
| PA5 | SCK |
| PA6 | MISO |
| PA7 | MOSI |
| PB0 | CE |
| PB1 | CSN |
| 3.3V | VCC + **10µF + 100nF 去耦** |
| GND | GND |

## 验收清单

- [x] `make` 0 warning
- [x] 串口打印 "NRF24 detected"
- [ ] 每秒 1 行 TX OK 状态（**等硬件**）
- [ ] STATUS 寄存器 bit5 (TX_DS) 置位
- [ ] 双板 ping-pong（**等 C8T6**）
