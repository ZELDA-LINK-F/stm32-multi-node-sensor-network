/*
 * bsp_spi1.c — SPI1 寄存器级驱动
 */
#include "drv_spi.h"

/* === RCC === */
#define RCC_BASE             0x40021000UL
#define RCC_APB2ENR          (*(volatile uint32_t *)(RCC_BASE + 0x18))

#define RCC_APB2ENR_IOPAEN   (1U << 2)
#define RCC_APB2ENR_IOPBEN   (1U << 3)
#define RCC_APB2ENR_AFIOEN   (1U << 0)
#define RCC_APB2ENR_SPI1EN   (1U << 12)

/* === GPIO === */
#define GPIOA_BASE           0x40010800UL
#define GPIOA_CRL            (*(volatile uint32_t *)(GPIOA_BASE + 0x00))

#define GPIOB_BASE           0x40010C00UL
#define GPIOB_CRL            (*(volatile uint32_t *)(GPIOB_BASE + 0x00))
#define GPIOB_ODR            (*(volatile uint32_t *)(GPIOB_BASE + 0x0C))

/* === SPI1 === */
#define SPI1_BASE            0x40013000UL
#define SPI1_CR1             (*(volatile uint32_t *)(SPI1_BASE + 0x00))
#define SPI1_SR              (*(volatile uint32_t *)(SPI1_BASE + 0x08))
#define SPI1_DR              (*(volatile uint32_t *)(SPI1_BASE + 0x0C))

/* SPI_CR1 位 */
#define SPI_CR1_MSTR         (1U << 2)   /* 主模式 */
#define SPI_CR1_BR_2         (0x2U << 3) /* baud = fPCLK/8 = 9MHz */
#define SPI_CR1_SPE          (1U << 6)   /* 使能 */
#define SPI_CR1_SSI          (1U << 8)   /* 内部从机选中（软件 NSS 模式必备）*/
#define SPI_CR1_SSM          (1U << 9)   /* 软件从机管理 */

/* SPI_SR 位 */
#define SPI_SR_RXNE          (1U << 0)   /* 接收非空 */
#define SPI_SR_TXE           (1U << 1)   /* 发送空 */
#define SPI_SR_BSY           (1U << 7)   /* 总线忙 */

/* === NRF24 控制引脚（PB0=CE, PB1=CSN）=== */
#define NRF24_CE_HIGH()      do { GPIOB_ODR |=  (1U << 0); } while (0)
#define NRF24_CE_LOW()       do { GPIOB_ODR &= ~(1U << 0); } while (0)
#define NRF24_CSN_HIGH()     do { GPIOB_ODR |=  (1U << 1); } while (0)
#define NRF24_CSN_LOW()      do { GPIOB_ODR &= ~(1U << 1); } while (0)

/* ============================================================
 * SPI1 + GPIO 初始化
 * ============================================================ */
void spi1_init(void) {
    /* 1. 开时钟 */
    RCC_APB2ENR |= RCC_APB2ENR_IOPAEN
                 | RCC_APB2ENR_IOPBEN
                 | RCC_APB2ENR_AFIOEN
                 | RCC_APB2ENR_SPI1EN;

    /* 2. PA5(SCK) PA7(MOSI) = 复用推挽 50MHz；PA6(MISO) = 浮空输入
     *    CNF[1:0] MODE[1:0]
     *    PA5/PA7:  CNF=10 (AF PP), MODE=11 (50MHz)  → 0xB
     *    PA6:      CNF=01 (floating), MODE=00 (input) → 0x4
     */
    GPIOA_CRL = (GPIOA_CRL & ~((0xFU << 20) | (0xFU << 24) | (0xFU << 28)))
              |  ((0xBU << 20) | (0x4U  << 24) | (0xBU  << 28));

    /* 3. PB0(CE) PB1(CSN) = 推挽 50MHz（CNF=00 MODE=11 → 0x3）*/
    GPIOB_CRL = (GPIOB_CRL & ~((0xFU << 0) | (0xFU << 4)))
              |  ((0x3U << 0) | (0x3U  << 4));
    NRF24_CSN_HIGH();   /* 初始未选中 */
    NRF24_CE_LOW();     /* 初始未激活 */

    /* 4. SPI1 配置：主模式 + /8 baud + 软件 NSS + 模式 0 + 8 位 + MSB */
    SPI1_CR1 = SPI_CR1_MSTR | SPI_CR1_BR_2 | SPI_CR1_SSI | SPI_CR1_SSM;
    SPI1_CR1 |= SPI_CR1_SPE;  /* 最后使能 */
}

uint8_t spi1_transfer(uint8_t data) {
    while (!(SPI1_SR & SPI_SR_TXE));   /* 等发送缓冲空 */
    SPI1_DR = data;
    while (!(SPI1_SR & SPI_SR_RXNE));  /* 等接收完成 */
    return (uint8_t)SPI1_DR;
}

void spi1_cs_low(void)  { NRF24_CSN_LOW(); }
void spi1_cs_high(void) { NRF24_CSN_HIGH(); }
