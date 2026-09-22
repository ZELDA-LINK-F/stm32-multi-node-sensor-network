/**
 * @file    stm32f1xx.h
 * @brief   STM32F103 寄存器宏定义（统一头）
 * @details 5 个 STM32 工程共用的寄存器地址 + 位定义
 *          分两种形式：
 *          - _ADDR 后缀：地址常量（用于数组初始化）
 *          - 普通宏：dereferenced 寄存器（用于赋值）
 * @author  ZELDA-LINK-F
 * @date    2026-09-22
 * @version 1.0
 * 
 * @section 设计原则
 * 1. SYS 层（最底层）：只定义寄存器，不做逻辑
 * 2. 所有 _ADDR 是地址常量，可用在 static const 数组里
 * 3. 普通宏是 dereferenced 指针，用于直接读写寄存器
 */

#ifndef STM32F1XX_H
#define STM32F1XX_H

#include <stdint.h>

/* === 时钟基地址 === */
#define RCC_BASE        0x40021000UL
#define RCC_APB2ENR     (*(volatile uint32_t *)(RCC_BASE + 0x18))

/* === GPIOA === */
#define GPIOA_BASE      0x40010800UL
#define GPIOA_CRL_ADDR  (GPIOA_BASE + 0x00)
#define GPIOA_CRH_ADDR  (GPIOA_BASE + 0x04)
#define GPIOA_IDR_ADDR  (GPIOA_BASE + 0x08)
#define GPIOA_ODR_ADDR  (GPIOA_BASE + 0x0CUL)
#define GPIOA_BSRR_ADDR (GPIOA_BASE + 0x10UL)
#define GPIOA_CRL       (*(volatile uint32_t *)GPIOA_CRL_ADDR)
#define GPIOA_CRH       (*(volatile uint32_t *)GPIOA_CRH_ADDR)
#define GPIOA_IDR       (*(volatile uint32_t *)GPIOA_IDR_ADDR)
#define GPIOA_ODR       (*(volatile uint32_t *)GPIOA_ODR_ADDR)
#define GPIOA_BSRR      (*(volatile uint32_t *)GPIOA_BSRR_ADDR)
#define RCC_APB2ENR_IOPAEN  (1U << 2)

/* === GPIOB === */
#define GPIOB_BASE      0x40010C00UL
#define GPIOB_CRL_ADDR  (GPIOB_BASE + 0x00)
#define GPIOB_CRH_ADDR  (GPIOB_BASE + 0x04)
#define GPIOB_IDR_ADDR  (GPIOB_BASE + 0x08)
#define GPIOB_ODR_ADDR  (GPIOB_BASE + 0x0CUL)
#define GPIOB_BSRR_ADDR (GPIOB_BASE + 0x10UL)
#define GPIOB_CRL       (*(volatile uint32_t *)GPIOB_CRL_ADDR)
#define GPIOB_CRH       (*(volatile uint32_t *)GPIOB_CRH_ADDR)
#define GPIOB_IDR       (*(volatile uint32_t *)GPIOB_IDR_ADDR)
#define GPIOB_ODR       (*(volatile uint32_t *)GPIOB_ODR_ADDR)
#define GPIOB_BSRR      (*(volatile uint32_t *)GPIOB_BSRR_ADDR)
#define RCC_APB2ENR_IOPBEN  (1U << 3)

/* === USART1 === */
#define USART1_BASE     0x40013800UL
#define USART1_SR       (*(volatile uint32_t *)(USART1_BASE + 0x00))
#define USART1_DR       (*(volatile uint32_t *)(USART1_BASE + 0x04))
#define USART1_BRR      (*(volatile uint32_t *)(USART1_BASE + 0x08))
#define USART1_CR1      (*(volatile uint32_t *)(USART1_BASE + 0x0CUL))
#define RCC_APB2ENR_USART1EN  (1U << 14)
#define USART_SR_TXE    (1U << 7)
#define USART_CR1_UE    (1U << 13)
#define USART_CR1_TE    (1U << 3)
#define USART_CR1_RE    (1U << 2)

/* === SysTick === */
#define SYSTICK_CTRL    (*(volatile uint32_t *)0xE000E010UL)
#define SYSTICK_LOAD    (*(volatile uint32_t *)0xE000E014UL)
#define SYSTICK_VAL     (*(volatile uint32_t *)0xE000E018UL)
#define SYSTICK_CTRL_CLKSOURCE  (1U << 2)
#define SYSTICK_CTRL_ENABLE     (1U << 0)
#define SYSTICK_CTRL_COUNTFLAG  (1U << 16)

/* === DWT（精确微秒延时）=== */
#define CoreDebug_DEMCR (*(volatile uint32_t *)0xE000EDFCUL)
#define DWT_CTRL        (*(volatile uint32_t *)0xE0001000UL)
#define DWT_CYCCNT      (*(volatile uint32_t *)0xE0001004UL)
#define DEMCR_TRCENA    (1U << 24)
#define DWT_CTRL_CYCCNTENA  (1U << 0)

/* === GPIO 模式宏（CNF + MODE）=== */
#define GPIO_OUT_PP_50M  0x3
#define GPIO_OUT_OD_50M  0x7
#define GPIO_IN_FLOAT    0x4

/* === 位操作宏 === */
#define GPIO_SET_BIT(gpio_bsrr, pin)   ((gpio_bsrr) = (1U << (pin)))
#define GPIO_RESET_BIT(gpio_bsrr, pin) ((gpio_bsrr) = (1U << ((pin) + 16)))

#endif
