# ESP32-S3 网关设计稿（B.5）

> **目标**：ESP32-S3 通过 NRF24 接收 3 个 STM32 节点数据，转换为 MQTT JSON 上报到 OneNet
>
> **项目复用**：与项目 A（边缘 AI）共用 ESP32-S3 板子和 ESP-IDF FreeRTOS 栈
>
> **核心文档**：
> - 协议帧格式：[PROTOCOL.md](PROTOCOL.md)
> - 节点端 FreeRTOS：[FREERTOS_DESIGN.md](FREERTOS_DESIGN.md)

---

## 1. 网关在系统中的位置

```
┌─────────────────────────────────────────────────────────┐
│                  完整系统架构                            │
│                                                         │
│  ┌──────────┐ NRF24 ┌──────────┐ Wi-Fi/MQTT ┌────────┐│
│  │ 节点 1   │ 2.4G  │          │            │        ││
│  │ C8T6     │◄─────►│  ESP32   │◄──────────►│ OneNet ││
│  │ +DHT11   │       │  -S3     │            │ Dashboard│
│  ├──────────┤       │  网关    │            │        ││
│  │ 节点 2   │       │          │            │ 数据点 ││
│  │ C8T6     │◄─────►│          │            │ 1:温度 ││
│  │ +BNO055  │       │          │            │ 2:姿态 ││
│  ├──────────┤       │          │            │ 3:光照 ││
│  │ 节点 3   │       │          │            │        ││
│  │ VET6     │◄─────►│          │            │        ││
│  │ +DS18B20 │       │          │            │        ││
│  │ +LCD     │       │          │            │        ││
│  └──────────┘       └──────────┘            └────────┘│
│                                                         │
│  异构 (D9 决策): 2×C8T6 + 1×VET6 + 1×ESP32-S3         │
└─────────────────────────────────────────────────────────┘
```

---

## 2. 硬件接线（ESP32-S3 + NRF24L01+）

### 2.1 引脚分配

| ESP32-S3 GPIO | 信号 | NRF24 引脚 | 备注 |
|---|---|---|---|
| GPIO11 | SCK | SCK | SPI2 时钟 |
| GPIO13 | MOSI | MOSI | 主出从入 |
| GPIO12 | MISO | MISO | 主入从出 |
| GPIO10 | CSN | CSN | 片选（低有效）|
| GPIO9  | CE  | CE  | 模式切换 |
| GPIO8  | IRQ | IRQ | 可选（中断唤醒）|
| 3V3    | VCC | VCC | **必须独立供电！** |
| GND    | GND | GND | 共地 |

### 2.2 ⚠️ 供电警告（最常翻车）

ESP32-S3 的 3.3V 引脚**最大 500mA**。NRF24 发射瞬间 ~115mA，**+ WiFi ~200mA + ESP32 自身 ~240mA = 可能跌到 700mA**。

**强烈建议**：
- NRF24 VCC 接 **AMS1117-3.3** 独立稳压（不在 ESP32 板上取电）
- 加 **10µF + 100nF** 去耦到 NRF24 VCC 引脚
- 否则 WiFi 发射时 NRF24 会丢包

### 2.3 ESP32-S3 SPI 资源

```
ESP32-S3 有 3 个 SPI 控制器：
  SPI0 → 内部 Flash（已占用）
  SPI1 → 用户可用
  SPI2 → 用户可用 ← 我们用这个
```

我们用 **SPI2_HOST** 驱动 NRF24。

---

## 3. 软件架构（4 任务并行）

### 3.1 任务划分

