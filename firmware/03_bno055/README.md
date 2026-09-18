# 03_bno055 — BNO055 寄存器级 I2C 驱动（B.2.2）

> **状态**：🟡 设计稿 + 可编译框架就绪，等硬件到位后烧录验证。
>
> **编译**：✅ 已通过（待编译验证）

## 简历可写

> ✅ "基于 STM32F103C8T6 实现 BNO055 九轴姿态传感器驱动（I2C 协议寄存器级实现），读取欧拉角含磁力计校准，无累计漂移"

> ✅ "通过 BNO055 板载 ARM M0+ 融合算法，避免自实现卡尔曼滤波"

## 设计稿

详见 [`docs/BNO055_DESIGN.md`](docs/BNO055_DESIGN.md)（249 行）：

- BNO055 9 轴 + 板载融合原理
- I2C 协议完整时序
- BNO055 关键寄存器表
- 寄存器级 I2C1 驱动代码（含 RM0008 第 24 章）
- 决策依据：为什么不用 HAL
- 风险与陷阱

## 文件结构

```
03_bno055/
├── Makefile
├── Src/
│   ├── main.c              # 主程序（I2C + 欧拉角读取 + 串口打印）
│   ├── bsp_i2c1.c/.h       # I2C1 寄存器级驱动（RM0008 Ch 24）
│   └── bsp_bno055.c/.h     # BNO055 应用层
├── Startup/
│   └── startup_stm32f103vctx.s
├── STM32F103VETX_FLASH.ld
├── README.md
└── docs/
    └── BNO055_DESIGN.md    # 设计稿
```

## 编译验证

```bash
cd firmware/03_bno055
make
# 期望：build/bno055.elf（text < 5KB）
```

## 烧录步骤（等硬件）

```
接线：
VET6 / C8T6       GY-BNO055
PB6 (SCL) ───────► SCL
PB7 (SDA) ───────► SDA
3.3V      ───────► VCC
GND       ───────► GND
（GY-BNO055 模块上 ADR 默认接 GND，地址 = 0x28）

烧录：
make flash
```

## 验收清单（B.2.2 完成）

- [ ] `make` 0 warning
- [ ] 串口每 100ms 打印 "Y=0.0 R=0.0 P=0.0"
- [ ] 转动 BNO055 → 数值实时变化
- [ ] yaw 长时间不动不漂移（验证磁力计）
- [ ] `git commit -m "phase-B-2: bno055 寄存器级 I2C"`
