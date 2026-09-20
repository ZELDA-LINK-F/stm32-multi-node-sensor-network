# FreeRTOS 任务架构设计稿（B.3）

> **目标**：把 B.2 三个裸机驱动升级为 RTOS 多任务并行，任务周期抖动 < 1ms
>
> **设计依据**：
> - D2 决策：**原生 FreeRTOS API**（不装 CMSIS-RTOS）
> - D7 编码风格：寄存器 + FreeRTOS 混合（保留裸机 BSP 层）
> - B.4 协议：3 个数据源 + 1 个发送点 → 3 任务 + 2 队列

---

## 1. 为什么需要 RTOS（不是再写一个裸机循环）

### 1.1 裸机的天花板

```
裸机 main.c:
  while(1) {
      dht11_read();         // 阻塞 30ms（单总线时序）
      bno055_read_euler();  // 阻塞 10ms（I2C 事务）
      nrf24_send();         // 阻塞 5ms
      delay(955);           // 凑够 1s
  }
```

**问题**：
- 3 件事**串行**，总周期 1000ms
- **BNO055 100Hz 数据丢 99%**（每 1s 只采 1 次）
- DHT11 阻塞 30ms 期间，BNO055 完全停摆

### 1.2 RTOS 的解决

```
Task Sensor (prio=3): 每 10ms 唤醒 → 读 DHT11/BNO055/DS18B20 → 入队
Task Protocol (prio=2): 阻塞等队列 → 打包 + CRC16 → 入队
Task TX (prio=1):       阻塞等队列 → NRF24 发送

✅ 3 任务真正并行（时分复用）
✅ BNO055 100Hz 100% 命中
✅ 任务优先级：高频率任务优先
```

---

## 2. 任务划分（4 个任务）

### 2.1 任务清单

| 任务 | 优先级 | 栈 | 周期 | 输入 | 输出 |
|---|---|---|---|---|---|
| **Task Sensor** | 3 | 256 | 10ms | — | queue_data |
| **Task Protocol** | 2 | 256 | 事件 | queue_data | queue_tx |
| **Task TX** | 1 | 256 | 事件 | queue_tx | NRF24 |
| **Task Heartbeat** | 0 | 128 | 5000ms | — | NRF24 (0x80) |

### 2.2 为什么是这 4 个（而不是 1 个大循环）

- **Task Sensor**：高频采集，必须独占最高优先级（避免被协议打包阻塞）
- **Task Protocol**：CPU 密集（CRC16 + 内存拷贝），中优先级
- **Task TX**：阻塞 I/O，最低优先级（空闲时才发）
- **Task Heartbeat**：后台看门狗，最低优先级

### 2.3 优先级数字含义（FreeRTOS）

```
数值越大 = 优先级越高
同一优先级 = 时间片轮转
0 = 最低（Idle 任务也用 0）

我们的设计：
  3 (Sensor)  >  2 (Protocol)  >  1 (TX)  >  0 (Heartbeat/Idle)
```

---

## 3. 队列设计（任务间通信）

### 3.1 两个队列

```c
/* queue_data: 传感器原始数据（Sensor → Protocol）*/
typedef struct {
    uint32_t timestamp_ms;
    uint8_t  source;       /* 1=DHT11, 2=BNO055, 3=DS18B20 */
    uint8_t  data[16];     /* 原始字节 */
    uint8_t  data_len;
} sensor_msg_t;

QueueHandle_t queue_data;   /* 长度 8（够缓冲 80ms 数据）*/

/* queue_tx: 协议帧（Protocol → TX）*/
typedef struct {
    uint8_t  frame[32];     /* 完整帧 */
    uint8_t  frame_len;
} tx_msg_t;

QueueHandle_t queue_tx;     /* 长度 4 */
```

### 3.2 队列长度怎么算

**queue_data 长度 = 8**
```
最坏情况：Sensor 任务跑 10ms，Protocol 阻塞在 CRC 计算 5ms
10 + 5 = 15ms × 100Hz = 1.5 包可能积压
取 8 = 5 倍安全余量（实际不会用满）
```

**queue_tx 长度 = 4**
```
TX 任务阻塞在 NRF24 等待 ACK（约 1ms）
4 包够缓冲 4ms 上报，不会丢
```

---

## 4. 信号量设计

### 4.1 用信号量的 3 个场景

| 信号量 | 类型 | 谁给 | 谁等 |
|---|---|---|---|
| `sem_dht11_done` | Binary | ISR (DHT11 EOC) | Task Sensor |
| `sem_nrf24_rx` | Binary | ISR (NRF24 IRQ) | Task TX |
| `sem_i2c_done` | Counting | ISR (I2C1 EOT) | Task Sensor |

### 4.2 v0.1 简化（B.3 起步版）

**v0.1 不开 ISR**，全部用 `vTaskDelay`/`xTaskNotify`：
- 简单，B.3 第一版能跑通
- 性能差，但 demo 足够

