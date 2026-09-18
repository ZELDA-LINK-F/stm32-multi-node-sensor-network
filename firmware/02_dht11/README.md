# 02_dht11 — DHT11 寄存器级驱动（B.2.1）

> **状态**：🟡 设计稿已就绪（`docs/DHT11_DESIGN.md`），等硬件 + 72MHz 时钟配置。
>
> **编译**：✅ 已通过（占位代码，串口循环打印"DHT11 not connected yet"）

## 简历可写

> ✅ "基于 STM32F103VET6 实现 DHT11 数字温湿度传感器的单总线协议驱动（参考 RM0008 GPIO/SysTick 章节），不依赖 HAL 库"

## 设计稿

详见 [`docs/DHT11_DESIGN.md`](docs/DHT11_DESIGN.md)（295 行），包含：

- DHT11 单总线协议完整时序图
- 40 bit 数据格式 + 校验和算法
- GPIO 配置（开漏 + 上拉）
- SysTick 1μs 延时实现
- 寄存器级驱动函数清单（`dht11_init` / `start` / `check` / `read_bit` / `read_byte` / `read_data`）
- 时序陷阱 + 面试故事

## 编译验证

```bash
cd firmware/02_dht11
make
# 期望：build/dht11.elf（text < 2KB）
```

## 烧录步骤（等硬件）

```bash
# 1. VET6 指南者接 USB（自带 ST-Link）
# 2. PA6 接 DHT11 DATA（模块自带 4.7kΩ 上拉）
# 3. VCC 接 3.3V，GND 接 GND
make flash
```

## 验收清单（B.2.1 完成）

- [ ] `make` 0 warning
- [ ] 串口每 2 秒打印 "T=25 H=60" 格式（手捂温度上升）
- [ ] 校验和 100 次无错
- [ ] 触摸 DHT11 表面 → 温度值变化
- [ ] 对着哈气 → 湿度值上升
- [ ] `git commit -m "phase-B-2: dht11 寄存器版"`

## 文件结构

```
02_dht11/
├── Makefile
├── Src/main.c              # 主程序 + 占位 DHT11 驱动
├── Startup/startup_stm32f103vctx.s
├── STM32F103VETX_FLASH.ld
├── README.md               # 本文件
└── docs/
    └── DHT11_DESIGN.md     # 设计稿（295 行）
```

## 启动顺序（B.2 完整流程）

```
W3 Day1: 本设计稿 review ✓
W3 Day2: DHT11 硬件到位
W3 Day3: 实现 dht11_read_data()（参考 DHT11_DESIGN.md §5）
W3 Day4: 编译 + 烧录 + 串口验证
W3 Day5: 触摸测试 + 哈气测试 + commit
```

## 风险与陷阱

| 风险 | 后果 | 解决 |
|---|---|---|
| 时钟不是 72MHz | SysTick 1μs 偏 9 倍 | **B.3 FreeRTOS 时一起配** |
| PA6 没接上拉电阻 | 总线浮空 | 用 DHT11 模块（板上已有 4.7kΩ）|
| 读位超时 | 通信失败 | 加 timeout 计数（已写在代码）|
| 主循环太长 | 错过应答 | DHT11 时序严格，**不能用 RTOS 任务切换**（B.3 时单独处理）|
