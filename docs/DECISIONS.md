# 决策记录（D1–D6）

> 进入 Phase B.1 前必须全部勾选。未勾选项会阻塞后续动作。

| # | 决策项 | 我的推荐 | 用户选择 | 状态 |
|---|---|---|---|---|
| D1 | 开发环境（Linux 工具链）| Linux + VSCode + EIDE + arm-none-eabi-gcc | — | ⏳ 待定 |
| D2 | FreeRTOS API 入口 | 原生 FreeRTOS API（面试加分）| 接受默认 | ✅ |
| D3 | 协议字节序 | 大端 | 接受默认 | ✅ |
| D4 | CRC16 算法 | CRC16-MODBUS（0xFFFF + 0xA001）| 接受默认 | ✅ |
| D5 | 网关项目结构 | 单独 `gateway/` 项目 | 接受默认 | ✅ |
| D6 | 节点供电 | USB 5V + 板载 3.3V LDO | 接受默认 | ✅ |

## D1 候选方案

| 子方案 | 工具链 | 编译 | 烧录 | 调试 | 简历加分 |
|---|---|---|---|---|---|
| ① Windows + Keil MDK | Keil 闭源 | Keil 一键 | Keil 一键 | Keil 仿真 | 低（学生默认）|
| ② Linux + VSCode + EIDE | arm-none-eabi-gcc + OpenOCD | EIDE 一键 / Make | OpenOCD / st-flash | cortex-debug + JLink | **高** |
| ③ Linux + STM32CubeIDE | CubeIDE 自带 GCC | CubeIDE 一键 | CubeIDE 一键 | CubeIDE 仿真 | 中 |

**当前倾向**：②。理由见 `docs/PLAN.md` §5。