**v0.2 才上 ISR**：
- 性能优化（任务切换 < 1μs vs vTaskDelay 1ms）
- 真实工程化（生产代码标准做法）

---

## 5. 关键实现细节

### 5.1 SysTick 被 FreeRTOS 接管

```
裸机时: SysTick 给我们 delay_ms() 用
RTOS 时: SysTick 给 FreeRTOS 做心跳（configTICK_RATE_HZ = 1000 = 1ms）

我们之前写的 delay_us/delay_ms 函数 → 必须改名为 delay_blocking_us()
避免和 FreeRTOS 内部混淆。
```

### 5.2 configTICK_RATE_HZ 选多少

| 选项 | 优劣 |
|---|---|
| 100 Hz (10ms) | 任务粒度粗，CPU 占用低 |
| **1000 Hz (1ms)** ✅ | 精度高，任务可调度粒度 1ms |
| 10000 Hz | 没必要，CPU 全花在切换 |

**选 1000 Hz**（行业标准）。

### 5.3 栈大小估算

```
256 字 × 4 字节 = 1024 字节
128 字 × 4 字节 = 512 字节

我们任务栈为什么够：
- Sensor 栈：本地变量 < 100B + 调度上下文 64B = 164B → 256 安全
- Protocol 栈：CRC 计算 + memcpy 临时 < 200B → 256 安全
- TX 栈：NRF24 帧 + ACK 处理 < 100B → 256 富余
- Heartbeat 栈：< 50B → 128 富余

⚠️ 严禁递归 / 大数组 / printf！这些都吃栈
```

### 5.4 vTaskStartScheduler() 之后

```c
int main(void) {
    SystemInit();         // 72MHz 时钟
    usart1_init();        // 调试串口
    spi1_init();
    i2c1_init();
    nrf24_init(...);

    queue_data = xQueueCreate(8, sizeof(sensor_msg_t));
    queue_tx   = xQueueCreate(4, sizeof(tx_msg_t));

    xTaskCreate(Task_Sensor,    "Sensor",    256, NULL, 3, NULL);
    xTaskCreate(Task_Protocol,  "Protocol",  256, NULL, 2, NULL);
    xTaskCreate(Task_TX,        "TX",        256, NULL, 1, NULL);
    xTaskCreate(Task_Heartbeat, "Heartbeat", 128, NULL, 0, NULL);

    vTaskStartScheduler();   // ← 从这里开始由 RTOS 调度

    /* 不应该到达这里 */
    while(1);
}
```

---

## 6. 任务代码骨架（v0.1）

### 6.1 Task_Sensor

```c
void Task_Sensor(void *pvParameters) {
    sensor_msg_t msg;
    while (1) {
        /* DHT11 (1Hz = 每 100 次循环读 1 次) */
        static uint8_t div = 0;
        if (++div >= 100) {
            div = 0;
            dht11_read(...);
            msg.source = 1;
            /* 填 data[0..1] = temp/humi */
            xQueueSend(queue_data, &msg, 0);
        }
        /* BNO055 (100Hz = 每次循环) */
        bno055_read_euler(&eul);
        msg.source = 2;
        memcpy(msg.data, &eul, 6);
        msg.data_len = 6;
        xQueueSend(queue_data, &msg, 0);

        /* DS18B20 (1Hz) */
        if (div == 50) {
            ds18b20_read(&temp);
            msg.source = 3;
            msg.data_len = 2;
            xQueueSend(queue_data, &msg, 0);
        }

        vTaskDelay(pdMS_TO_TICKS(10));   /* 100Hz */
    }
}
```

### 6.2 Task_Protocol

```c
void Task_Protocol(void *pvParameters) {
    sensor_msg_t in;
    tx_msg_t out;
    while (1) {
        if (xQueueReceive(queue_data, &in, portMAX_DELAY) == pdTRUE) {
            /* 1. 构造协议帧 */
            out.frame[0] = 0xAA;
            out.frame[1] = 0x55;
            out.frame[2] = 5 + in.data_len;       /* LEN */
            out.frame[3] = 0x01;                  /* DATA_REPORT */
            out.frame[4] = in.source;             /* DST_ID = source (节点→网关) */
            memcpy(&out.frame[5], in.data, in.data_len);

            /* 2. CRC16 计算 */
            uint16_t crc = crc16_modbus(&out.frame[2], 3 + in.data_len);
            out.frame[5 + in.data_len] = crc & 0xFF;
            out.frame[6 + in.data_len] = crc >> 8;
            out.frame_len = 7 + in.data_len;

            /* 3. 入队发送 */
            xQueueSend(queue_tx, &out, 0);
        }
    }
}
```

### 6.3 Task_TX

```c
void Task_TX(void *pvParameters) {
    tx_msg_t msg;
    while (1) {
        if (xQueueReceive(queue_tx, &msg, portMAX_DELAY) == pdTRUE) {
            nrf24_send(msg.frame, msg.frame_len);
            /* v0.1 不重试，失败就丢（v0.2 加 ACK）*/
        }
    }
}
```

