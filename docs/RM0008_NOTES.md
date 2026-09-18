# RM0008 关键章节笔记（STM32F103 Reference Manual）

> 用于本项目寄存器级开发速查。配套 B.1 hello_uart、B.2 驱动、B.3 时钟配置。

## 第 7 章 RCC（Reset and Clock Control）

### 7.3 RCC 寄存器

| 寄存器 | 地址 | 用途 |
|---|---|---|
| `RCC_CR` | 0x40021000 | 时钟源控制（HSE / HSI / PLL）|
| `RCC_CFGR` | 0x40021004 | 时钟选择 + 分频 |
| `RCC_APB2ENR` | 0x40021018 | APB2 外设时钟使能（**最常用**）|
| `RCC_APB1ENR` | 0x4002101C | APB1 外设时钟使能 |

### 常用 APB2 外设使能位（RCC_APB2ENR）

| 位 | 外设 |
|---|---|
| bit 2 | IOPA（PA 端口时钟）|
| bit 3 | IOPB |
| bit 11 | IOPD |
| bit 14 | USART1 |
| bit 12 | SPI1 |
| bit 0 | AFIO（复用功能）|

### 时钟树（HSE 8MHz → 72MHz 系统时钟）

```
HSE 8MHz ─┐
          ├──PLL ×9──SYSCLK 72MHz──┬──AHB 72MHz──┬──APB1 36MHz (÷2)── 36MHz
HSI 8MHz ─┘                        │             └──APB2 72MHz (÷1)── 72MHz
                                   └──SysTick
```

配置流程（B.3 FreeRTOS 时配）：
1. `RCC_CR |= RCC_CR_HSEON` 开 HSE
2. 等待 `RCC_CR & RCC_CR_HSERDY`
3. `RCC_CFGR |= RCC_CFGR_PLLSRC` 选 HSE 为 PLL 输入
4. `RCC_CFGR |= RCC_CFGR_PLLMULL9` 设 ×9
5. `RCC_CR |= RCC_CR_PLLON` 开 PLL
6. 等待 `RCC_CR & RCC_CR_PLLRDY`
7. `RCC_CFGR |= RCC_CFGR_SW_PLL` 切换 SYSCLK 到 PLL
8. 等待 `RCC_CFGR & RCC_CFGR_SWS_PLL`
9. 配置 `RCC_CFGR` 的 HPRE / PPRE1 / PPRE2 分频
10. 设置 `FLASH_ACR` 的 LATENCY = 2（72MHz 必须）

## 第 9 章 GPIO

### 9.2 GPIO 引脚配置（CRL/CRH 寄存器）

每个引脚 4 位：`CNF[1:0] MODE[1:0]`

| MODE | 含义 |
|---|---|
| 00 | 输入 |
| 01 | 输出 10MHz |
| 10 | 输出 2MHz |
| 11 | 输出 50MHz |

| CNF（输出） | 含义 |
|---|---|
| 00 | 推挽输出 |
| 01 | 开漏输出 |
| 10 | AF 推挽（**USART TX / SPI 用**） |
| 11 | AF 开漏 |

| CNF（输入） | 含义 |
|---|---|
| 00 | 模拟输入 |
| 01 | 浮空输入（**USART RX 用**） |
| 10 | 上拉/下拉输入 |
| 11 | 保留 |

### USART1 引脚配置（PA9 TX, PA10 RX）

```c
/* PA9: CNF=10 (AF PP), MODE=11 (50MHz) → 4 位值 0xB */
GPIOA_CRH = (GPIOA_CRH & ~(0xF << 4)) | (0xB << 4);

/* PA10: CNF=01 (floating), MODE=00 (input) → 4 位值 0x4 */
GPIOA_CRH = (GPIOA_CRH & ~(0xF << 8)) | (0x4 << 8);
```

## 第 25 章 USART

### 25.3.4 波特率计算

```
USARTDIV = fck / (16 × baud)
BRR[15:4] = mantissa = USARTDIV 整数部分
BRR[3:0]  = fraction = (USARTDIV - mantissa) × 16
```

**例**：fck = 72MHz, baud = 115200
```
USARTDIV = 72000000 / (16 × 115200) = 39.0625
mantissa = 39
fraction = 0.0625 × 16 = 1
BRR = (39 << 4) | 1 = 0x271
```

### 25.6 USART 寄存器

| 寄存器 | 地址偏移 | 用途 |
|---|---|---|
| `USART_SR` | 0x00 | 状态（TXE / TC / RXNE / ORE） |
| `USART_DR` | 0x04 | 数据 |
| `USART_BRR` | 0x08 | 波特率 |
| `USART_CR1` | 0x0C | 控制（UE / TE / RE / M / PCE）|
| `USART_CR2` | 0x10 | 控制（STOP 位）|
| `USART_CR3` | 0x14 | 控制（DMA / 中断）|

### USART_CR1 关键位

| 位 | 名称 | 含义 |
|---|---|---|
| 13 | UE | USART 使能（**必须最后开**） |
| 3  | TE | 发送使能 |
| 2  | RE | 接收使能 |
| 12 | M | 字长（0=8位，1=9位） |
| 10 | PCE | 校验使能 |

### USART_SR 关键位

| 位 | 名称 | 含义 |
|---|---|---|
| 7 | TXE | 发送数据寄存器空（**可写入下一个字符**）|
| 6 | TC | 发送完成（最后一字节移位完成） |
| 5 | RXNE | 接收数据寄存器非空 |

### 发送一个字符的标准流程