```
┌────────────────────────────────────────────────────┐
│           ESP32-S3 Gateway (ESP-IDF)              │
│                                                    │
│  ┌──────────────┐                                  │
│  │ SPI ISR      │ ──NRF24 收到 1 包──┐             │
│  └──────────────┘                    │             │
│                                      ▼             │
│                              ┌──────────────┐      │
│                              │ Task RX      │      │
│                              │  prio 10     │      │
│                              │  NRF24→队列  │      │
│                              └──────┬───────┘      │
│                                     │              │
│                                queue_frames        │
│                                     │              │
│                              ┌──────▼───────┐      │
│                              │ Task Parser  │      │
│                              │  prio 8      │      │
│                              │  CRC + 解析  │      │
│                              └──────┬───────┘      │
│                                     │              │
│                                queue_data          │
│                                     │              │
│                              ┌──────▼───────┐      │
│                              │ Task MQTT    │      │
│                              │  prio 5      │      │
│                              │  JSON + 上报 │      │
│                              └──────┬───────┘      │
│                                     │              │
│                                     ▼              │
│                              ┌──────────────┐      │
│                              │ OneNet Cloud │      │
│                              └──────────────┘      │
│                                                    │
│  ┌──────────────┐                                  │
│  │ WiFi 事件     │ → xEventGroup → Task WiFiMgr   │
│  └──────────────┘                                  │
└────────────────────────────────────────────────────┘
```

### 3.2 任务清单

| 任务 | 优先级 | 栈 | 周期 | 功能 |
|---|---|---|---|---|
| **Task NRF24_RX** | 10 | 4096 | 中断触发 | 从 NRF24 FIFO 读 → CRC 校验 → 入队 |
| **Task Parser** | 8 | 2048 | 队列触发 | 解析协议字段 → 转换为 JSON 字段 |
| **Task MQTT** | 5 | 4096 | 队列触发 | 构造 JSON → mqtt_publish → OneNet |
| **Task WiFiMgr** | 3 | 2048 | 事件触发 | 管理 Wi-Fi 连接 + 重连 |
| **Task Heartbeat** | 1 | 1024 | 30s | 打印系统状态 + 上报心跳 |

### 3.3 与 STM32 端任务对比

| 维度 | STM32 节点 | ESP32 网关 |
|---|---|---|
| RTOS | FreeRTOS-Kernel V10.6.1 | ESP-IDF FreeRTOS |
| 任务数 | 4 | 5 |
| 协议栈 | 无（裸 SPI）| TCP/IP + MQTT |
| 通信对象 | NRF24 (单方向发) | NRF24 (收) + WiFi (发) |
| 优先级数字范围 | 0-4 | 0-25 |

---

## 4. NRF24 驱动移植（STM32 → ESP32）

### 4.1 算法复用

我们之前写的 `bsp_nrf24.c/.h` 是**纯算法**，与平台无关：

```c
// 这部分代码可复用：
uint8_t nrf24_init(nrf24_mode_t mode, uint8_t channel);
uint8_t nrf24_send(const uint8_t *payload, uint8_t len);
uint8_t nrf24_receive(uint8_t *payload, uint8_t max_len);
void nrf24_set_mode(nrf24_mode_t mode);
void nrf24_set_tx_address(const uint8_t *addr5);
uint8_t nrf24_read_status(void);
```

### 4.2 平台差异（SPI 部分）

| 维度 | STM32 | ESP32 |
|---|---|---|
| SPI 初始化 | 寄存器级 RCC + GPIO + SPI1 | ESP-IDF spi_bus_initialize + spi_device_add |
| SPI 收发 | spi1_transfer(1 byte) | spi_device_polling_transmit(handles multi-byte) |
| 字节序 | 直接赋值 | 直接赋值 |
| CSN 控制 | GPIO 寄存器 | ESP-IDF spi_device_select |

**策略**：写一个 `nrf24_spi.c` 抽象层，提供 `nrf24_spi_transfer(uint8_t)` 接口。STM32 和 ESP32 各实现一份。

---

## 5. 协议解析器（C 实现）

### 5.1 数据结构（对应 PROTOCOL.md）

