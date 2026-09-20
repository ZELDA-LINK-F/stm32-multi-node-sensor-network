/*
 * 04_nrf24 — NRF24L01+ PTX 模式测试（B.2.3）
 *
 * 接线（VET6 指南者）：
 *   PA5  ──► NRF24 SCK
 *   PA6  ──► NRF24 MISO
 *   PA7  ──► NRF24 MOSI
 *   PB0  ──► NRF24 CE
 *   PB1  ──► NRF24 CSN
 *   3.3V ──► NRF24 VCC（**必须独立供电或加去耦**）
 *   GND  ──► NRF24 GND
 *
 * 测试流程：
 *   1. 初始化 USART1 115200
 *   2. 初始化 SPI1 + NRF24（PTX 模式，channel 76）
 *   3. 每秒发送 "HELLO NRF24!\n"，打印 TX OK / FAIL
 */

#include <stdint.h>
#include "system_stm32f1xx.h"
#include "bsp_spi1.h"
#include "bsp_nrf24.h"

/* === USART1（复用前 3 个工程的串口代码）=== */
#define USART1_BASE        0x40013800UL
#define USART1_SR          (*(volatile uint32_t *)(USART1_BASE + 0x00))
#define USART1_DR          (*(volatile uint32_t *)(USART1_BASE + 0x04))
#define USART1_BRR         (*(volatile uint32_t *)(USART1_BASE + 0x08))
#define USART1_CR1         (*(volatile uint32_t *)(USART1_BASE + 0x0C))

#define USART_SR_TXE       (1U << 7)
#define USART_CR1_UE       (1U << 13)
#define USART_CR1_TE       (1U << 3)
#define USART_CR1_RE       (1U << 2)

#define USART1_BRR_115200_72MHZ  ((39U << 4) | 1U)

/* SysTick */
#define SysTick_LOAD       (*(volatile uint32_t *)0xE000E014UL)
#define SysTick_VAL        (*(volatile uint32_t *)0xE000E018UL)
#define SysTick_CTRL       (*(volatile uint32_t *)0xE000E010UL)

static void usart1_init(void) {
    /* RCC_APB2ENR IOPA + USART1 */
    *(volatile uint32_t *)0x40021018 |= (1U << 2) | (1U << 14);
    /* PA9 (TX) = AF PP 50MHz, PA10 (RX) = floating input */
    *(volatile uint32_t *)0x40010804 =
        (*(volatile uint32_t *)0x40010804 & ~((0xFU << 4) | (0xFU << 8)))
        | ((0xBU << 4) | (0x4U << 8));
    USART1_BRR = USART1_BRR_115200_72MHZ;
    USART1_CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

static void usart1_putc(char c) {
    while (!(USART1_SR & USART_SR_TXE));
    USART1_DR = (uint32_t)c;
}

static void usart1_puts(const char *s) { while (*s) usart1_putc(*s++); }

static void usart1_putu(uint32_t v) {
    char buf[11]; int i = 0;
    if (v == 0) { usart1_putc('0'); return; }
    while (v) { buf[i++] = '0' + (v % 10); v /= 10; }
    while (i) usart1_putc(buf[--i]);
}

/* SysTick 1ms @ 72MHz */
static void systick_init_1ms(void) {
    SysTick_LOAD = 72000 - 1;
    SysTick_VAL  = 0;
    SysTick_CTRL = 0x05;
}

static void delay_ms(uint32_t ms) {
    while (ms--) {
        SysTick_VAL = 0;
        while (!(SysTick_CTRL & (1U << 16)));
    }
}

/* === main === */
int main(void) {
    usart1_init();
    systick_init_1ms();
    spi1_init();

    usart1_puts("\r\n=== NRF24L01+ Driver (register-level SPI1) ===\r\n");
    usart1_puts("Phase B.2.3 - register-level SPI + NRF24\r\n");
    usart1_puts("Mode: PTX, Channel: 76 (2.476 GHz), 2 Mbps\r\n\r\n");

    /* 探测 NRF24 */
    if (!nrf24_init(NRF24_MODE_PTX, 76)) {
        usart1_puts("ERROR: NRF24 not detected!\r\n");
        usart1_puts("  - Check SPI wiring (PA5/6/7)\r\n");
        usart1_puts("  - Check CE=PB0, CSN=PB1\r\n");
        usart1_puts("  - Check 3.3V power + 10uF decoupling cap\r\n");
        while (1) {
            delay_ms(1000);
            usart1_puts("[retry] NRF24 not found\r\n");
        }
    }

    usart1_puts("NRF24 detected! Sending 'HELLO' every 1s...\r\n");
    usart1_puts("(PTX only — full ping-pong needs 2 boards, see B.5)\r\n\r\n");

    uint32_t count = 0;
    while (1) {
        uint8_t payload[16];
        uint8_t n = 0;
        const char *p = "HELLO #";
        while (*p && n < 14) payload[n++] = (uint8_t)*p++;
        payload[n++] = '0' + (count % 10);
        /* 不强制 \0，NRF24 是二进制数据 */

        uint8_t ok = nrf24_send(payload, n);

        usart1_puts("[");
        usart1_putu(count);
        usart1_puts(ok ? "] TX OK  " : "] TX FAIL");
        usart1_puts("  status=0x");
        usart1_putu(nrf24_read_status());
        usart1_puts("\r\n");

        count++;
        delay_ms(1000);
    }

    return 0;
}
