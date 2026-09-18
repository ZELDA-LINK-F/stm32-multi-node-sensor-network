/*
 * 02_dht11 — DHT11 温湿度传感器（寄存器级，B.2.1）
 *
 * 目标：每 2 秒从 DHT11 读取温湿度，串口打印
 * 设计稿：firmware/02_dht11/docs/DHT11_DESIGN.md
 *
 * 状态：
 *   - ✅ SysTick 1μs 延时框架已就绪
 *   - ✅ 串口 USART1 框架已就绪
 *   - ⏳ DHT11 驱动函数待实现（等硬件 + 72MHz 时钟）
 *
 * B.2.1 完成条件：
 *   1. 时钟配 72MHz（与 B.3 FreeRTOS 一起做）
 *   2. 实现 dht11_read_data()
 *   3. 串口能看到 "T=25 H=60" 循环打印
 */

#include <stdint.h>

/* ============================================================
 * USART1 寄存器（与 01_hello_uart 相同）
 * ============================================================ */
#define RCC_BASE          0x40021000UL
#define RCC_APB2ENR       (*(volatile uint32_t *)(RCC_BASE + 0x18))

#define GPIOA_BASE        0x40010800UL
#define GPIOA_CRH         (*(volatile uint32_t *)(GPIOA_BASE + 0x04))

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

/* ============================================================
 * SysTick 寄存器（RM0008 Ch 12 / ARMv7-M）
 * ============================================================ */
#define SysTick_LOAD     (*(volatile uint32_t *)0xE000E014UL)
#define SysTick_VAL      (*(volatile uint32_t *)0xE000E018UL)
#define SysTick_CTRL     (*(volatile uint32_t *)0xE000E010UL)

/* ============================================================
 * 串口工具
 * ============================================================ */
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
    if (v == 0) { usart1_putc('0'); return; }
    char buf[11]; int i = 0;
    while (v > 0) { buf[i++] = '0' + (v % 10); v /= 10; }
    while (i > 0) usart1_putc(buf[--i]);
}

/* ============================================================
 * SysTick 微秒延时（1μs @ 72MHz 系统时钟）
 * ⚠️  B.2 启动前提：必须先把系统时钟配到 72MHz
 *     当前 B.1 简化版用 HSI 8MHz，delay_us 实际延时 9μs
 * ============================================================ */
static void systick_init_1us(void) {
    SysTick_LOAD = 72 - 1;    /* 72MHz / 72 = 1MHz */
    SysTick_VAL  = 0;
    SysTick_CTRL = 0x05;      /* CLKSOURCE=1, TICKINT=0, ENABLE=1 */
}

static void delay_us(uint32_t us) {
    for (uint32_t i = 0; i < us; i++) {
        SysTick_VAL = 0;
        while (!(SysTick_CTRL & (1 << 16)));  /* 等待 COUNTFLAG */
    }
}

static void delay_ms(uint32_t ms) {
    while (ms--) delay_us(1000);
}

/* ============================================================
 * DHT11 驱动（占位，B.2.1 启动时实现）
 * 详见 firmware/02_dht11/docs/DHT11_DESIGN.md §5
 * ============================================================ */
static int dht11_read_data(uint8_t buf[5]) {
    /* TODO: B.2.1 启动后实现
     *
     * 步骤：
     * 1. 发送起始信号（拉低 20ms）
     * 2. 切输入模式，等待 DHT11 应答（80μs 低 + 80μs 高）
     * 3. 读 40 bit 数据（5 字节）
     * 4. 校验和检查
     *
     * 当前返回 0 = 失败占位
     */
    (void)buf;
    return 0;
}

/* ============================================================
 * main
 * ============================================================ */
int main(void) {
    usart1_init();
    systick_init_1us();

    usart1_puts("\r\n=== DHT11 Driver (register-level) ===\r\n");
    usart1_puts("Phase B.2.1 — placeholder, hardware not yet connected\r\n");
    usart1_puts("See firmware/02_dht11/docs/DHT11_DESIGN.md\r\n\r\n");

    uint8_t data[5] = {0};
    uint32_t count = 0;

    while (1) {
        if (dht11_read_data(data) == 0) {
            /* 占位：暂时打印模拟值 */
            usart1_puts("[");
            usart1_putu(count);
            usart1_puts("] DHT11 not connected yet (TODO)\r\n");
        } else {
            /* 真实读到的数据 */
            usart1_puts("[");
            usart1_putu(count);
            usart1_puts("] T=");
            usart1_putu(data[2]);
            usart1_puts(" H=");
            usart1_putu(data[0]);
            usart1_puts("\r\n");
        }

        count++;
        delay_ms(2000);  /* 每 2 秒一次 */
    }

    return 0;
}