```c
while (!(USART1->SR & USART_SR_TXE));   /* 等待发送缓冲区空 */
USART1->DR = (uint32_t)ch;                /* 写入字符 */
```

### 接收一个字符的标准流程

```c
while (!(USART1->SR & USART_SR_RXNE));   /* 等待接收缓冲区非空 */
uint8_t ch = (uint8_t)USART1->DR;         /* 读出字符（同时清 RXNE） */
```

## 章节索引（B.2 / B.3 用）

| 外设 | RM0008 章节 | 关键寄存器 |
|---|---|---|
| GPIO | Ch 9 | CRL / CRH / IDR / ODR / BSRR |
| RCC | Ch 7 | CR / CFGR / APB2ENR / APB1ENR |
| USART | Ch 25 | SR / DR / BRR / CR1-3 |
| SPI | Ch 23 | CR1 / SR / DR |
| I2C | Ch 24 | CR1 / CR2 / SR1 / DR |
| TIM | Ch 15 | CR1 / DIER / SR / CNT / PSC / ARR |
| ADC | Ch 11 | CR1 / CR2 / SR / DR / SQR1-3 |
| EXTI | Ch 10 | IMR / FTSR / RTSR / PR / SWIER |
| NVIC | Ch 12 | ISER / ICER / ISPR / ICPR / IP |
| DMA | Ch 13 | CCR / CNDTR / CPAR / CMAR |

## 推荐阅读顺序

1. Ch 7 RCC → Ch 9 GPIO → Ch 25 USART（B.1 hello_uart）
2. Ch 25 USART 中断部分（B.2.1 DHT11 单总线，需要 EXTI）
3. Ch 23 SPI + Ch 24 I2C（B.2.2 MPU6050 + B.2.3 NRF24）
4. Ch 15 TIM（B.3 FreeRTOS SysTick + B.4 协议定时器）
5. Ch 12 NVIC + Ch 10 EXTI（B.3 FreeRTOS 中断优先级）

> 完整 RM0008 PDF 在仓库根目录或 ST 官网下载。

## BNO055（I2C 设备，非 RM0008 内容）

> GY-BNO055 是 Bosch 九轴传感器，**自带 ARM Cortex-M0+ 跑融合算法**。
> 通过 I2C 通信，STM32F103 用 I2C1（PB6=SCL, PB7=SDA）。
> 不在 RM0008 内，需要看 BNO055 datasheet：https://www.bosch-sensortec.com/products/smart-sensors/bno055/

### BNO055 关键特性

| 特性 | 值 |
|---|---|
| 轴数 | 9（加速度 + 陀螺仪 + 磁力计）|
| 融合算法 | 板载（无需自己写）|
| 输出 | 欧拉角 / 四元数 / 旋转向量 / 重力向量 |
| 默认 I2C 地址 | **0x28**（ADR 引脚接 GND）|
| 备用 I2C 地址 | 0x29（ADR 引脚接 VDD）|
| 工作电压 | 3.3V（GY 模块板上自带稳压）|
| 默认 I2C 时钟 | 400 kHz（Fast Mode）|

### BNO055 关键寄存器（datasheet §4）

| 寄存器地址 | 名称 | 说明 |
|---|---|---|
| 0x00 | CHIP_ID | 应读出 0xA0（识别用）|
| 0x07 | EUL_PITCH_MSB | 俯仰角（pitch）高字节 |
| 0x08 | EUL_ROLL_MSB | 横滚角（roll）高字节 |
| 0x09 | EUL_HEADING_MSB | 航向角（yaw）高字节 |
| 0x3D | OPR_MODE | 操作模式（NDOF = 0x0C = 九轴融合）|
| 0x3B | PWR_MODE | 电源模式（0x00 = Normal）|
| 0x3E | SYS_TRIGGER | 系统触发（含复位位）|

### BNO055 I2C 读欧拉角流程

```c
// 1. 探测设备（HAL_I2C_IsDeviceReady 或自己写）
uint8_t who_am_i;
HAL_I2C_Mem_Read(&hi2c1, 0x28<<1, 0x00, I2C_MEMADD_SIZE_8BIT, &who_am_i, 1, 100);
// 期望：0xA0

// 2. 配置为 NDOF 模式（九轴融合）
uint8_t mode = 0x0C;
HAL_I2C_Mem_Write(&hi2c1, 0x28<<1, 0x3D, I2C_MEMADD_SIZE_8BIT, &mode, 1, 100);

// 3. 读欧拉角（每 100ms 一次）
uint8_t buf[6];
HAL_I2C_Mem_Read(&hi2c1, 0x28<<1, 0x1A, I2C_MEMADD_SIZE_8BIT, buf, 6, 100);
// buf[0..1] = heading (yaw), buf[2..3] = roll, buf[4..5] = pitch
// 单位：1/16 度（即 raw_value / 16.0 = 度）
```

### BNO055 vs MPU6050 简历写法对比

```
MPU6050（普通）："通过 I2C 读取 MPU6050 加速度与陀螺仪原始数据，应用卡尔曼滤波融合姿态"
BNO055（进阶）："通过 I2C 配置 GY-BNO055 九轴传感器，应用板载 Bosch 传感器融合算法，
                  直接读取欧拉角（含磁力计校准，无累计漂移）"
```

> **用 BNO055 的简历含金量高于 MPU6050**：面试官会问"为什么选 BNO055 不选 MPU6050"，你能答出"省去融合算法开发"和"磁力计校准"就是亮点。
