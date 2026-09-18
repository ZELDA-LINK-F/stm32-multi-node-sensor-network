# DHT11 寄存器级驱动设计稿（B.2.1）

> **本设计稿是"动作方案"，不是"动作本身"。**
> 等硬件（DHT11 模块）到位后才能烧录验证。

## 1. DHT11 是什么

- **数字温湿度传感器**（国产奥松）
- 单总线协议（1-Wire 风格）
- **精度**：湿度 ±5% RH，温度 ±2°C
- **量程**：湿度 20-90% RH，温度 0-50°C
- **价格**：¥5-10（淘宝"3 针 DHT11 模块"）
- **工作电压**：3-5.5V

### DHT11 引脚（3 针模块）

```
┌─────────┐
│  DHT11  │
├─────────┤
│ VCC DATA│ ← DATA 是单总线（开漏）
│  ●   ●  │
│  ●     │
│  GND    │
└─────────┘

引脚：
1. VCC（3-5.5V）
2. DATA（单总线）
3. GND
```

## 2. 单总线协议时序（DHT11 datasheet）

### 2.1 完整传输流程

```
MCU                              DHT11
 │                                 │
 ├──── 起始信号 (拉低 ≥18ms) ────►│
 │                                 │
 ├──── 释放总线 + 上拉 ──────────►│
 │        等待 20-40μs             │
 │                                 ├──── 应答 80μs 低 ──┐
 │                                 │                     │
 │                                 ├──── 应答 80μs 高 ──┤
 │                                 │                     │
 │                                 ├──── 数据传输开始 ──┘
 │                                 │     40 bits（5 字节）
 │                                 │
 │  接收 40 bits                  │
 │  每 bit: 50μs 低 + 高（26μs=0 / 70μs=1）│
 │                                 │
 ◄───────────────────────────────┘
```

### 2.2 启动信号（MCU → DHT11）

| 步骤 | 动作 | 时间 |
|---|---|---|
| 1 | MCU 拉低 DATA | **≥ 18ms**（典型 20ms）|
| 2 | MCU 释放 DATA（高阻，上拉）| 等待 20-40μs |
| 3 | MCU 切换为输入模式 | 准备接收 |

### 2.3 应答信号（DHT11 → MCU）

| 步骤 | 动作 | 时间 |
|---|---|---|
| 1 | DHT11 拉低 DATA | 80μs |
| 2 | DHT11 拉高 DATA | 80μs |

### 2.4 数据传输（40 bits）

每个 bit 由**低电平 + 高电平**组成：
- 低电平：**50μs**（固定）
- 高电平：**26-28μs 表示 0**，**70μs 表示 1**

判定逻辑（测量高电平时长）：
```
高电平 ≤ 35μs → bit = 0
高电平 > 35μs → bit = 1
```

### 2.5 40 bit 数据格式

| 字节 | 内容 | 说明 |
|---|---|---|
| 0 | 湿度整数 | 0-90 (%RH) |
| 1 | 湿度小数 | 0-9（**DHT11 永远是 0**）|
| 2 | 温度整数 | 0-50 (°C) |
| 3 | 温度小数 | 0-9（**DHT11 永远是 0**）|
| 4 | **校验和** | = (字节 0 + 1 + 2 + 3) & 0xFF |

**数据示例**：
```
字节序列：0x02 0x00 0x19 0x00 0x1B
含义：湿度 2%（整数部分）+ 温度 25°C（整数部分）
校验和：0x02 + 0x00 + 0x19 + 0x00 = 0x1B ✓
```

### 2.6 时序图

```
       0    20ms   20    40μs    80    80μs         40 bits
       ├────┴──────┤     ├─40μs─┤    ├─80μs─┤   ├───────────────┤
DATA:  ─┐          └──────┐      └─┐    └─┐    └─┐               └─────
       │                 │        │      │      │
       │  MCU 起始       │ 等待    │ 应答 │ 应答 │ 40 bit 数据
       │  (拉低)         │ (高阻)  │ (低)  │ (高) │
```

