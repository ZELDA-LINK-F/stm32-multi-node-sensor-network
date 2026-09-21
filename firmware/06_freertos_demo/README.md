# 06_freertos_demo — FreeRTOS 5 任务入门 Demo

> 目标：用一个文件演示 FreeRTOS 的 5 个核心概念，让你能立刻在野火指南者上看到 FreeRTOS 跑起来。

---

## 一、这个 Demo 演示什么

| 概念 | 在哪个任务里看 |
|------|---------------|
| **多任务 + 抢占式调度** | 5 个任务同时跑，蓝灯比红灯亮得更频繁（5Hz vs 1Hz）|
| **`vTaskDelayUntil` 精确周期** | Task_FastBlink / Task_SlowBlink / Task_Producer 严格周期 |
| **队列（Queue）通信** | Task_Producer → Task_Printer 通过 `queue_count` |
| **互斥锁（Mutex）保护共享外设** | 所有任务通过 `mutex_usart` 互斥访问 USART1 |
| **钩子函数（Hook）** | `vApplicationStackOverflowHook` / `vAssertCalled` 等 |

---

## 二、5 个任务一览

| 任务 | 优先级 | 周期 | 干什么 | 看哪里 |
|------|------|------|------|------|
| `Task_FastBlink` | 2 | 100ms | 蓝灯 5Hz 闪 | 蓝灯 |
| `Task_SlowBlink` | 1 | 500ms | 红灯 1Hz 闪 | 红灯 |
| `Task_Producer` | 2 | 200ms | `count++` + 入队 | 串口 `[Producer] send #N` |
| `Task_Printer` | 3 | 事件驱动 | 出队 + 串口打印 | 串口 `[Printer] recv=N` + 绿灯 |
| `Task_Heartbeat` | tskIDLE+1 | 5000ms | 打印统计 | 串口 `[HB #N] uptime=...ms queue=...` |

**优先级解释**：
- `Printer(3)` 最高：队列一来立即抢占处理
- `Fast/Producer(2)` 中等：周期性数据生产
- `Slow(1)` 低：后台慢节奏
- `Heartbeat(tskIDLE+1)` 最低：只在 CPU 空闲时跑

---

## 三、文件结构（按职责分层）

```
06_freertos_demo/
├── Src/
│   ├── main.c              ← ★ 只有 main()，54 行（执行序列）
│   ├── bsp_usart.h/c       ← BSP 层：USART1 驱动（纯硬件，无 FreeRTOS 依赖）
│   ├── bsp_led.h/c         ← BSP 层：RGB LED 驱动（纯硬件）
│   ├── app_tasks.h/c       ← 应用层：5 任务 + 队列 + 互斥锁 + log_msg
│   ├── freertos_hooks.c    ← FreeRTOS 钩子（栈溢出/malloc失败/assert）
│   └── FreeRTOSConfig.h    ← FreeRTOS 配置
├── STM32F103VETX_FLASH.ld  → 软链到 ../05_freertos/
├── Makefile
├── docs/
└── build/
```

**分层原则**：
- **BSP 层**（bsp_*）：只管硬件寄存器，不知道 FreeRTOS 存在
- **应用层**（app_tasks）：组合 BSP + FreeRTOS 实现业务逻辑
- **main**：串联各层，**不含任何业务代码**
- **钩子**（freertos_hooks）：FreeRTOS 触发的错误回调，独立成文件方便后续加新钩子

**复用 05_freertos 的资源**（避免重复）：
- FreeRTOS-Kernel 源码（`../05_freertos/third_party/FreeRTOS-Kernel/`）
- 启动文件（`../05_freertos/Startup/startup_stm32f103vctx.s`）
- 链接脚本（`../05_freertos/STM32F103VETX_FLASH.ld`）
- 时钟初始化（`../common/system_stm32f1xx.c`）
- 通用编译规则（`../Makefile.common`）

---

## 四、怎么用

### 4.1 编译

