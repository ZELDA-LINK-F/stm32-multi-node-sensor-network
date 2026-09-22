/*
 * hal_delay.c — 精确延时（基于 DWT 周期计数器）
 *
 * 为什么用 DWT 而不是 SysTick？
 *   - SysTick 配置繁琐（LOAD/VAL/CTRL 三步）
 *   - SysTick 容易被其他代码意外修改
 *   - DWT 是 ARM 内核调试单元，永远在跑，1 cycle = 1/72 us
 */
#include "hal_delay.h"
#include "stm32f1xx.h"

#define CPU_HZ  72000000UL

void delay_init(void) {
    CoreDebug_DEMCR |= DEMCR_TRCENA;
    DWT_CYCCNT = 0;
    DWT_CTRL  |= DWT_CTRL_CYCCNTENA;
}

void delay_us(uint32_t us) {
    uint32_t start = DWT_CYCCNT;
    uint32_t ticks = us * (CPU_HZ / 1000000UL);
    while ((DWT_CYCCNT - start) < ticks);
}

void delay_ms(uint32_t ms) {
    while (ms--) delay_us(1000);
}