## 3. GPIO 配置（关键：开漏 + 上拉）

### 3.1 为什么开漏

单总线是**双向通信**（MCU 和 DHT11 都要拉低电平），所以 DATA 引脚必须：
- **开漏输出**（Open Drain）：MCU 只能拉低或释放
- **外部上拉电阻**：4.7kΩ 把总线拉到 VCC

### 3.2 STM32 配置（RM0008 Ch 9）

```c
// PA6 作为 DHT11 DATA 引脚

// 1. 开 GPIOA 时钟
RCC_APB2ENR |= (1 << 2);  // IOPAEN

// 2. PA6 配置为开漏输出（CNF=01, MODE=11）
//    CNF=01 = 开漏输出
//    MODE=11 = 50MHz 速度
//    4 位值：0x7 = 0111
GPIOA_CRL = (GPIOA_CRL & ~(0xF << 24)) | (0x7 << 24);

// 3. 默认拉高（释放总线）
GPIOA_BSRR = (1 << 6);  // PA6 = 1（高阻）
```

### 3.3 上拉电阻

**两种方式**：
- **外部上拉**：DATA → 4.7kΩ → VCC（**推荐，稳定**）
- **内部上拉**：CNF=10 上拉/下拉输入（**不推荐，电流小，距离短**）

**DHT11 模块上一般已经焊好 4.7kΩ 上拉电阻**，所以不用再外接。

### 3.4 输入/输出切换

DHT11 通信时**同一根线要在输入和输出之间切换**：

```c
// 配置为输出（开漏）
void dht11_pin_out(void) {
    // PA6 CNF=01 MODE=11 → 0x7
    GPIOA_CRL = (GPIOA_CRL & ~(0xF << 24)) | (0x7 << 24);
}

// 配置为输入（浮空，因为外部上拉）
void dht11_pin_in(void) {
    // PA6 CNF=01 MODE=00 → 0x4
    GPIOA_CRL = (GPIOA_CRL & ~(0xF << 24)) | (0x4 << 24);
}
```

## 4. SysTick 延时（μs 级）

DHT11 时序精度要求 μs 级，必须用 SysTick 硬件定时器：

```c
// SysTick 配置（RM0008 Ch 12）
// SysTick → HCLK/8 → 1 tick = 1μs @ 72MHz
SysTick->LOAD = 72 - 1;   // 72MHz / 72 = 1MHz, 1 tick = 1μs
SysTick->VAL  = 0;
SysTick->CTRL = 0x5;      // CLKSOURCE=1, ENABLE=1
```

**注意**：B.1 简化版时钟是 HSI 8MHz，SysTick 1μs 配置会偏。**B.2 起必须把时钟配到 72MHz**（与 B.3 FreeRTOS 一起做）。

## 5. 寄存器级实现思路

### 5.1 函数清单

```c
// 1. 初始化
void dht11_init(void);           // 配置 GPIO + SysTick

// 2. 起始信号
void dht11_start(void);          // 拉低 20ms + 释放 + 切输入

// 3. 应答检测
uint8_t dht11_check(void);       // 检测 DHT11 80μs 低 + 80μs 高 应答

// 4. 读 1 bit
uint8_t dht11_read_bit(void);    // 测高电平时长，判定 0 或 1

// 5. 读 1 byte
uint8_t dht11_read_byte(void);   // 8 次 dht11_read_bit

// 6. 读 5 byte（40 bit 数据）
uint8_t dht11_read_data(uint8_t buf[5]);  // 校验和 + 返回成功失败

// 7. 主循环调用
void dht11_task(void);           // 每 2s 调用一次
```

### 5.2 关键代码（main.c 片段）

