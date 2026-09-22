/*
 * hal_usart.c — USART1 @ PA9/PA10 @ 115200
 */
#include "hal_usart.h"
#include "stm32f1xx.h"
#include "hal_gpio.h"

#define BRR_115200_72MHZ  ((39U << 4) | 1U)

void usart1_init_115200(void) {
    /* 1. 开 GPIOA + USART1 时钟 */
    gpio_enable_clock(GPIO_PORT_A);
    RCC_APB2ENR |= RCC_APB2ENR_USART1EN;

    /* 2. PA9 (TX) = 推挽输出, PA10 (RX) = 浮空输入 */
    gpio_config_pin(GPIO_PORT_A, 9, GPIO_MODE_OUTPUT_PP_50M);
    gpio_config_pin(GPIO_PORT_A, 10, GPIO_MODE_INPUT_FLOAT);

    /* 3. 配波特率 + 启用 */
    USART1_BRR = BRR_115200_72MHZ;
    USART1_CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

void usart1_putc(char c) {
    while (!(USART1_SR & USART_SR_TXE));
    USART1_DR = (uint32_t)c;
}

void usart1_puts(const char *s) {
    while (*s) usart1_putc(*s++);
}

void usart1_putu(uint32_t v) {
    char buf[11];
    int i = 0;
    if (v == 0) { usart1_putc('0'); return; }
    while (v) { buf[i++] = '0' + (v % 10); v /= 10; }
    while (i) usart1_putc(buf[--i]);
}