```c
typedef struct {
    uint8_t  type;        /* 消息类型 */
    uint8_t  dst_id;      /* 源/目标地址 */
    uint32_t timestamp;   /* 接收时间戳（ms）*/
    union {
        struct { uint8_t temp_int; uint8_t temp_dec; uint8_t humi; } dht11;
        struct { int16_t yaw; int16_t roll; int16_t pitch; } bno055;
        struct { int16_t temp; uint16_t light; uint8_t status; } vet6;
    } payload;
    uint8_t  valid;       /* CRC 通过标志 */
} protocol_data_t;
```

### 5.2 解析器（来自 PROTOCOL.md §5）

```c
uint8_t protocol_parse(const uint8_t *buf, uint8_t len,
                       protocol_data_t *out, uint32_t timestamp) {
    /* 1. 最小长度 */
    if (len < 7) return 0;

    /* 2. 帧头 */
    if (buf[0] != 0xAA || buf[1] != 0x55) return 0;

    /* 3. LEN 一致性 */
    if ((buf[2] + 2) != len) return 0;

    /* 4. CRC16 校验（覆盖 LEN 起所有字节）*/
    uint16_t calc = crc16_modbus(&buf[2], len - 4);
    uint16_t recv = ((uint16_t)buf[len-1] << 8) | buf[len-2];
    if (calc != recv) return 0;

    /* 5. 解析字段 */
    out->type      = buf[3];
    out->dst_id    = buf[4];
    out->timestamp = timestamp;
    out->valid     = 1;

    /* 6. 按类型解析 PAYLOAD（PROTOCOL.md §2）*/
    uint8_t plen = buf[2] - 5;
    const uint8_t *p = &buf[5];

    if (out->type == 0x01 && out->dst_id == 1) {
        /* DHT11: temp_int + temp_dec + humi */
        out->payload.dht11.temp_int = p[0];
        out->payload.dht11.temp_dec = p[1];
        out->payload.dht11.humi     = p[2];
    } else if (out->type == 0x01 && out->dst_id == 2) {
        /* BNO055: yaw + roll + pitch (大端 int16) */
        out->payload.bno055.yaw   = (int16_t)((p[0] << 8) | p[1]);
        out->payload.bno055.roll  = (int16_t)((p[2] << 8) | p[3]);
        out->payload.bno055.pitch = (int16_t)((p[4] << 8) | p[5]);
    } else if (out->type == 0x01 && out->dst_id == 3) {
        /* VET6: temp(×100) + light + status */
        out->payload.vet6.temp  = (int16_t)((p[0] << 8) | p[1]);
        out->payload.vet6.light = (uint16_t)((p[2] << 8) | p[3]);
        out->payload.vet6.status= p[4];
    }

    return 1;
}
```

### 5.3 CRC16 算法（直接复用）

```c
uint16_t crc16_modbus(const uint8_t *data, uint8_t len) {
    uint16_t crc = 0xFFFF;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x0001) crc = (crc >> 1) ^ 0xA001;
            else              crc >>= 1;
        }
    }
    return crc;
}
```

我们已经用 Python (`tools/crc16_test.py`) 验证过这个算法（标准测试向量 ✅）。

---

## 6. MQTT 主题设计

### 6.1 主题结构（D9 异构 + B.4 协议）

```
stm32/{node_id}/{data_type}

例：
  stm32/01/dht11    → {"temp":25.3,"humi":60}
  stm32/02/bno055   → {"yaw":30.5,"roll":-5.2,"pitch":10.0}
  stm32/03/vet6     → {"temp":23.45,"light":1500,"status":"normal"}
  stm32/sys/heartbeat → {"uptime":3600,"nodes_online":[1,2,3]}
```

### 6.2 OneNet 数据点映射

| MQTT 主题 | OneNet 数据流 | 数据点 |
|---|---|---|
| `stm32/01/dht11` | `stm32_node1_env` | `temp` / `humi` |
| `stm32/02/bno055` | `stm32_node2_imu` | `yaw` / `roll` / `pitch` |
| `stm32/03/vet6` | `stm32_node3_main` | `temp` / `light` / `status` |

