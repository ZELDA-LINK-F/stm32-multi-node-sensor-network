/*
 * 03_bno055 — BNO055 9 轴姿态传感器（寄存器级 I2C，B.2.2）
 *
 * 目标：每 100ms 读一次欧拉角（roll/pitch/yaw），串口打印
 *
 * 状态：
 *   - ✅ I2C1 寄存器级驱动就绪（bsp_i2c1.c）
 *   - ✅ BNO055 应用层就绪（bsp_bno055.c）
 *   - ✅ USART1 串口输出就绪
 *   - ⏳ 等硬件 + 72MHz 时钟配置
 *
 * B.2.2 完成条件：
 *   1. 时钟配 72MHz（与 B.3 FreeRTOS 一起做）
 *   2. 串口能看到 "R=0.0 P=0.0 Y=0.0" 循环打印
 *   3. 转动 BNO055 → 数值实时变化
 *   4. yaw 长时间不动不漂移（磁力计校准验证）
 */

#include <stdint.h>
#include "bsp_bno055.h"
#include "bsp_i2c1.h"

/* === USART1（复用 01_hello_uart 的）=== */
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

/* === SysTick === */
#define SysTick_LOAD     (*(volatile uint32_t *)0xE000E014UL)
#define SysTick_VAL      (*(volatile uint32_t *)0xE000E018UL)
#define SysTick_CTRL     (*(volatile uint32_t *)0xE000E010UL)

/* === 串口函数 === */
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

static void usart1_puti(int16_t v) {
    if (v < 0) { usart1_putc('-'); v = -v; }
    if (v == 0) { usart1_putc('0'); return; }
    char buf[8]; int i = 0;
    while (v > 0) { buf[i++] = '0' + (v % 10); v /= 10; }
    while (i > 0) usart1_putc(buf[--i]);
}

/* 打印浮点（单位 1/16 度 → 度）*/
static void usart1_put_angle(int16_t raw) {
    /* raw / 16.0 = 度 */
    int16_t integer = raw / 16;
    int16_t frac = (raw < 0 ? -raw : raw) % 16;
    usart1_puti(integer);
    usart1_putc('.');
    if (frac < 10) usart1_putc('0');
    usart1_puti(frac);
}

/* === SysTick 1μs 延时（@ 72MHz）=== */
static void systick_init_1us(void) {
    SysTick_LOAD = 72 - 1;
    SysTick_VAL  = 0;
    SysTick_CTRL = 0x05;
}

static void delay_us(uint32_t us) {
    for (uint32_t i = 0; i < us; i++) {
        SysTick_VAL = 0;
        while (!(SysTick_CTRL & (1U << 16)));
    }
}

static void delay_ms(uint32_t ms) {
    while (ms--) delay_us(1000);
}

/* === main === */
int main(void) {
    usart1_init();
    systick_init_1us();
    i2c1_init();

    usart1_puts("\r\n=== BNO055 Driver (register-level I2C) ===\r\n");
    usart1_puts("Phase B.2.2 — register-level I2C + BNO055\r\n");
    usart1_puts("See firmware/03_bno055/docs/BNO055_DESIGN.md\r\n\r\n");

    /* 探测 BNO055 */
    if (!bno055_init()) {
        usart1_puts("ERROR: BNO055 not detected (check I2C wiring / 4.7k pull-up)\r\n");
        usart1_puts("  - Verify PB6 (SCL) / PB7 (SDA) connected\r\n");
        usart1_puts("  - Verify VCC 3.3V, GND connected\r\n");
        usart1_puts("  - Verify GY-BNO055 ADR=GND (default addr 0x28)\r\n");
        while (1) {
            delay_ms(1000);
            usart1_puts("[retry] BNO055 not found\r\n");
        }
    }

    usart1_puts("BNO055 detected! NDOF mode enabled.\r\n");
    usart1_puts("Reading euler angles (unit: degree)...\r\n\r\n");

    uint32_t count = 0;
    while (1) {
        bno055_euler_t eul;
        if (bno055_read_euler(&eul)) {
            usart1_puts("[");
            usart1_puti((int16_t)count);
            usart1_puts("] Y=");
            usart1_put_angle(eul.heading);
            usart1_puts(" R=");
            usart1_put_angle(eul.roll);
            usart1_puts(" P=");
            usart1_put_angle(eul.pitch);
            usart1_puts("\r\n");
        } else {
            usart1_puts("[");
            usart1_puti((int16_t)count);
            usart1_puts("] read error\r\n");
        }
        count++;
        delay_ms(100);
    }

    return 0;
}
