/*
 * 02_dht11 — DHT11 温湿度采集（重构版）
 *
 * 整个工程只做 3 件事：
 *   1. 初始化 USART + 延时 + DHT11
 *   2. 每 2 秒读一次 DHT11
 *   3. 串口打印结果
 *
 * 所有底层细节都在 firmware/common/ 里（BSP 层）
 */
#include <stdint.h>
#include "system_stm32f1xx.h"     /* SystemInit（72MHz）*/
#include "hal_delay.h"            /* delay_init/us/ms */
#include "hal_usart.h"            /* usart1_init_115200/putc/puts/putu */
#include "dht11.h"                /* dht11_init/read */

int main(void) {
    /* === 1. 初始化 === */
    SystemInit();          /* 72MHz 时钟 */
    delay_init();          /* DWT 周期计数器 */
    usart1_init_115200();  /* USART1 @ 115200 */
    dht11_init();          /* PA3 开漏输出 + 4.7kΩ 上拉 */

    usart1_puts("\r\n=== DHT11 Driver (refactored) ===\r\n");
    usart1_puts("Phase B.2.1 - register-level single-bus\r\n");
    usart1_puts("DATA = PA3 (open-drain, 4.7k pull-up)\r\n\r\n");

    /* === 2. 主循环 === */
    uint32_t count = 0;
    while (1) {
        uint8_t t_int, t_dec, h_int, h_dec;
        if (dht11_read(&t_int, &t_dec, &h_int, &h_dec) == 0) {
            /* 成功 */
            usart1_puts("[");
            usart1_putu(count);
            usart1_puts("] Temp=");
            usart1_putu(t_int); usart1_putc('.');
            usart1_putu(t_dec);
            usart1_puts(" C  Humi=");
            usart1_putu(h_int); usart1_putc('.');
            usart1_putu(h_dec);
            usart1_puts(" %\r\n");
        } else {
            usart1_puts("[");
            usart1_putu(count);
            usart1_puts("] DHT11 read FAILED (check wiring)\r\n");
        }
        count++;
        delay_ms(2000);  /* 每 2 秒一次 */
    }
}