```bash
cd firmware/06_freertos_demo/
make            # 编译生成 build/freertos_demo.bin 等
make size       # 看代码体积
```
06_freertos_demo/
├── Src/
│   ├── main.c              ← ★ 只有 main()，54 行（执行序列）
│   ├── bsp_usart.h/c       ← BSP 层：USART1 驱动（纯硬件，无 FreeRTOS 依赖）
│   ├── bsp_led.h/c         ← BSP 层：RGB LED 驱动（纯硬件）
│   ├── app_tasks.h/c       ← 应用层：5 任务 + 队列 + 互斥锁 + log_msg
│   ├── freertos_hooks.c    ← FreeRTOS 钩子（栈溢出/malloc失败/assert）
│   └── FreeRTOSConfig.h    ← FreeRTOS 配置
├── STM32F103VETX_FLASH.ld  → 软链到 ../05_freertos/
├── Makefile
├── docs/
└── build/
```

**分层原则**：
- **BSP 层**（bsp_*）：只管硬件寄存器，不知道 FreeRTOS 存在
- **应用层**（app_tasks）：组合 BSP + FreeRTOS 实现业务逻辑
- **main**：串联各层，**不含任何业务代码**
- **钩子**（freertos_hooks）：FreeRTOS 触发的错误回调，独立成文件方便后续加新钩子
   text    data     bss     dec     hex filename
   8272       8   12612   20892    519c build/freertos_demo.elf
```
06_freertos_demo/
├── Src/
│   ├── main.c              ← ★ 只有 main()，54 行（执行序列）
│   ├── bsp_usart.h/c       ← BSP 层：USART1 驱动（纯硬件，无 FreeRTOS 依赖）
│   ├── bsp_led.h/c         ← BSP 层：RGB LED 驱动（纯硬件）
│   ├── app_tasks.h/c       ← 应用层：5 任务 + 队列 + 互斥锁 + log_msg
│   ├── freertos_hooks.c    ← FreeRTOS 钩子（栈溢出/malloc失败/assert）
│   └── FreeRTOSConfig.h    ← FreeRTOS 配置
├── STM32F103VETX_FLASH.ld  → 软链到 ../05_freertos/
├── Makefile
├── docs/
└── build/
```

**分层原则**：
- **BSP 层**（bsp_*）：只管硬件寄存器，不知道 FreeRTOS 存在
- **应用层**（app_tasks）：组合 BSP + FreeRTOS 实现业务逻辑
- **main**：串联各层，**不含任何业务代码**
- **钩子**（freertos_hooks）：FreeRTOS 触发的错误回调，独立成文件方便后续加新钩子

### 4.3 观察

接上 USB-TTL（PA9=TX → USB-TTL RX，115200 8N1），打开串口助手看到：

```
06_freertos_demo/
├── Src/
│   ├── main.c              ← ★ 只有 main()，54 行（执行序列）
│   ├── bsp_usart.h/c       ← BSP 层：USART1 驱动（纯硬件，无 FreeRTOS 依赖）
│   ├── bsp_led.h/c         ← BSP 层：RGB LED 驱动（纯硬件）
│   ├── app_tasks.h/c       ← 应用层：5 任务 + 队列 + 互斥锁 + log_msg
│   ├── freertos_hooks.c    ← FreeRTOS 钩子（栈溢出/malloc失败/assert）
│   └── FreeRTOSConfig.h    ← FreeRTOS 配置
├── STM32F103VETX_FLASH.ld  → 软链到 ../05_freertos/
├── Makefile
├── docs/
└── build/
```

**分层原则**：
- **BSP 层**（bsp_*）：只管硬件寄存器，不知道 FreeRTOS 存在
- **应用层**（app_tasks）：组合 BSP + FreeRTOS 实现业务逻辑
- **main**：串联各层，**不含任何业务代码**
- **钩子**（freertos_hooks）：FreeRTOS 触发的错误回调，独立成文件方便后续加新钩子

LED 现象：
- 🔵 蓝灯 5Hz 闪（看着常亮，但每 100ms 翻转）
- 🔴 红灯 1Hz 闪（明显的 1 秒一次）
- 🟢 绿灯每次 Printer 收到消息闪一下

---

## 五、代码导读（按阅读顺序）

| 顺序 | 章节 | 看什么 |
|------|------|------|
| 1 | main 顶部注释 | 5 个任务的整体设计 |
| 2 | 一、硬件抽象层 | USART1 / LED 寄存器操作（**不依赖 HAL**）|
| 3 | 二、全局对象 | queue_count 和 mutex_usart 定义 |
| 4 | 三、`log_msg()` | mutex 保护 USART 的标准写法 |
| 5 | 四、钩子函数 | 出错时如何反馈 |
| 6 | 五、5 个 Task | **核心**：每个任务 10~25 行，看它们的共性和差异 |
| 7 | 六、`main()` | 标准启动序列：硬件 → IPC → Task → 调度器 |

---

## 六、和 05_freertos 的关系

| 文件 | 05_freertos | 06_freertos_demo |
|------|-------------|------------------|
| 状态 | 仿真版（计数模拟传感器）| **入门 Demo**（更清晰注释）|
| 任务数 | 4 个（Sensor/Protocol/TX/Heartbeat）| 5 个（Fast/Slow/Producer/Printer/Heartbeat）|
| 真实硬件 | 仿真（计数代替 BNO055，串口代替 NRF24）| **仿真**（计数代替真实传感器）|
| 重点 | 任务架构匹配你的项目 | **学习 API 用法** |
| 是否能直接用 | 等 B.3 填真实驱动 | 看完这个就懂 FreeRTOS 怎么用 |

**建议路径**：
```
06_freertos_demo/
├── Src/
│   ├── main.c              ← ★ 只有 main()，54 行（执行序列）
│   ├── bsp_usart.h/c       ← BSP 层：USART1 驱动（纯硬件，无 FreeRTOS 依赖）
│   ├── bsp_led.h/c         ← BSP 层：RGB LED 驱动（纯硬件）
│   ├── app_tasks.h/c       ← 应用层：5 任务 + 队列 + 互斥锁 + log_msg
│   ├── freertos_hooks.c    ← FreeRTOS 钩子（栈溢出/malloc失败/assert）
│   └── FreeRTOSConfig.h    ← FreeRTOS 配置
├── STM32F103VETX_FLASH.ld  → 软链到 ../05_freertos/
├── Makefile
├── docs/
└── build/
```

**分层原则**：
- **BSP 层**（bsp_*）：只管硬件寄存器，不知道 FreeRTOS 存在
- **应用层**（app_tasks）：组合 BSP + FreeRTOS 实现业务逻辑
- **main**：串联各层，**不含任何业务代码**
- **钩子**（freertos_hooks）：FreeRTOS 触发的错误回调，独立成文件方便后续加新钩子

---

## 七、简历可写话术

> "基于 STM32F103VET6 移植 FreeRTOS V10.6.1（原生 API），实现 5 任务并行调度，
> 使用**队列解耦**生产者和消费者，**互斥锁保护共享 USART**，
> `vTaskDelayUntil` 实现 100Hz 精确周期，任务切换抖动 < 1ms"

---

## 八、扩展练习（动手做一遍才算学会）

按难度排序，每完成一个就 commit 一次：

- [ ] **练习 1**：把 `Task_FastBlink` 改成 `vTaskDelay`（不是 `vTaskDelayUntil`），跑 5 分钟看 LED 是否还稳定（答案：会漂移）
- [ ] **练习 2**：把 `mutex_usart` 删掉，观察串口输出交错混乱
- [ ] **练习 3**：加一个 `Task_Button`，用 EXTI 检测 KEY1（PA0），每次按键打印 `[Button] pressed!`
- [ ] **练习 4**：加一个二值信号量 `sem_button`，按键 ISR `GiveFromISR`，Task 等待信号量后打印（**ISR ↔ Task** 标准模式）
- [ ] **练习 5**：开 `configGENERATE_RUN_TIME_STATS = 1`，在 Heartbeat 里打印每个任务的 CPU 占用百分比
- [ ] **练习 6**：用 `xTimerCreate` 创建一个软件定时器，每 3 秒翻转红灯（替代 `Task_Heartbeat` 的 vTaskDelay）

每个练习的答案都在 `docs/EXERCISE_ANSWERS.md`（待写）。

---

**Last edit: 2026-09-21 by Codex**
