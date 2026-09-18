# BNO055 寄存器级驱动设计稿（B.2.2）

> **本设计稿是"动作方案"，不是"动作本身"。**
> 等硬件（C8T6 + GY-BNO055）到位后烧录验证。

## 1. BNO055 是什么

**GY-BNO055** 是 Bosch 公司的 9 轴智能传感器，**自带 ARM Cortex-M0+ 跑传感器融合算法**。

| 特性 | 值 |
|---|---|
| 轴数 | 9（加速度 + 陀螺仪 + 磁力计）|
| 输出 | 欧拉角 / 四元数 / 旋转向量 / 重力向量 |
| 融合算法 | **板载**（不用自己写卡尔曼滤波）|
| 通信 | I2C（默认）/ UART / SPI |
| I2C 地址 | **0x28**（ADR=GND）/ 0x29（ADR=VDD）|
| 工作电压 | 3.3V（GY 模块板上自带稳压）|
| 价格 | ¥40-80（淘宝 GY-BNO055）|

### BNO055 vs MPU6050 优势

| 维度 | MPU6050 | **BNO055** |
|---|---|---|
| 轴数 | 6 | **9（含磁力计）** |
| 融合 | ❌ 需自己写 | ✅ 板载 |
| 输出 | 原始数据 | **直接欧拉角** |
| 精度（yaw）| 漂移 | **磁力计校准无漂移** |
| 简历可写 | "读加速度陀螺仪" | **"9 轴融合 + 板载算法 + 磁力计校准"** |

## 2. I2C 协议基础（RM0008 第 24 章）

### 2.1 I2C 物理层

```
MCU                    BNO055
 │                        │
SCL ──────────────┬────── │
                 │      │
                 │      SDA ──┬────── SCL
                 │             │
                 │             │ (开漏，需要上拉 4.7kΩ)
                 │             │
                4.7kΩ          4.7kΩ
                 │             │
                VCC           VCC

特点：
- 双向单线数据（SDA）+ 时钟线（SCL）
- 开漏输出，必须上拉
- 多设备可挂同一条总线（用地址区分）
```

### 2.2 I2C 时序

```
起始信号（START）：
    SCL ──────┐         ┌───
              └─────────┘
    SDA ─┐         ┌───
         └─────────┘
         ↑ SCL 高时 SDA 下降 = START

停止信号（STOP）：
    SCL ──────┐         ┌───
              └─────────┘
    SDA ───────┐         ┌──
              └─────────┘
              ↑ SCL 高时 SDA 上升 = STOP

数据位：
    SCL ──┐   ┌───┐   ┌───
          └───┘   └───┘
    SDA ───┐         ┌──
          └─────────┘
          ↑ SCL 低时 SDA 可变
          ↑ SCL 高时 SDA 必须稳定（采样）
```

### 2.3 I2C 地址字节

```
8 位地址 = 7 位设备地址 + 1 位 R/W
BNO055 地址 = 0x28（7 位）= 0x50（8 位写）/ 0x51（8 位读）

示例：写 BNO055
  START → 0x50（地址+W）→ ACK → 寄存器地址 → ACK → 数据 → ACK → STOP
```

### 2.4 STM32 I2C1 引脚

| 引脚 | 复用功能 | 配置 |
|---|---|---|
| PB6 | I2C1_SCL | 复用开漏输出，50MHz |
| PB7 | I2C1_SDA | 复用开漏输出，50MHz |

需要 4.7kΩ 上拉电阻（GY-BNO055 模块板上通常已焊）。

## 3. BNO055 关键寄存器（datasheet §4）

| 寄存器 | 地址 | 用途 |
|---|---|---|
| **CHIP_ID** | 0x00 | 应读出 **0xA0**（识别 BNO055）|
| **ACC_DATA_X_LSB** | 0x08 | 加速度 X LSB |
| **EUL_PITCH_LSB** | 0x1E | 俯仰角 LSB |
| **EUL_ROLL_LSB** | 0x1F | 横滚角 LSB |
| **EUL_HEADING_LSB** | 0x1B | 航向角 LSB |
| **EUL_PITCH_MSB** | 0x1D | 俯仰角 MSB |
| **EUL_ROLL_MSB** | 0x1C | 横滚角 MSB |
| **EUL_HEADING_MSB** | 0x1A | 航向角 MSB |
| **OPR_MODE** | 0x3D | **操作模式**（NDOF = 0x0C）|
| **PWR_MODE** | 0x3E | 电源模式（0x00 = Normal）|
| **SYS_TRIGGER** | 0x3F | 系统触发（含复位位 0x20）|

## 4. 操作流程

### 4.1 初始化流程

```
1. I2C1 初始化（100kHz @ 36MHz APB1）
   - 开 GPIOB 时钟 + I2C1 时钟
   - PB6/PB7 配复用开漏
   - I2C1_CR2 = 36（APB1 时钟 MHz）
   - I2C1_CCR = 180（100kHz 速率）
   - I2C1_CR1 |= 1（PE = 1，使能）

2. 探测 BNO055
   - 读寄存器 0x00（CHIP_ID）
   - 应读出 0xA0
   - 不对 → I2C 总线/地址/接线问题

3. 配置为 NDOF 模式
   - 写寄存器 0x3D = 0x0C（NDOF = 九轴融合）
   - 等待 10ms（模式切换需要时间）

4. 主循环读欧拉角
   - 每 100ms 读寄存器 0x1A-0x1F（6 字节 = 3 个 16 位欧拉角）
   - 转换为度数：raw_value / 16.0
```

