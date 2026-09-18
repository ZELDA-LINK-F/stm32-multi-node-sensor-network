# 决策记录（D1–D6）

> 进入 Phase B.1 前必须全部勾选。

| # | 决策项 | 选择 | 状态 |
|---|---|---|---|
| D1 | 开发环境 | **② Linux + VSCode + EIDE + arm-none-eabi-gcc + OpenOCD** | ✅ |
| D2 | FreeRTOS API 入口 | 原生 FreeRTOS API（面试加分）| ✅ |
| D3 | 协议字节序 | 大端 | ✅ |
| D4 | CRC16 算法 | CRC16-MODBUS（0xFFFF + 0xA001）| ✅ |
| D5 | 网关项目结构 | 单独 `gateway/` 项目 | ✅ |
| D6 | 节点供电 | USB 5V + 板载 3.3V LDO | ✅ |

## D1 工具链细节

| 角色 | 工具 | 用途 |
|---|---|---|
| 代码生成 | STM32CubeMX（Linux AppImage 或 Java） | 生成 HAL 初始化代码（`.c/.h`） |
| 编辑器 | VSCode + Embedded IDE 扩展 | 工程管理、代码补全、编译按钮 |
| 编译器 | arm-none-eabi-gcc + newlib | 产出 `.elf` / `.bin` |
| 烧录器 | st-flash 或 OpenOCD | 写入 STM32 Flash |
| 调试器 | VSCode Cortex-Debug + OpenOCD（GDB server）| 断点、寄存器、FreeRTOS 感知 |

## D1 配套约定

- 工程由 CubeMX 生成后纳入 VSCode，**`.ioc` 文件提交 git**
- 编译产物（`build/`、`Debug/`、`Release/`）由 `.gitignore` 过滤
- 烧录命令统一为 `make flash` 或 EIDE 按钮，对应命令：
  ```bash
  st-flash --reset write build/firmware.bin 0x08000000
  ```
- 调试命令（可选）：
  ```bash
  openocd -f interface/stlink.cfg -f target/stm32f1x.cfg
  # 另一个终端：arm-none-eabi-gdb build/firmware.elf
  ```
