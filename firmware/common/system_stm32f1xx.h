/*
 * system_stm32f1xx.h — SystemInit 头文件
 *
 * 用法：startup_stm32f103vctx.s 在 Reset_Handler 里 `bl SystemInit`
 *       user code 可以读 SystemCoreClock 知道当前 SYSCLK
 */
#ifndef SYSTEM_STM32F1XX_H
#define SYSTEM_STM32F1XX_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 系统当前时钟频率（Hz），SystemInit 后 = 72MHz */
extern uint32_t SystemCoreClock;

/* 配置 72MHz 时钟，由 startup 在 main 之前调用 */
void SystemInit(void);

#ifdef __cplusplus
}
#endif

#endif /* SYSTEM_STM32F1XX_H */