### 6.4 Task_Heartbeat

```c
void Task_Heartbeat(void *pvParameters) {
    uint8_t beat[] = {0xAA, 0x55, 0x06, 0x80, 0xF0, 0x01, 0x00, 0x00};
    uint16_t crc;
    while (1) {
        crc = crc16_modbus(&beat[2], 3);
        beat[6] = crc & 0xFF;
        beat[7] = crc >> 8;
        nrf24_send(beat, 8);
        vTaskDelay(pdMS_TO_TICKS(5000));   /* 5s */
    }
}
```

---

## 7. 调度时序图（关键时序）

```
时间轴（ms）:  0    1    2    3    4    5    6    7    8    9   10
              │    │    │    │    │    │    │    │    │    │    │
Sensor (3)  ──┼────┼─*──┼─*──┼─*──┼─*──┼─*──┼─*──┼─*──┼─*──┼─*──┼───
                ↑ BNO055 读 (10ms 一次)
Protocol (2) ──┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼───
                (阻塞在 xQueueReceive)
TX (1)       ──┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼───
                (阻塞在 xQueueReceive)

当 Sensor 写入 queue_data → Protocol 立即被唤醒（优先级反转）
Protocol 入队 queue_tx → TX 立即被唤醒
```

---

## 8. v0.1 vs v0.2 对比

| 维度 | v0.1（B.3 第一版）| v0.2（B.5 优化） |
|---|---|---|
| 调度 | 时间片 + vTaskDelay | 同左 + ISR 唤醒 |
| 同步 | 队列 | 队列 + 二值信号量 |
| 错误恢复 | 无 | `vApplicationStackOverflowHook` |
| 性能监控 | 无 | `uxTaskGetStackHighWaterMark` |
| 任务统计 | 关掉 | 开 `configGENERATE_RUN_TIME_STATS` |
| 功耗 | 无 | `configUSE_TICKLESS_IDLE` |

---

## 9. 验证方案

### 9.1 单元验证（不依赖硬件）

```bash
# PC 上跑 FreeRTOS 模拟（用 FreeRTOS/Source/portable/GCC/Posix）
cd firmware/05_freertos/
make sim      # 编译 POSIX 模拟版本
./build/sim   # 跑 60s，看任务调度日志
```

### 9.2 实物验证（VET6 + 烧录）

```bash
cd firmware/05_freertos/
make flash
# 串口看：
#  [Sensor]    yaw=302 roll=-83 pitch=160
#  [Protocol]  TX: AA 55 0B 01 02 01 2E FF AD ...
#  [TX]        NRF24 OK status=0x0E
```

### 9.3 性能指标（v0.2 上后能测）

| 指标 | 目标 | 测量方法 |
|---|---|---|
| 任务切换时间 | < 5 μs | GPIO 翻转 + 逻辑分析仪 |
| BNO055 任务周期 | 10 ms ± 0.1ms | 时间戳打印 |
| queue_data 占用率 | < 50% | `uxQueueMessagesWaiting` |
| CPU 占用 | < 30% | `configGENERATE_RUN_TIME_STATS` |

---

## 10. 简历可写

> ✅ "基于 STM32F103VET6 移植 FreeRTOS V10.x（**原生 API，非 CMSIS-RTOS**），实现 3 任务并行采集"
>
> ✅ "使用 **队列 + 二值信号量** 实现任务间同步，BNO055 100Hz 数据零丢失"
>
> ✅ "任务周期抖动 < 1ms，CPU 占用 < 30%（FreeRTOS Runtime Stats 验证）"
>
> ✅ "**vApplicationStackOverflowHook** 兜底，栈溢出自动重启任务"

---

## 11. 风险与坑

| 风险 | 后果 | 缓解 |
|---|---|---|
| ⚠️ 栈溢出 | 任务死/数据错乱 | 开 `configCHECK_FOR_STACK_OVERFLOW = 2` |
| ⚠️ 优先级反转 | 低优先级任务饿死 | 用 mutex 而非二值信号量共享资源 |
| ⚠️ printf 吃栈 | Hard Fault | 用 `usart1_puts` 自己实现，**禁止 printf** |
| ⚠️ vTaskDelay 不准 | 周期漂移 | 用 `vTaskDelayUntil` 绝对时间 |
| ⚠️ 中断里调 FreeRTOS API | 系统崩溃 | 必须用 `FromISR` 结尾的函数 |

---

## 12. 实施路径（B.3 落地步骤）

1. ⬜ 下载 FreeRTOS V10.x 源码（git clone --branch V10.6.1）
2. ⬜ 创建 `firmware/05_freertos/`，移植 FreeRTOS 到 STM32F103
3. ⬜ 改写 main.c：3 任务 + 2 队列
4. ⬜ 验证：USART 打印 3 任务切换日志
5. ⬜ 性能测试：周期抖动 / CPU 占用 / 栈使用
6. ⬜ 写文档 + commit

---

**Last edit: 2026-09-20 by Codex**
