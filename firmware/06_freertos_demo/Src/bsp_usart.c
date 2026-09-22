#include <stdint.h>

/*
 * hal_usart.c — USART1 寄存器版驱动（不依赖 HAL 库）
 */
#include "hal_usart.h"

/* USART1 寄存器 */
#define USART1_BASE   0x40013800UL
#define USART1_SR     (*(volatile uint32_t *)(USART1_BASE + 0x00))
#define USART1_DR     (*(volatile uint32_t *)(USART1_BASE + 0x04))
#define USART1_BRR    (*(volatile uint32_t *)(USART1_BASE + 0x08))
#define USART1_CR1    (*(volatile uint32_t *)(USART1_BASE + 0x0C))

#define USART_SR_TXE  (1U << 7)
#define USART_CR1_UE  (1U << 13)
#define USART_CR1_TE  (1U << 3)
#define USART_CR1_RE  (1U << 2)

/* 72MHz → 115200 波特率：BRR = 72M/115200 = 625 = (39<<4)|1 */
#define USART1_BRR_115200_72MHZ  ((39U << 4) | 1U)

void hal_usart_init(void) {
    /* 开 GPIOA + USART1 时钟（APB2ENR bit2=IOPA, bit14=USART1）*/
    *(volatile uint32_t *)0x40021018 |= (1U << 2) | (1U << 14);

    /* PA9 (TX) 复用推挽输出 50MHz：CRL bit[4:7] = 0xB */
    /* PA10 (RX) 浮空输入：         CRH bit[8:11] = 0x4 */
    *(volatile uint32_t *)0x40010804 =
        (*(volatile uint32_t *)0x40010804 & ~((0xFU << 4) | (0xFU << 8)))
        | ((0xBU << 4) | (0x4U << 8));

    USART1_BRR = USART1_BRR_115200_72MHZ;
    USART1_CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

void hal_usart_putc(char c) {
    while (!(USART1_SR & USART_SR_TXE));   /* 等发送缓冲空 */
    USART1_DR = (uint32_t)c;
}

void hal_usart_puts(const char *s) {
    while (*s) hal_usart_putc(*s++);
}

void hal_usart_putu(uint32_t v) {
    char buf[11];
    int i = 0;
    if (v == 0) { hal_usart_putc('0'); return; }
    while (v) { buf[i++] = '0' + (v % 10); v /= 10; }
    while (i) hal_usart_putc(buf[--i]);
}