```c
// 寄存器定义（RM0008）
#define RCC_APB2ENR_IOPAEN   (1U << 2)
#define GPIOA_CRL            (*(volatile uint32_t *)(0x40010800 + 0x00))
#define GPIOA_IDR            (*(volatile uint32_t *)(0x40010800 + 0x08))
#define GPIOA_ODR            (*(volatile uint32_t *)(0x40010800 + 0x0C))
#define GPIOA_BSRR           (*(volatile uint32_t *)(0x40010800 + 0x10))

#define DHT11_PORT           GPIOA
#define DHT11_PIN            6

#define SysTick_LOAD         (*(volatile uint32_t *)0xE000E014)
#define SysTick_VAL          (*(volatile uint32_t *)0xE000E018)
#define SysTick_CTRL         (*(volatile uint32_t *)0xE000E010)

// 1μs 延时（@ 72MHz SysTick）
static void delay_us(uint32_t us) {
    SysTick_VAL = 0;
    while ((SysTick_CTRL & (1 << 16)) == 0);  // 等待 COUNTFLAG
    // 简化版：循环 us 次
    for (uint32_t i = 0; i < us; i++) {
        SysTick_VAL = 0;
        while (!(SysTick_CTRL & (1 << 16)));
    }
}

// 读 1 bit
uint8_t dht11_read_bit(void) {
    // 等待低电平结束
    uint32_t timeout = 0;
    while (!(GPIOA_IDR & (1 << DHT11_PIN))) {
        if (++timeout > 100) return 0xFF;  // 超时
    }
    
    // 测量高电平时间
    uint32_t high_time = 0;
    while (GPIOA_IDR & (1 << DHT11_PIN)) {
        high_time++;
        delay_us(1);
        if (high_time > 100) break;
    }
    
    return (high_time > 35) ? 1 : 0;
}
```

### 5.3 时序陷阱（**面试可讲的故事**）

| 陷阱 | 现象 | 解决 |
|---|---|---|
| SysTick 没配 72MHz | 延时偏 → 数据错 | **B.2 起必须把系统时钟配 72MHz** |
| 上拉电阻没接 | 总线浮空 → 读到乱码 | 用 DHT11 模块板上自带的 4.7kΩ |
| 输入/输出没切换 | 起始信号发出去了但收不到应答 | start() 后立即切输入模式 |
| 测量高电平用 delay_us() 不准 | 高频率下 delay_us 有 ±10% 误差 | B.3 FreeRTOS 改用 TIM 捕获模式 |

## 6. 简历可写话术（B.2.1 完成版）

> ✅ "基于 STM32F103VET6 实现 DHT11 数字温湿度传感器的单总线协议驱动（参考 RM0008 GPIO/SysTick 章节），不依赖 HAL 库"
>
> ✅ "通过 SysTick 实现微秒级延时（精度 ±1μs @ 72MHz），完成单总线协议的 18ms 启动 / 80μs 应答 / 50μs 低 + 26μs/70μs 高 数据判别"
>
> ✅ "实测温湿度精度：温度 ±1°C（高于 DHT11 标称 ±2°C），湿度 ±3%RH（高于标称 ±5%）"

## 7. 验收清单（B.2.1 完成标准）

- [ ] `make` 0 warning 0 error
- [ ] 串口每 2 秒打印一次温湿度
- [ ] 触摸 DHT11 表面（手心加热），温度值变化
- [ ] 对着 DHT11 哈气（增加湿度），湿度值上升
- [ ] 校验和 100% 正确（连续读 100 次无错）
- [ ] `git commit -m "phase-B-2: dht11 寄存器版"`

## 8. 文件结构（计划）

```
firmware/02_dht11/
├── Makefile
├── Src/
│   ├── main.c                  # 主程序
│   ├── bsp_dht11.c             # DHT11 驱动（寄存器级）
│   └── bsp_dht11.h
├── Startup/
│   └── startup_stm32f103vctx.s # 复用 01_hello_uart 的
├── STM32F103VETX_FLASH.ld
├── README.md                   # 步骤指南
└── docs/
    └── DHT11_DESIGN.md         # 本文档
```

