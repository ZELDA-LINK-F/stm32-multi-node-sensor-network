/*
 * 02_dht11 — DHT11 温湿度传感器（寄存器级单总线，B.2.1）
 * 目标：每 2 秒从 DHT11 读取温湿度，串口打印
 * 数据线：PA8（开漏输出 + 外部 4.7kΩ 上拉）
 *
 * 状态：✅ 寄存器级单总线驱动完整实现（dht11.c/h）
 *       ⏳ 等硬件接线验证
 *
 * 简历可写：
 *   ✅ "STM32F103 寄存器级实现 DHT11 单总线协议（PA8 开漏 + 5 字节帧解析）"
 *   ✅ "无任何外部库，~140 行 C 代码完成时序控制 + 校验和"
 */

#include <stdint.h>
#include "system_stm32f1xx.h"
#include "dht11.h"

/* === USART1（PA9=TX @ 115200）=== */
#define RCC_BASE          0x40021000UL
#define RCC_APB2ENR       (*(volatile uint32_t *)(RCC_BASE + 0x18))
#define GPIOA_CRH         (*(volatile uint32_t *)(GPIOA_BASE + 0x04))
#define GPIOA_BASE        0x40010800UL
#define USART1_BASE       0x40013800UL
#define USART1_SR         (*(volatile uint32_t *)(USART1_BASE + 0x00))
#define USART1_DR         (*(volatile uint32_t *)(USART1_BASE + 0x04))
#define USART1_BRR        (*(volatile uint32_t *)(USART1_BASE + 0x08))
#define USART1_CR1        (*(volatile uint32_t *)(USART1_BASE + 0x0C))

#define RCC_APB2ENR_IOPAEN     (1U << 2)
#define RCC_APB2ENR_USART1EN   (1U << 14)
#define USART_SR_TXE           (1U << 7)
#define USART_CR1_UE           (1U << 13)
#define USART_CR1_TE           (1U << 3)
#define USART_CR1_RE           (1U << 2)
#define USART1_BRR_115200_72MHZ  ((39U << 4) | 1U)

static void usart1_init(void) {
    RCC_APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_USART1EN;
    GPIOA_CRH = (GPIOA_CRH & ~((0xFU << 4) | (0xFU << 8)))
                | ((0xBU << 4) | (0x4U << 8));
    USART1_BRR = USART1_BRR_115200_72MHZ;
    USART1_CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

static void usart1_putc(char c) {
    while (!(USART1_SR & USART_SR_TXE));
    USART1_DR = (uint32_t)c;
}

static void usart1_puts(const char *s) {
    while (*s) usart1_putc(*s++);
}

static void usart1_putu(uint32_t v) {
    char buf[11]; int i = 0;
    if (v == 0) { usart1_putc('0'); return; }
    while (v) { buf[i++] = '0' + (v % 10); v /= 10; }
    while (i) usart1_putc(buf[--i]);
}

/* SysTick 1ms 延时（@ 72MHz） */
static void delay_ms(uint32_t ms) {
    volatile uint32_t *val = (volatile uint32_t *)0xE000E018UL;
    volatile uint32_t *ctrl = (volatile uint32_t *)0xE000E010UL;
    /* 先确保 SysTick 在跑（24MHz 计数 1ms = 24000）*/
    static uint8_t inited = 0;
    if (!inited) {
        *(volatile uint32_t *)0xE000E014UL = 72000 - 1;
        *val = 0;
        *ctrl = 0x05;
        inited = 1;
    }
    while (ms--) {
        *val = 0;
        while (!(*ctrl & (1U << 16)));
    }
}

/* === main === */
int main(void) {
    SystemInit();           /* 72MHz 时钟 */
    usart1_init();
    dht11_init();           /* 初始化 PA8 单总线 */

    usart1_puts("\r\n=== DHT11 Driver (PA8 single-bus) ===\r\n");
    usart1_puts("Phase B.2.1 - register-level single-bus driver\r\n");
    usart1_puts("DATA = PA8 (open-drain, external 4.7k pull-up)\r\n\r\n");

    uint32_t count = 0;
    while (1) {
        uint8_t t_int, t_dec, h_int, h_dec;
        if (dht11_read(&t_int, &t_dec, &h_int, &h_dec) == 0) {
            usart1_puts("[");
            usart1_putu(count);
            usart1_puts("] Temp=");
            usart1_putu(t_int);
            usart1_puts(".");
            usart1_putu(t_dec);
            usart1_puts(" C  Humi=");
            usart1_putu(h_int);
            usart1_puts(".");
            usart1_putu(h_dec);
            usart1_puts(" %\r\n");
        } else {
            usart1_puts("[");
            usart1_putu(count);
            usart1_puts("] DHT11 read FAILED (check wiring)\r\n");
        }

        count++;
        delay_ms(2000);
    }

    return 0;
}
