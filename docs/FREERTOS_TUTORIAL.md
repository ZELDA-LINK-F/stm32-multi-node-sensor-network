# FreeRTOS 完整教程（基于项目 B 实战）

> **目标**：把 FreeRTOS 讲清楚，能讲给面试官听
> 
> **特色**：所有概念都对应我们项目里的真实代码（firmware/05_freertos + firmware/gateway）
> 
> **学完后你能**：
> - 解释 FreeRTOS 调度原理
> - 写任务 + 队列 + 信号量
> - 排查 Hard Fault / 栈溢出 / 优先级反转
> - 在简历上说"熟悉 FreeRTOS"

---

## 目录

1. [为什么需要 RTOS](#1-为什么需要-rtos)
2. [裸机 vs RTOS 直观对比](#2-裸机-vs-rtos-直观对比)
3. [任务（Task）核心概念](#3-任务task核心概念)
4. [调度器（Scheduler）原理](#4-调度器scheduler原理)
5. [优先级（Priority）详解](#5-优先级priority详解)
6. [队列（Queue）核心机制](#6-队列queue核心机制)
7. [信号量（Semaphore）vs 互斥锁（Mutex）](#7-信号量semaphore-vs-互斥锁mutex)
8. [中断（ISR）与任务通信](#8-中断isr与任务通信)
9. [栈大小与优先级反转](#9-栈大小与优先级反转)
10. [面试 Q&A 50 题](#10-面试-qa-50-题)

---

## 1. 为什么需要 RTOS

### 裸机的痛点（以 BNO055 100Hz 为例）

假设你要在裸机上做 3 件事：
- 每 100ms 读一次 BNO055（100Hz）
- 每 1s 读一次 DHT11
- 每 5s 发一次心跳

**裸机写法**：
```c
while (1) {
    bno055_read();      // 阻塞 10ms
    dht11_read();       // 阻塞 30ms
    usart_puts("HB");   // 1ms
    delay(1000);        // 等等 1 秒
}
```

**问题**：
1. ❌ BNO055 数据丢 99%（每 1000ms 循环一次）
2. ❌ DHT11 阻塞 30ms 期间，BNO055 完全停摆
3. ❌ 任务越多，延迟越严重
4. ❌ 难保证每个任务"周期性准时"

**RTOS 的解法**：每个任务独立运行，由调度器分配 CPU 时间。

---

## 2. 裸机 vs RTOS 直观对比

```
裸机（一个 while 循环）：
  ┌─────────────────────────────────────┐
  │ BNO055(10ms) → DHT11(30ms) → HB(1ms) │
  │           ↓                         │
  │       delay(1000ms)                 │
  └─────────────────────────────────────┘
  特点：串行 → 总周期 1041ms → BNO055 实际 1Hz

RTOS（多个任务并发）：
  ┌─────────┐  ┌─────────┐  ┌─────────┐
  │Sensor   │  │Protocol │  │TX       │
  │(10ms)   │  │(事件)   │  │(事件)   │
  └────┬────┘  └────┬────┘  └────┬────┘
       │            │            │
       └─queue_data─┴─queue_tx───┘
  特点：并行（时分复用）→ BNO055 真 100Hz
```

**关键差异**：RTOS 用"队列"解耦任务，**任务之间不直接调用**。

---

## 3. 任务（Task）核心概念

### 任务是什么

任务 = 一个**永远不会返回**的函数：

```c
void Task_Sensor(void *pvParameters) {
    while (1) {           // 死循环
        /* 干活 */
        vTaskDelay(...);  // 让出 CPU
    }
}
```

### 创建任务（我们项目的代码）

```c
xTaskCreate(
    Task_Sensor,    /* 函数指针 */
    "Sensor",       /* 任务名（调试用）*/
    256,            /* 栈大小（word，不是字节！4 字节/word → 1024 字节）*/
    NULL,           /* 传给任务的参数 */
    3,              /* 优先级（0-4，最高 4）*/
    NULL            /* 任务句柄（不需要可以 NULL）*/
);
```

### 任务的"4 个状态"

```
       xTaskCreate
          ↓
    ┌─────────┐  vTaskSuspend
    │ Ready   │◄──────────┐
    └────┬────┘           │
         │ 调度器选中      │
         ↓                │
    ┌─────────┐           │
    │ Running │──────────┘ vTaskResume / 延时到
    └────┬────┘
         │ 等待（队列空 / vTaskDelay / 信号量）
         ↓
    ┌─────────┐
    │Blocked  │
    └─────────┘
```

**重要**：单核 CPU 上**任意时刻只有 1 个任务在 Running**——RTOS 是"时分复用"。

---

## 4. 调度器（Scheduler）原理

### 我们的 STM32F103 上怎么实现的

**硬件**：Cortex-M3 用 **SysTick 中断**（每 1ms）触发调度

```
SysTick 中断 (每 1ms)
    ↓
xPortSysTickHandler (port.c)
    ↓
xTaskIncrementTick()       ← tick 计数 +1
    ↓
检查是否有任务应该唤醒
    ↓
如果当前任务应该让出 → 触发 PendSV
    ↓
PendSV_Handler (port.c)
    ↓
保存当前任务栈 (R0-R15, xPSR)
    ↓
载入新任务栈
    ↓
新任务开始执行
```

### 调度时机（4 种）

1. **SysTick 中断**（每 1ms）—— 检查优先级调度
2. **任务主动让出**——`vTaskDelay` / `xQueueReceive` 阻塞时
3. **更高优先级任务就绪**——`xQueueSend` 唤醒接收任务时
4. **手动切换**——`taskYIELD()` / `portYIELD_FROM_ISR()`

---

## 5. 优先级（Priority）详解

### 我们项目的优先级设计（4 任务）

| 任务 | 优先级 | 为什么 |
|---|---|---|
| Task_Sensor | **3**（最高）| BNO055 100Hz 数据不能丢 |
| Task_Protocol | 2 | 协议打包跟上节奏 |
| Task_TX | 1 | NRF24 发送可稍等 |
| Task_Heartbeat | **0**（最低）| 心跳无实时要求 |

**原则**：**数据流上游优先**（生产者 > 消费者）

### 优先级数值含义

```c
#define configMAX_PRIORITIES  5     /* 我们配的 */

/* 数字越大 = 优先级越高 */
/* 0 = Idle 任务 */
/* 同优先级 = 时间片轮转（每个 tick 切换一次）*/
```

### 抢占 vs 时间片

```
Task A (prio 3) ─────┐
                      │ 抢占（高优先级立即打断）
Task B (prio 1) ──────┘
                      ↓
Task B 继续 ────────┐
                    │ 时间片轮转（同优先级轮流）
Task C (prio 1) ────┘
```

---

## 6. 队列（Queue）核心机制

### 为什么用队列？

**直接调用的问题**：
```c
void Task_Sensor(void) {
    bno055_read_euler(&eul);  // 直接给另一个任务的变量
}
```
- 数据竞争（一个写一个读）
- 同步问题（消费者还没准备好）
- 耦合度高（任务互相知道对方）

**队列的解法**：
```c
// 发送方
xQueueSend(queue_data, &sensor_msg, portMAX_DELAY);

// 接收方
xQueueReceive(queue_data, &msg, portMAX_DELAY);  // 阻塞直到有数据
```

### 队列的内部实现（简化）

```c
typedef struct {
    uint8_t *pcHead;       // 写指针
    uint8_t *pcTail;       // 读指针
    uint8_t *pcWriteTo;    // 下一个写位置
    UBaseType_t uxMessagesWaiting;  // 当前消息数
    UBaseType_t uxLength;          // 容量
    uint8_t *pvBuffer;      // 数据环形缓冲
} Queue_t;
```

**关键**：队列是**线程安全**的，多任务并发访问不会出问题（FreeRTOS 内部加锁了）。

### 我们项目的队列

```c
typedef struct {
    uint32_t timestamp;
    uint8_t  source;
    uint8_t  data[16];
    uint8_t  data_len;
} sensor_msg_t;        /* 24 字节 */

queue_data = xQueueCreate(8, sizeof(sensor_msg_t));  // 容量 8
```

**队列长度 = 8 怎么算？**
- Sensor 每 10ms 入队一次
- 最坏情况 Protocol 卡 80ms（CRC 计算很慢）
- 80ms × 100Hz = 8 包可能积压
- 取 8 = 5 倍安全余量

---

## 7. 信号量 vs 互斥锁

### 信号量（Semaphore）

**用途**：任务间**事件通知**（"某事发生了"）

```c
SemaphoreHandle_t sem_rx;

// 初始化
sem_rx = xSemaphoreCreateBinary();

// ISR 里（中断发生时）
xSemaphoreGiveFromISR(sem_rx, NULL);

// 任务里（等待事件）
if (xSemaphoreTake(sem_rx, portMAX_DELAY) == pdTRUE) {
    // 事件发生了，处理
}
```

### 互斥锁（Mutex）

**用途**：保护**共享资源**（防止两个任务同时访问）

```c
SemaphoreHandle_t mtx_uart;

// 写 UART
xSemaphoreTake(mtx_uart, portMAX_DELAY);
usart_puts(data);
xSemaphoreGive(mtx_uart);
```

### 关键区别

| 维度 | 信号量 | 互斥锁 |
|---|---|---|
| 用途 | 事件通知 | 资源保护 |
| 所有权 | 无 | 有（谁 take 谁 give）|
| 优先级继承 | ❌ | ✅（防优先级反转）|
| 我们的项目 | v0.2 用 | 没用（v0.1 简化）|

---

## 8. ISR 与任务通信

### 黄金规则

> **ISR 里绝对不能阻塞**（不能调 `xQueueReceive` / `vTaskDelay`）

### 解决方案：FromISR 系列

```c
// ISR 里（接收完成中断）
void USART1_IRQHandler(void) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(queue_rx, &data, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
```

**为什么？** ISR 必须快进快出（μs 级），不能 ms 级阻塞。

### 我们项目的现状（v0.1）

**没有用 ISR**——所有任务用 `vTaskDelay` 轮询：
```c
vTaskDelay(pdMS_TO_TICKS(10));  // 100Hz 轮询
```

**v0.2 改进方向**：加 ISR + 信号量（性能提升 ~10 倍）。

---

## 9. 栈大小与优先级反转

### 栈大小估算（实战公式）

```
栈开销 = 调度上下文 (64B) + 任务局部变量 + 函数调用深度

我们配 256 word = 1024 字节（每个任务）
实际使用（栈高水位线 uxTaskGetStackHighWaterMark）：
  - Sensor: ~150B（局部变量 + 调用）
  - Protocol: ~300B（CRC 计算临时变量）
  - TX: ~100B（usart_puts）
  - Heartbeat: ~50B
```

**栈不够会怎样？Hard Fault！**（FreeRTOS 配置文件勾上 `configCHECK_FOR_STACK_OVERFLOW=2` 即可检测）

### 优先级反转（最经典的 RTOS 坑）

**场景**：
```
Task H (prio 3)  需要 mutex A → 等
Task M (prio 2)  持有 mutex A → 跑
Task L (prio 1)  抢占 Task M → 跑很久
→ Task H 卡死（虽然它优先级最高）
```

**解决**：用 mutex（带优先级继承），不要用二值信号量。

---

## 10. 面试 Q&A 50 题

### 基础（10 题）

1. **FreeRTOS 是什么？单核还是多核？**
   > 实时操作系统内核（RTOS），单核（时分复用）。多核需额外配置。

2. **任务和线程有什么区别？**
   > 在 FreeRTOS 里**没区别**——都是 `xTaskCreate` 创建。"任务"是 FreeRTOS 的官方叫法。

3. **优先级数字越大越高还是越低？**
   > **越大越高**（和我们直觉相反）。`configMAX_PRIORITIES-1` 是最高。

4. **什么是时间片轮转？**
   > 同优先级任务轮流执行，每个 tick（1ms）切换一次。

5. **`vTaskDelay` 和 `delay` 有什么区别？**
   > `delay` 是死等，占 CPU；`vTaskDelay` 让出 CPU，调度器跑别的任务。

6. **`xTaskCreate` 第三个参数 256 是字节吗？**
   > **不是字节，是 word**（4 字节）。所以是 1024 字节栈。

7. **什么时候用队列，什么时候用全局变量？**
   > 队列：任务间通信。全局变量：单个任务用，或加互斥锁。

8. **队列满了 `xQueueSend` 会怎样？**
   > 看第 3 参数：`0`=立即返回失败；`portMAX_DELAY`=阻塞直到有空位。

9. **调度器启动后还能用 printf 吗？**
   > **不建议**。`printf` 很重（浮点支持更重），会破坏任务实时性。

10. **FreeRTOS 用在哪些 MCU？**
    > STM32 / ESP32 / NXP / Nordic / 任何 Cortex-M。**STM32F103 + FreeRTOS 是秋招最常见组合**。

### 中级（20 题）

11. **SysTick 怎么配合 RTOS？**
    > SysTick 每 1ms 触发一次 → xPortSysTickHandler → 检查任务切换。

12. **`vTaskDelayUntil` 和 `vTaskDelay` 区别？**
    > `vTaskDelayUntil` 是**绝对时间**（周期性准确），`vTaskDelay` 是**相对时间**（有累积漂移）。

13. **为什么 ISR 不能直接调 `xQueueSend`？**
    > ISR 必须快进快出（μs 级），`xQueueSend` 可能阻塞。

14. **栈溢出检测的两种方法？**
    > Method 1：任务切换时检查栈顶魔术字；Method 2：检查栈指针越界。我们用 2。

15. **优先级反转是什么？怎么防？**
    > 见 §9。用 mutex（带优先级继承）。

16. **`configUSE_PREEMPTION` 0 和 1 区别？**
    > 0 = 协作式（任务不主动让就不切）；1 = 抢占式（高优先级可打断低优先级）。我们用 1。

17. **`configMAX_SYSCALL_INTERRUPT_PRIORITY` 是什么？**
    > 限制 ISR 优先级，**只有 ≤ 这个值的 ISR 才能调 `FromISR` 函数**。

18. **`heap_1/2/3/4/5` 区别？**
    > heap_1：只分配不释放；heap_4：合并碎片（推荐）；heap_5：支持多块内存。我们用 heap_4。

19. **如何测任务执行时间？**
    > `configGENERATE_RUN_TIME_STATS=1` + `xTaskGetRunTimeStats()`。

20. **`vApplicationStackOverflowHook` 什么时候调用？**
    > 栈溢出时（由 `configCHECK_FOR_STACK_OVERFLOW=2` 触发）。

21. **队列的"环形缓冲"是怎么实现的？**
    > 数组 + 读写指针，`pcWriteTo == pcReadFrom` 区分空/满。

22. **Mutex 怎么实现优先级继承？**
    > 持有 mutex 的低优先级任务**临时提升优先级**到等待 mutex 的高任务优先级。

23. **`portTICK_PERIOD_MS` 怎么用？**
    > `vTaskDelay(pdMS_TO_TICKS(100))` = 100ms tick = `pdMS_TO_TICKS(100)`。

24. **`xTaskCreate` 失败的可能原因？**
    > heap 不够（每个任务 256 word + TCB ≈ 1.3KB）。我们 8KB heap 够 4 任务。

25. **什么时候用 `vTaskSuspend`？**
    > 暂停任务（不调度）。`vTaskResume` 恢复。我们项目没用。

26. **FreeRTOS 怎么管理中断？**
    > `configKERNEL_INTERRUPT_PRIORITY=15`（最低），`BASEPRI` 寄存器屏蔽低优先级 ISR。

27. **`taskENTER_CRITICAL` 怎么用？**
    > 临界区（关调度），成对使用。但**不能从 ISR 调**。

28. **`configUSE_TIME_SLICING` 默认是什么？**
    > 1（开启）。同优先级时间片轮转。

29. **`xQueueCreate(uxQueueLength, uxItemSize)` 的参数？**
    > 长度（消息数）+ 每条消息大小（字节）。

30. **`uxQueueMessagesWaiting` 返回什么？**
    > 当前队列里的消息数（用于监控）。

### 高级（20 题）

31. **Cortex-M3 上 FreeRTOS 的栈长什么样？**
    > 中断时硬件自动压栈（R0-R3, R12, LR, PC, xPSR），FreeRTOS 用 PendSV 切换时手动保存剩余（R4-R11）。

32. **`vTaskStartScheduler` 之后发生了什么？**
    > 创建 Idle 任务 + 第一个任务 → 设置 SysTick → 跳到第一个任务 → 进入主循环。

33. **`portYIELD_FROM_ISR` 必须调用吗？**
    > 不必须（可以省），但调用能让 ISR 立即触发调度，不用等下一个 tick。

34. **FreeRTOS 怎么知道任务栈大小？**
    > 任务创建时指定（`usStackDepth` word），任务 TCB 里保存栈顶指针。

35. **`vTaskList` 输出格式？**
    > `TaskName State Priority Stack Num\r\n`，比如 `Sensor R 3 234 0`。

36. **双核 MCU 怎么用 FreeRTOS？**
    > `configRUN_MULTIPLE_PRIORITIES=1` + `portCORE_ID_x` 区分。

37. **`configUSE_APPLICATION_TASK_TAG` 怎么用？**
    > 给任务打 tag，用于调试或 hook 函数。

38. **`vTaskDelay` 精度是多少？**
    > 1 tick（默认 1ms）。要更高精度用硬件定时器 ISR。

39. **FreeRTOS 怎么支持浮点？**
    > 启用 FPU + `configUSE_TASK_FPU_SUPPORT=2`（lazy stacking）。

40. **`xQueueSendToFront` vs `xQueueSend`？**
    > 前者插到队列头部（LIFO），后者插到尾部（FIFO）。

41. **队列和消息缓冲区（Stream Buffer）区别？**
    > Stream Buffer 是无格式字节流（适合 NRF24 原始数据），Queue 是定长消息。

42. **`vTaskNotify` vs 信号量？**
    > 通知更快（无需创建 IPC 对象），但 1 对 1。

43. **`xTaskAbortDelay` 怎么用？**
    > 让阻塞的任务立即返回。用于"提前结束等待"。

44. **FreeRTOS 怎么测最坏任务响应时间？**
    > `vTaskDelay` 时间 + 中断处理时间 + 调度开销 ≈ 1-2 tick（1-2ms）。

45. **`xTaskCreateRestricted` 是什么？**
    > 限制任务能跑的 CPU（多核用）。我们单核不用。

46. **`configSUPPORT_STATIC_ALLOCATION` 什么时候用？**
    > 不用动态内存（嵌入式内存紧张场景）。我们项目用 heap_4 动态分配。

47. **Tracealyzer 怎么用？**
    > 商业工具，追踪任务调度、ISR、队列事件，30 天试用。

48. **`vTaskGetInfo` 返回什么？**
    > 任务的运行时统计（TCB、优先级、栈使用、状态、运行时间）。

49. **FreeRTOS 怎么保证原子性？**
    > 调度锁 `taskENTER_CRITICAL`（关调度器到退出临界区），或关中断 `taskDISABLE_INTERRUPTS`。

50. **FreeRTOS + TCP/IP（FreeRTOS+TCP）vs LwIP？**
    > FreeRTOS+TCP 是官方集成；LwIP 是第三方。ESP-IDF 用 LwIP。

---

## 实战练习

### 练习 1：在 05_freertos 加 LED 心跳（已完成）

```c
// 在 Task_Heartbeat 里翻转 LED
if ((tick % 300) == 0) led_toggle_all();
```

### 练习 2：加 vApplicationStackOverflowHook 输出任务名

```c
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    usart1_puts("[OVERFLOW] "); usart1_puts(pcTaskName);
    for (;;) {}  /* 卡住，方便用 OpenOCD 看 PC */
}
```

### 练习 3：用 vTaskDelayUntil 做精确 100Hz

```c
TickType_t xLastWakeTime = xTaskGetTickCount();
while (1) {
    bno055_read(&eul);
    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(10));  /* 精确 10ms 周期 */
}
```

### 练习 4：加队列监控

```c
// 在 Task_Heartbeat
usart1_puts("queue_data=");
usart1_putu(uxQueueMessagesWaiting(queue_data));
usart1_puts("\r\n");
```

---

## 进一步学习资源

- **官方文档**：https://www.freertos.org/Documentation/RTOS_book.html
- **正点原子 FreeRTOS 手册**（中文，B 站有视频）
- **源码**：`firmware/05_freertos/third_party/FreeRTOS-Kernel/` 我们项目里就有
- **重点文件**：
  - `tasks.c`（5000+ 行，调度核心）
  - `queue.c`（2000 行，IPC）
  - `list.c`（300 行，链表数据结构）
  - `portable/GCC/ARM_CM3/port.c`（Cortex-M3 移植层）

---

**学完这套，你能在面试里自信地说"我熟悉 FreeRTOS，包括任务调度、队列通信、ISR 同步、栈管理"**。
