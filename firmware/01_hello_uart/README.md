# 01_hello_uart — Phase B.1 第一动作

> **目标**：串口每 1 秒打印一行 `Hello UART! count=N`，验证 STM32F103VET6 + HAL + GCC + st-flash 全链路通。

## 步骤

### Step 1：用 CubeMX 生成工程

1. 启动 `STM32CubeMX`
2. **File → New Project** → 选 `STM32F103VET6`（LQFP100 封装）
3. **Pinout & Configuration**：
   - `USART1`：Mode = Asynchronous，Baud = 115200，Word = 8，Parity = None，Stop = 1
   - 引脚：`PA9` = USART1_TX，`PA10` = USART1_RX
   - `RCC`：HSE = Crystal/Ceramic Resonator（8 MHz）
   - `SYS`：Debug = Serial Wire（SWD），Timebase = TIM1
4. **Clock Configuration**：HCLK = 72 MHz（CubeMX 会自动算）
5. **Project Manager**：
   - Project Name: `hello_uart`
   - Project Location: `/home/zelda/learn/iot-web/秋招备战/stm32_sensor_net/firmware/01_hello_uart/`
   - Toolchain/IDE: **Makefile**（不是 STM32CubeIDE！我们要 GCC）
   - ☑ Generate peripheral initialization as a pair of .c/.h files per peripheral
   - ☑ Add linker script
6. **GENERATE CODE**

### Step 2：写 main.c 业务代码

打开 `Src/main.c`，在 `/* USER CODE BEGIN 2 */` 和 `/* USER CODE END 2 */` 之间加：

```c
/* USER CODE BEGIN 2 */
uint32_t count = 0;
char msg[64];
/* USER CODE END 2 */
```

在 `while (1)` 循环里加：

```c
/* USER CODE BEGIN 3 */
HAL_Delay(1000);
count++;
int n = snprintf(msg, sizeof(msg), "Hello UART! count=%lu\r\n", count);
HAL_UART_Transmit(&huart1, (uint8_t *)msg, n, HAL_MAX_DELAY);
/* USER CODE END 3 */
```

### Step 3：编译

```bash
cd firmware/01_hello_uart
make
# 期望输出：build/hello_uart.elf + .bin + .hex
```

### Step 4：烧录

插上 ST-Link → VET6 板：

```bash
bash flash.sh
# 或：make flash
```

### Step 5：验收

打开串口助手（`minicom -D /dev/ttyUSB0 -b 115200` 或 MobaXterm/PuTTY）：

```
Hello UART! count=1
Hello UART! count=2
...
```

每行 1 秒一次，**60 秒无乱码、无丢行** = Phase B.1 done。

## 验收清单

- [ ] `make` 0 warning
- [ ] `st-info --probe` 找到 VET6
- [ ] `flash.sh` 烧录成功
- [ ] 串口稳定输出 60 秒
- [ ] `phase-B-1: hello uart` 已 commit

## 故障排查

| 现象 | 原因 | 解决 |
|---|---|---|
| 串口乱码 | 时钟树错 / 波特率错 | CubeMX 重新检查 HCLK=72M，USART1 时钟源 = PCLK2 |
| 烧录失败"init mode failed" | SWD 引脚被占用 | CubeMX 关闭 USART1 重映射的 SWD 引脚 |
| 没找到 ttyUSB0 | USB 串口芯片驱动 | 多数 CH340/CP2102 Ubuntu 内核自带 |
| HAL_Delay 卡死 | SysTick 没起 | CubeMX 把 SysTick 设 Timebase source |
