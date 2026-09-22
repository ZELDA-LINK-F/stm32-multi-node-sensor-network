/*
 * hal_usart.h — USART1 驱动接口（野火指南者 PA9/PA10 @ 115200 8N1）
 *
 * 这一层只管硬件，不依赖 FreeRTOS。
 * 多任务互斥打印请用 app_tasks.h 里的 log_msg()。
 */
#ifndef BSP_USART_H
#define BSP_USART_H

#include <stdint.h>

void hal_usart_init(void);
void hal_usart_putc(char c);
void hal_usart_puts(const char *s);
void hal_usart_putu(uint32_t v);   /* 打印十进制无符号整数 */

#endif
