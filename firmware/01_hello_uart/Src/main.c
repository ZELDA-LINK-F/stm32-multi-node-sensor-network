/*
 * 01_hello_uart — 寄存器级 USART1 串口打印
 * 参考手册：RM0008 第 25 章 USART
 * 目标芯片：STM32F103VET6（Cortex-M3, 72MHz, 512KB Flash, 64KB RAM）
 * 串口：USART1 @ PA9 (TX) / PA10 (RX), 115200 8N1
 *
 * 简历可写：从 RM0008 第 25 章直接配 USART 寄存器，
 *         不依赖 HAL 库，编译产物 < 5KB。
 */

#include <stdint.h>

/* ============================================================
 * 1. 寄存器定义（按 RM0008 表 1 存储器映射）
 * ============================================================ */

/* RCC 基址 0x40021000 */
#define RCC_BASE          0x40021000UL
#define RCC_APB2ENR       (*(volatile uint32_t *)(RCC_BASE + 0x18))

/* GPIOA 基址 0x40010800 */
#define GPIOA_BASE        0x40010800UL
#define GPIOA_CRL         (*(volatile uint32_t *)(GPIOA_BASE + 0x00))
#define GPIOA_CRH         (*(volatile uint32_t *)(GPIOA_BASE + 0x04))
#define GPIOA_ODR         (*(volatile uint32_t *)(GPIOA_BASE + 0x0C))

/* USART1 基址 0x40013800 */
#define USART1_BASE       0x40013800UL
#define USART1_SR         (*(volatile uint32_t *)(USART1_BASE + 0x00))
#define USART1_DR         (*(volatile uint32_t *)(USART1_BASE + 0x04))
#define USART1_BRR        (*(volatile uint32_t *)(USART1_BASE + 0x08))
#define USART1_CR1        (*(volatile uint32_t *)(USART1_BASE + 0x0C))
#define USART1_CR2        (*(volatile uint32_t *)(USART1_BASE + 0x10))
#define USART1_CR3        (*(volatile uint32_t *)(USART1_BASE + 0x14))

/* 位定义 */
#define RCC_APB2ENR_IOPAEN     (1U << 2)
#define RCC_APB2ENR_USART1EN   (1U << 14)

#define USART_SR_TXE           (1U << 7)   /* 发送数据寄存器空 */
#define USART_SR_TC            (1U << 6)   /* 发送完成 */
#define USART_CR1_UE           (1U << 13)  /* USART 使能 */
#define USART_CR1_TE           (1U << 3)   /* 发送使能 */
#define USART_CR1_RE           (1U << 2)   /* 接收使能 */

/* ============================================================
 * 2. USART1 波特率计算（RM0008 25.3.4 Fractional baud rate generation）
 * ============================================================
 * 公式：USARTDIV = fck / (16 * baud)
 * 例：fck = 72MHz, baud = 115200
 *     USARTDIV = 72000000 / (16 * 115200) = 39.0625
 *     BRR = mantissa[15:4] | fraction[3:0]
 *         = (39 << 4) | 1 = 0x271
 */
#define USART1_BRR_115200_72MHZ  ((39U << 4) | 1U)  /* 0x271 = 625 */

/* ============================================================
 * 3. 工具函数
 * ============================================================ */

/* 阻塞延时（粗略，约 n 个 CPU 周期） */
static void delay_loop(volatile uint32_t n) {
    while (n--) {
        __asm__ volatile ("nop");
    }
}

/* USART1 发送一个字符（轮询 TXE 位） */
static void usart1_putc(char c) {
    while (!(USART1_SR & USART_SR_TXE)) {
        /* 等待 TXE=1，即数据寄存器空，可以写入下一个字符 */
    }
    USART1_DR = (uint32_t)c;
}

/* USART1 发送字符串 */
static void usart1_puts(const char *s) {
    while (*s) {
        usart1_putc(*s++);
    }
}

/* USART1 发送一个十进制无符号整数 */
static void usart1_putu(uint32_t v) {
    char buf[11];   /* 最多 10 位 + '\0' */
    int i = 0;

    if (v == 0) {
        usart1_putc('0');
        return;
    }

    while (v > 0) {
        buf[i++] = '0' + (v % 10);
        v /= 10;
    }
    while (i > 0) {
        usart1_putc(buf[--i]);
    }
}

/* ============================================================
 * 4. USART1 初始化（寄存器级）
 * ============================================================ */
static void usart1_init(void) {
    /* 4.1 开外设时钟（RCC_APB2ENR） */
    RCC_APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_USART1EN;

    /* 4.2 配置 PA9 (TX): 复用推挽输出，50MHz
     *     配置 PA10 (RX): 浮空输入
     *     GPIOA_CRH 控制 PA8-PA15，每引脚 4 位
     *       CNF[1:0] MODE[1:0]
     *       PA9:  CNF=10 (AF PP), MODE=11 (50MHz)  → 0xB << 4
     *       PA10: CNF=01 (floating), MODE=00 (input) → 0x4 << 8
     */
    GPIOA_CRH = (GPIOA_CRH & ~((0xFU << 4) | (0xFU << 8)))
                | ((0xBU << 4) | (0x4U << 8));

    /* 4.3 设置波特率（在使能 USART 前） */
    USART1_BRR = USART1_BRR_115200_72MHZ;

    /* 4.4 使能 USART1、发送器、接收器
     *     默认：8N1（CR1 默认 M=0 → 8 位，PCE=0 → 无校验）
     *           1 个停止位（CR2 默认 STOP=00）
     */
    USART1_CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;

    /* 注意：fck = 72MHz 来自外部 8MHz HSE × 9 倍频
     *       这部分 startup + SystemInit 在 startup_stm32f103vctx.s 里做 */
}

/* ============================================================
 * 5. main
 * ============================================================ */
int main(void) {
    usart1_init();

    usart1_puts("\r\n=== Hello UART! (register-level) ===\r\n");
    usart1_puts("MCU: STM32F103VET6 @ 72MHz\r\n");
    usart1_puts("USART1: 115200 8N1\r\n\r\n");

    uint32_t count = 0;
    while (1) {
        usart1_puts("Hello UART! count=");
        usart1_putu(count);
        usart1_puts("\r\n");
        count++;
        delay_loop(8000000);   /* 72MHz 下约 1 秒 */
    }

    /* 不应该到达这里 */
    return 0;
}
