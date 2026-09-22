/*
 * 04_nrf24 — NRF24L01+ SPI 测试（BSP 重构版）
 *
 * 流程：
 *   1. 初始化 USART + NRF24
 *   2. 每秒发送一次 "HELLO"
 *   3. 串口打印发送状态
 */
#include <stdint.h>
#include "system_stm32f1xx.h"
#include "hal_delay.h"
#include "hal_usart.h"
#include "bsp_nrf24.h"

int main(void) {
    SystemInit();
    delay_init();
    usart1_init_115200();

    usart1_puts("\r\n=== NRF24L01+ Driver (BSP refactored) ===\r\n");
    usart1_puts("Phase B.2.3 - register-level SPI1\r\n");
    usart1_puts("SCK=PA5 MOSI=PA7 MISO=PA6 CSN=PB1 CE=PB0\r\n\r\n");

    /* 初始化 NRF24 为 PTX（发送方），信道 76 */
    if (!nrf24_init(NRF24_MODE_PTX, 76)) {
        usart1_puts("ERROR: NRF24 not detected\r\n");
        while (1) {
            delay_ms(1000);
            usart1_puts("[retry] NRF24 not found\r\n");
        }
    }
    usart1_puts("NRF24 initialized. PTX mode, channel 76.\r\n");
    usart1_puts("Sending 'HELLO' every 1 second...\r\n\r\n");

    uint32_t count = 0;
    while (1) {
        const char *msg = "HELLO ";
        uint8_t ok = nrf24_send((const uint8_t *)msg, 6);

        usart1_puts("[");
        usart1_putu(count);
        usart1_puts(ok ? "] TX OK  " : "] TX FAIL");
        usart1_puts("  status=0x");
        usart1_putu(nrf24_read_status());
        usart1_puts("\r\n");

        count++;
        delay_ms(1000);
    }
}
