/*
 * hal_delay.h — 精确延时（基于 DWT 周期计数器）
 * @ 72MHz CPU：DWT 每 cycle = 1/72 us
 */
#ifndef BSP_DELAY_H
#define BSP_DELAY_H

#include <stdint.h>

void delay_init(void);          /* 初始化 DWT */
void delay_us(uint32_t us);     /* 微秒延时（用 DWT 精确） */
void delay_ms(uint32_t ms);     /* 毫秒延时（DWT 实现） */

#endif
