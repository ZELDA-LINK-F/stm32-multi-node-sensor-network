/*
 * 01_hello_uart — USART Hello World（重构版）
 *
 * 整个工程：USART1 每秒打印一次
 * 全部用 BSP API，main.c 只看流程
 */
#include <stdint.h>
#include "system_stm32f1xx.h"   /* SystemInit 72MHz */
#include "hal_delay.h"          /* delay_init/ms */
#include "hal_usart.h"          /* usart1_init_115200/putc/puts/putu */

int main(void) {
    SystemInit();
    delay_init();
    usart1_init_115200();

    usart1_puts("\r\n=== Hello UART! (refactored) ===\r\n");
    usart1_puts("MCU: STM32F103VET6 @ 72MHz\r\n");
    usart1_puts("USART1: 115200 8N1\r\n\r\n");

    uint32_t count = 0;
    while (1) {
        usart1_puts("Hello UART! count=");
        usart1_putu(count);
        usart1_puts("\r\n");
        count++;
        delay_ms(1000);
    }
}
