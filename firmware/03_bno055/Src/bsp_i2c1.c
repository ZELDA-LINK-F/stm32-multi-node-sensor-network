/*
 * bsp_i2c1.c — I2C1 寄存器级实现
 */
#include "bsp_i2c1.h"

/* === 寄存器定义（RM0008 表 1 + Ch 24）== */
#define RCC_APB2ENR_IOPBEN    (1U << 3)
#define RCC_APB1ENR_I2C1EN    (1U << 21)

#define GPIOB_BASE            0x40010C00UL
#define GPIOB_CRL             (*(volatile uint32_t *)(GPIOB_BASE + 0x00))
#define GPIOB_CRH             (*(volatile uint32_t *)(GPIOB_BASE + 0x04))

#define I2C1_BASE             0x40005400UL
#define I2C1_CR1              (*(volatile uint32_t *)(I2C1_BASE + 0x00))
#define I2C1_CR2              (*(volatile uint32_t *)(I2C1_BASE + 0x04))
#define I2C1_DR               (*(volatile uint32_t *)(I2C1_BASE + 0x10))
#define I2C1_SR1              (*(volatile uint32_t *)(I2C1_BASE + 0x14))
#define I2C1_SR2              (*(volatile uint32_t *)(I2C1_BASE + 0x18))
#define I2C1_CCR              (*(volatile uint32_t *)(I2C1_BASE + 0x1C))
#define I2C1_TRISE            (*(volatile uint32_t *)(I2C1_BASE + 0x20))

#define I2C_SR1_SB            (1U << 0)   /* 起始位已发送 */
#define I2C_SR1_ADDR          (1U << 1)   /* 地址已发送 + ACK */
#define I2C_SR1_BTF           (1U << 2)   /* 字节传输完成 */
#define I2C_SR1_RXNE          (1U << 6)   /* 接收非空 */
#define I2C_SR1_AF             (1U << 10)  /* 应答失败（NACK）*/
#define I2C_SR1_TXE           (1U << 7)   /* 发送空 */
#define I2C_CR1_PE            (1U << 0)
#define I2C_CR1_START         (1U << 8)
#define I2C_CR1_STOP          (1U << 9)
#define I2C_CR1_ACK           (1U << 10)
#define I2C_CR1_SWRST         (1U << 15)

#define RCC_BASE              0x40021000UL
#define RCC_APB1ENR           (*(volatile uint32_t *)(RCC_BASE + 0x1C))

/* I2C1 初始化：100kHz @ 36MHz APB1 */
void i2c1_init(void) {
    /* 1. 开时钟 */
    *(volatile uint32_t *)(RCC_BASE + 0x18) |= RCC_APB2ENR_IOPBEN;
    RCC_APB1ENR |= RCC_APB1ENR_I2C1EN;

    /* 2. PB6/PB7 配复用开漏输出 50MHz（CNF=11, MODE=11 → 0xF）*/
    GPIOB_CRL = (GPIOB_CRL & ~((0xFU << 24) | (0xFU << 28)))
                 | ((0xFU << 24) | (0xFU << 28));

    /* 3. I2C1 配置 */
    I2C1_CR1 |= I2C_CR1_SWRST;   /* 软件复位 */
    I2C1_CR1 &= ~I2C_CR1_SWRST;

    I2C1_CR2 = 36;                /* APB1 时钟 = 36MHz */
    I2C1_CCR = 180;               /* 100kHz @ 36MHz：CCR = 36000000 / (100000 * 2) = 180 */
    I2C1_TRISE = 37;              /* 1000ns / 28ns + 1 = 37 */

    I2C1_CR1 |= I2C_CR1_PE;       /* 使能 I2C1 */
}

/* 等待事件 */
uint8_t i2c1_wait_event(uint32_t event, uint32_t timeout) {
    while (!(I2C1_SR1 & event)) {
        if (--timeout == 0) return 0;
    }
    return 1;
}

/* 起始信号 */
uint8_t i2c1_start(void) {
    I2C1_CR1 |= I2C_CR1_START;
    return i2c1_wait_event(I2C_SR1_SB, I2C_TIMEOUT);
}

/* 停止信号 */
void i2c1_stop(void) {
    I2C1_CR1 |= I2C_CR1_STOP;
}

/* 发送 1 字节，返回 1 = 应答 ACK，0 = 非应答 NACK */
uint8_t i2c1_send_byte(uint8_t data) {
    if (!i2c1_wait_event(I2C_SR1_TXE, I2C_TIMEOUT)) return 0;
    I2C1_DR = data;
    if (!i2c1_wait_event(I2C_SR1_BTF, I2C_TIMEOUT)) return 0;
    /* 检查 AF (Acknowledge Failure) 位 = SR1 bit 10
     *  - AF=1 → 从机发了 NACK → 失败
     *  - AF=0 → 从机发了 ACK  → 成功
     * AF 必须写 0 清，否则下次还认为失败
     */
    if (I2C1_SR1 & I2C_SR1_AF) {
        I2C1_SR1 &= ~I2C_SR1_AF;
        return 0;
    }
    return 1;
}

/* 接收 1 字节，ack=1 发 ACK，ack=0 发 NACK */
uint8_t i2c1_receive_byte(uint8_t ack) {
    if (ack) I2C1_CR1 |= I2C_CR1_ACK;
    else     I2C1_CR1 &= ~I2C_CR1_ACK;

    if (!i2c1_wait_event(I2C_SR1_RXNE, I2C_TIMEOUT)) return 0;
    return (uint8_t)I2C1_DR;
}