### 6.3 主题命名约定的设计选择

- **`stm32/` 前缀**：OneNet 上区分项目 A（边缘 AI）和项目 B（传感网）
- **`{node_id}` 数字**：1-3 匹配 PROTOCOL.md 地址分配
- **`{data_type}` 英文**：可读性好，前端解析简单

---

## 7. JSON 转换（协议帧 → MQTT 载荷）

### 7.1 转换函数

```c
char* data_to_json(const protocol_data_t *d, char *buf, size_t buflen) {
    if (d->dst_id == 1) {  /* DHT11 */
        snprintf(buf, buflen,
                 "{\"temp\":%d.%d,\"humi\":%d}",
                 d->payload.dht11.temp_int, d->payload.dht11.temp_dec,
                 d->payload.dht11.humi);
    } else if (d->dst_id == 2) {  /* BNO055 */
        snprintf(buf, buflen,
                 "{\"yaw\":%.2f,\"roll\":%.2f,\"pitch\":%.2f}",
                 d->payload.bno055.yaw / 16.0,
                 d->payload.bno055.roll / 16.0,
                 d->payload.bno055.pitch / 16.0);
    } else if (d->dst_id == 3) {  /* VET6 */
        snprintf(buf, buflen,
                 "{\"temp\":%.2f,\"light\":%u,\"status\":%d}",
                 d->payload.vet6.temp / 100.0,
                 d->payload.vet6.light,
                 d->payload.vet6.status);
    } else {
        snprintf(buf, buflen, "{\"error\":\"unknown_node\"}");
    }
    return buf;
}
```

### 7.2 上报到 OneNet

```c
void mqtt_publish_node_data(const protocol_data_t *d) {
    char json[128];
    data_to_json(d, json, sizeof(json));

    char topic[64];
    if (d->dst_id == 1)      snprintf(topic, sizeof(topic), "stm32/01/dht11");
    else if (d->dst_id == 2) snprintf(topic, sizeof(topic), "stm32/02/bno055");
    else if (d->dst_id == 3) snprintf(topic, sizeof(topic), "stm32/03/vet6");

    esp_mqtt_client_publish(mqtt_client, topic, json, 0, 1, 0);
    /*                              ↑       ↑    ↑    ↑  ↑
     *                              handle  topic payload len QoS retain
     */
}
```

---

## 8. ESP-IDF 工程结构

```
firmware/gateway/                  ← 新建（B.5 第二轮才写代码）
├── CMakeLists.txt
├── sdkconfig.defaults             ← 默认配置
├── main/
│   ├── main.c                     ← app_main() + 任务创建
│   ├── gateway_config.h           ← WiFi SSID / OneNet token
│   ├── bsp_nrf24_spi.c            ← ESP32 SPI 适配（用 ESP-IDF API）
│   ├── bsp_nrf24.c                ← 算法（STM32 端复用）
│   ├── protocol.c                 ← 协议解析器 + CRC16
│   ├── mqtt_client.c              ← MQTT 客户端
│   ├── one_net.c                  ← OneNet API 封装
│   └── tasks/
│       ├── task_nrf24_rx.c
│       ├── task_parser.c
│       ├── task_mqtt.c
│       ├── task_wifi_mgr.c
│       └── task_heartbeat.c
└── docs/
    └── GATEWAY_DESIGN.md          ← 本文件
```

---

## 9. OneNet 接入

### 9.1 平台选择

| 平台 | 难度 | 文档稳定性 |
|---|---|---|
| **OneNet 物联网平台（老版）** ✅ | 中 | 文档 10 年稳定 |
| OneNet Studio | 低（GUI） | API 经常改版 |
| 阿里云 IoT | 中 | 文档好 |
| 腾讯云 IoT Explorer | 中 | 文档好 |

**选 OneNet 老版**：D1-D9 决策历史里已经选过。

### 9.2 接入流程

