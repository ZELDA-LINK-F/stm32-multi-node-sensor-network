# 项目 B 架构规范（代码规范 v1.0）

> **目的**：让项目代码像正规嵌入式项目
> **范围**：firmware/ 所有 STM32 工程

## 1. 分层架构（4 层）

```
Layer 4: APP（应用）          ← firmware/XX/Src/main.c
Layer 3: DRV（设备驱动）      ← firmware/common/drv_*.c
Layer 2: HAL（硬件抽象）      ← firmware/common/hal_*.c
Layer 1: SYS（系统）          ← firmware/common/stm32f1xx.h
```

**调用方向**：APP → DRV → HAL → SYS（单向）

## 2. 文件命名规范

| 前缀 | 含义 | 例子 |
|---|---|---|
| `stm32f1xx_` | 寄存器宏 | `stm32f1xx.h` |
| `sys_` | 系统功能 | `sys_clock.c` |
| `hal_` | 硬件抽象 | `hal_gpio.c` |
| `drv_` | 设备驱动 | `drv_dht11.c` |
| `app_` | 业务逻辑 | `app_main.c` |

**注意**：之前 `bsp_*` 应该改成 `hal_*`（更标准术语）

### 函数命名

```c
hal_gpio_init(...)        /* HAL 层通用动作 */
drv_dht11_read(...)       /* DRV 层具体设备 */
app_main_loop(...)        /* APP 层业务流程 */
```

规则：
- `<layer>_<module>_<action>()`
- 全小写 + 下划线
- 动词在前

### 宏/类型

```c
#define HAL_GPIO_PORT_A    0      /* 全大写下划线 */
#define DRV_DHT11_PIN      3

typedef enum {                 /* 类型 _t 后缀 */
    DRV_DHT11_OK = 0,
    DRV_DHT11_ERR_TIMEOUT,
} drv_dht11_status_t;
```

## 3. 头文件规范

每个 .h 必须有 Doxygen 注释：

```c
/**
 * @file    drv_dht11.h
 * @brief   DHT11 温湿度传感器驱动
 * @details 单总线协议，开漏 + 外部上拉 4.7kΩ
 * @author  ZELDA-LINK-F
 * @date    2026-09-22
 */

#ifndef DRV_DHT11_H
#define DRV_DHT11_H
...
#endif
```

## 4. 错误处理

所有 DRV 函数返回状态码：

```c
typedef enum {
    DRV_XXX_OK = 0,
    DRV_XXX_ERR_TIMEOUT,
    DRV_XXX_ERR_CHECKSUM,
} drv_xxx_status_t;
```

## 5. 目标目录结构

```
firmware/
├── common/                     ← 公共库
│   ├── stm32f1xx.h             ← SYS 寄存器
│   ├── sys_clock.c/.h          ← SYS 72MHz
│   ├── hal_gpio.c/.h           ← HAL GPIO
│   ├── hal_usart.c/.h          ← HAL USART
│   ├── hal_delay.c/.h          ← HAL DWT 延时
│   ├── hal_i2c.c/.h            ← HAL I2C（新增）
│   ├── hal_spi.c/.h            ← HAL SPI（新增）
│   ├── drv_dht11.c/.h          ← DRV DHT11
│   ├── drv_nrf24.c/.h          ← DRV NRF24
│   └── drv_bno055.c/.h         ← DRV BNO055
├── 01_hello_uart/
│   └── src/main.c              ← APP 极简
├── 02_dht11/
│   ├── src/main.c
│   └── src/drv_dht11.c/.h
├── 03_bno055/
│   ├── src/main.c
│   └── src/drv_bno055.c/.h
├── 04_nrf24/
│   ├── src/main.c
│   └── src/drv_nrf24.c/.h
└── 05_freertos/
    └── src/main.c
```

**HAL 在 common/** = 通用实现
**DRV 在 XX/src/** = 用 HAL 实现具体芯片

## 6. 渐进迁移路径

| 优先级 | 任务 | 工作量 |
|---|---|---|
| 🔴 高 | Doxygen 文档化 | 1h |
| 🔴 高 | bsp_* → hal_* 改名 | 30min |
| 🟡 中 | 错误码统一 | 2h |
| 🟡 中 | HAL 拆细（i2c/spi）| 3h |
| 🟢 低 | 类型抽象（typedef）| 2h |

## 7. 编码风格

- 缩进 4 空格
- 行宽 ≤ 100
- 花括号 K&R 风格
- 注释 Doxygen 风格
- 头文件保护 `#ifndef XXX_H`

## 8. 不做的事

- ❌ HAL 不依赖 RTOS
- ❌ 不用面向对象（保持纯 C）
- ❌ 不用动态内存
- ❌ 不引入第三方库

**Last edit: 2026-09-22**
