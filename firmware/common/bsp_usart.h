/*
 * bsp_usart.h — USART 统一 API
 */
#ifndef BSP_USART_H
#define BSP_USART_H

#include <stdint.h>

/* 默认 USART1 @ 115200, 72MHz */
void usart1_init_115200(void);

/* 输出 */
void usart1_putc(char c);
void usart1_puts(const char *s);
void usart1_putu(uint32_t v);  /* 无符号十进制 */

#endif