```
1. 注册 OneNet 账号
2. 创建产品：设备接入协议 = MQTT
3. 创建设备：得到 device_id + api_key
4. ESP32 连接：mqtt://mqtt.heclouds.com:6002
5. 订阅主题 / 上报数据
```

### 9.3 鉴权

```
mqtt_username = device_id
mqtt_password = api_key 的 MD5（首字母大写）
```

---

## 10. 验证方案

### 10.1 单元验证（PC 跑）

```bash
# 协议解析器单元测试
cd firmware/gateway/test/
gcc -o test_protocol test_protocol.c ../main/protocol.c
./test_protocol
# 输入：硬编码的测试帧
# 输出：parsed: yaw=30.5° valid=1
```

### 10.2 集成验证（实物）

```bash
# 1. 节点端烧 hello_uart（验证 SPI/NRF24）
cd firmware/04_nrf24
make flash

# 2. ESP32-S3 烧 gateway
cd firmware/gateway
idf.py build flash monitor

# 3. 看串口日志：
#  [NRF24_RX] Got 13 bytes from addr 0xE7E7E7E7E7
#  [Parser] CRC OK, type=DATA_REPORT, dst_id=2
#  [Parser] yaw=302 roll=-83 pitch=160
#  [MQTT] Published to stm32/02/bno055: {"yaw":30.5,"roll":-5.2,"pitch":10.0}

# 4. 看 OneNet Dashboard：数据流 stm32_node2_imu 出现新数据点
```

### 10.3 性能指标

| 指标 | 目标 | 测量 |
|---|---|---|
| 端到端延迟 | < 100ms | 节点发包 → 网关收包 → OneNet 显示 时间差 |
| 丢包率 | < 1% | 1000 包对比收发计数 |
| OneNet 上报成功率 | > 99% | 串口日志 vs Dashboard 数据 |
| 网关 CPU 占用 | < 30% | ESP-IDF Runtime Stats |

---

## 11. 风险与缓解

| 风险 | 缓解 |
|---|---|
| ⚠️ NRF24 供电不足（WiFi 干扰）| 独立 AMS1117 + 10µF 去耦 |
| ⚠️ OneNet 鉴权过期 | 写本地存储，重启自动重连 |
| ⚠️ MQTT QoS 选择 | 用 QoS 1（PROTOCOL.md §4.3） |
| ⚠️ WiFi 断连 | EventGroup 触发 WiFiMgr 任务重连 |
| ⚠️ NRF24 SPI 速率冲突 | 用 SPI2（独立于 ESP32 内部 Flash）|
| ⚠️ 多节点同时上报 → 网关 FIFO 满 | NRF24 有 3 級 FIFO + ACK 重传 |

---

## 12. 简历可写

> ✅ "ESP32-S3 网关通过 NRF24 接收 3 节点异构数据（**SPI2 9MHz 寄存器级驱动**），实现协议解析 + CRC16 校验 + JSON 转换"
>
> ✅ "基于 ESP-IDF FreeRTOS 设计 **5 任务并行**（RX / Parser / MQTT / WiFiMgr / Heartbeat），双队列解耦 NRF24 与 MQTT"
>
> ✅ "通过 **MQTT QoS 1** 接入 OneNet 物联网平台，3 类主题（`stm32/{id}/{type}`）映射数据流，端到端延迟 < 100ms"

---

## 13. 实施路径（B.5 落地步骤）

1. ⬜ 创建 `firmware/gateway/` ESP-IDF 工程
2. ⬜ 移植 NRF24 驱动（SPI2 实现 + 算法复用）
3. ⬜ 写协议解析器（PROTOCOL.md §5）
4. ⬜ 写 MQTT 客户端 + OneNet 鉴权
5. ⬜ 写 5 任务（架构图 §3）
6. ⬜ 集成测试（节点 ↔ 网关 ↔ OneNet）
7. ⬜ 性能测试 + 优化

---

**Last edit: 2026-09-20 by Codex**
