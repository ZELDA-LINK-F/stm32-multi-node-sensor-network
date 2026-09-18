# 决策记录（D1–D7）

> 进入 Phase B.1 前必须全部勾选。**当前全部勾选完毕**。

| # | 决策项 | 选择 | 状态 |
|---|---|---|---|
| D1 | 开发环境 | Linux + VSCode + EIDE + arm-none-eabi-gcc + OpenOCD | ✅ |
| D2 | FreeRTOS API 入口 | 原生 FreeRTOS API（面试加分）| ✅ |
| D3 | 协议字节序 | 大端 | ✅ |
| D4 | CRC16 算法 | CRC16-MODBUS（0xFFFF + 0xA001）| ✅ |
| D5 | 网关项目结构 | 单独 `gateway/` 项目 | ✅ |
| D6 | 节点供电 | USB 5V + 板载 3.3V LDO | ✅ |
| D7 | 编码风格 | **B.1 寄存器级 + B.2 混合 + 不装 CubeMX** | ✅ |

## D7 编码风格细节

| Phase | 风格 | 库 / 工具 |
|---|---|---|
| **B.1 hello_uart** | 纯寄存器 | 无（直接操作 RCC / GPIO / USART 寄存器）|
| **B.2.1 DHT11** | 寄存器 | 无（单总线 + GPIO + SysTick 延时）|
| **B.2.2 MPU6050** | HAL | STM32CubeF1 源码包（HAL_I2C）|
| **B.2.3 NRF24L01+** | HAL SPI + 寄存器配 NRF | STM32CubeF1 HAL_SPI + 直接写 NRF24 命令字 |
| **B.3 FreeRTOS** | 原生 API | FreeRTOS Kernel V10.x |
| **B.4 协议** | 纯 C | 无（与硬件无关）|
| **B.5 组网** | 复用 | — |

## HAL 库获取方式（**不装 CubeMX 应用**）

从 ST 官网直接下源码包：
```
https://www.st.com/en/embedded-software/stm32cubef1.html
en.stm32cubef1.zip  → 解压得到 Drivers/STM32F1xx_HAL_Driver/
```

把 `Drivers/` 拷贝到对应工程即可。**CubeMX 应用绝不安装**。

## D7 简历可写

> ✅ "基于 STM32F103VET6 实现裸机寄存器级 USART1 / GPIO / RCC 配置（参考 RM0008 Reference Manual），不依赖 HAL 库"
>
> ✅ "基于 STM32CubeF1 HAL 库实现 I2C / SPI 设备驱动（MPU6050 / NRF24L01+）"
>
> ✅ "Linux 下使用 arm-none-eabi-gcc + Makefile + OpenOCD/st-flash 全命令行开发 STM32"