### 4.2 欧拉角数据格式

```
原始字节：[HEADING_MSB, HEADING_LSB, ROLL_MSB, ROLL_LSB, PITCH_MSB, PITCH_LSB]

角度 = (MSB << 8 | LSB) / 16.0  （单位：度）

示例：
  bytes = [0x02, 0x00, 0xFE, 0x80, 0x01, 0x40]
  heading = (0x02 << 8 | 0x00) / 16.0 = 512 / 16 = 32.0°
  roll    = (0xFE << 8 | 0x80) / 16.0 = -384 / 16 = -24.0°
  pitch   = (0x01 << 8 | 0x40) / 16.0 = 320 / 16 = 20.0°
```

### 4.3 I2C 寄存器读写函数

```c
// 写 1 字节到 BNO055 寄存器
uint8_t bno055_write(uint8_t reg, uint8_t data) {
    i2c1_start();
    i2c1_send_addr(0x28 << 1 | 0);   // 0x50 写地址
    if (!i2c1_wait_ack()) return 0;
    i2c1_send_byte(reg);
    if (!i2c1_wait_ack()) return 0;
    i2c1_send_byte(data);
    if (!i2c1_wait_ack()) return 0;
    i2c1_stop();
    return 1;
}

// 从 BNO055 读 1 字节
uint8_t bno055_read(uint8_t reg, uint8_t *data) {
    // 先写寄存器地址（不发送 STOP）
    i2c1_start();
    i2c1_send_addr(0x28 << 1 | 0);
    if (!i2c1_wait_ack()) return 0;
    i2c1_send_byte(reg);
    if (!i2c1_wait_ack()) return 0;
    
    // 重启 + 读
    i2c1_start();
    i2c1_send_addr(0x28 << 1 | 1);   // 0x51 读地址
    if (!i2c1_wait_ack()) return 0;
    *data = i2c1_receive_byte();
    i2c1_send_nack();   // 最后一个字节发 NACK
    i2c1_stop();
    return 1;
}
```

## 5. 决策：HAL 还是寄存器级？

| 维度 | HAL I2C | **寄存器级 I2C** |
|---|---|---|
| 代码量 | 50 行 | **200 行** |
| HAL 依赖 | 需要 STM32CubeF1 源码包 | **无依赖** |
| 调试难度 | HAL 出错难定位 | **直接看寄存器** |
| 简历含金量 | ⭕ HAL API 调用 | ✅ **"读 RM0008 自己实现 I2C 协议"** |
| 开发速度 | 快 | 慢 |
| **本项目选择** | ⭕ | ✅ **寄存器级**（保持项目一致性）|

> **决策依据**：B.1 hello_uart 已用寄存器级 USART，B.2.1 DHT11 用寄存器级单总线，B.2.2 BNO055 用寄存器级 I2C —— **整个项目统一风格**，简历可写"从 RM0008 一路读到底实现 UART / 单总线 / I2C 三种协议"。

## 6. 简历可写话术

> ✅ "基于 STM32F103C8T6 实现 BNO055 九轴姿态传感器驱动（I2C 协议寄存器级实现），读取欧拉角含磁力计校准，无累计漂移"

> ✅ "通过 BNO055 板载 ARM M0+ 融合算法，**避免自实现卡尔曼滤波**，节省 200+ 行代码"

> ✅ "对比 MPU6050 与 BNO055 选型：BNO055 磁力计校准避免 yaw 漂移，板载融合降低 CPU 占用至 < 1%"

## 7. 验收清单（B.2.2 完成）

- [ ] `make` 0 warning
- [ ] I2C1 初始化正确（示波器能看到 SCL/SDA 波形）
- [ ] 读 CHIP_ID = 0xA0
- [ ] 配置 NDOF 模式成功
- [ ] 串口每 100ms 打印一次欧拉角（roll/pitch/yaw）
- [ ] 转动 BNO055 → 串口数值实时变化
- [ ] yaw 长时间不动不漂移（验证磁力计校准）
- [ ] `git commit -m "phase-B-2: bno055 寄存器级 I2C"`

## 8. 风险与陷阱

| 风险 | 后果 | 解决 |
|---|---|---|
| 4.7kΩ 上拉电阻没接 | I2C 通信完全失败 | GY-BNO055 模块板上一般已焊，没焊就外接 |
| I2C 时钟太快（400kHz）| 信号失真 | 建议 100kHz（BNO055 默认）|
| 时钟不是 72MHz | I2C CCR 计算错 | B.3 FreeRTOS 时一起配 |
| BNO055 模式切换时间 | 立即读欧拉角得 0 | 切模式后 `delay_ms(10)` |
| 多个 I2C 设备地址冲突 | 总线锁死 | 默认 BNO055 = 0x28，确认没冲突 |

## 9. 文件结构（计划）

```
firmware/03_bno055/
├── Makefile
├── Src/
│   ├── main.c              # 主程序
│   ├── bsp_i2c1.c          # I2C1 寄存器级驱动
│   ├── bsp_i2c1.h
│   ├── bsp_bno055.c        # BNO055 应用层
│   └── bsp_bno055.h
├── Startup/
│   └── startup_stm32f103vctx.s
├── STM32F103VETX_FLASH.ld
├── README.md
└── docs/
    └── BNO055_DESIGN.md    # 本文档
```
