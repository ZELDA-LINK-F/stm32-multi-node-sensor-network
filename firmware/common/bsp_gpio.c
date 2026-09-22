/*
 * bsp_gpio.c — GPIO 统一操作
 */
#include "bsp_gpio.h"

/* 端口基地址表（顺序对应 gpio_port_t）*/

static const uint32_t GPIO_CRL_TABLE[] = {
    GPIOA_CRL_ADDR,
    GPIOB_CRL_ADDR,
};

static const uint32_t GPIO_CRH_TABLE[] = {
    GPIOA_CRH_ADDR,
    GPIOB_CRH_ADDR,
};

static const uint32_t GPIO_BSRR_TABLE[] = {
    GPIOA_BSRR_ADDR,
    GPIOB_BSRR_ADDR,
};

static const uint32_t GPIO_IDR_TABLE[] = {
    GPIOA_IDR_ADDR,
    GPIOB_IDR_ADDR,
};

static const uint32_t GPIO_EN_BIT[] = {
    RCC_APB2ENR_IOPAEN,
    RCC_APB2ENR_IOPBEN,
};

/* === 启用端口时钟 === */
void gpio_enable_clock(gpio_port_t port) {
    RCC_APB2ENR |= GPIO_EN_BIT[port];
}

/* === 配置引脚模式（CNF + MODE）=== */
void gpio_config_pin(gpio_port_t port, uint8_t pin, gpio_mode_t mode) {
    volatile uint32_t *cr;
    uint8_t shift;

    if (pin < 8) {
        cr = (volatile uint32_t *)GPIO_CRL_TABLE[port];
        shift = pin * 4;
    } else {
        cr = (volatile uint32_t *)GPIO_CRH_TABLE[port];
        shift = (pin - 8) * 4;
    }
    *cr = (*cr & ~(0xFU << shift)) | ((uint32_t)mode << shift);
}

/* === 写引脚（0 = 低，1 = 高）=== */
void gpio_set_pin(gpio_port_t port, uint8_t pin, uint8_t level) {
    volatile uint32_t *bsrr = (volatile uint32_t *)GPIO_BSRR_TABLE[port];
    if (level) {
        *bsrr = (1U << pin);          /* 高 */
    } else {
        *bsrr = (1U << (pin + 16));   /* 低 */
    }
}

/* === 读引脚 === */
uint8_t gpio_get_pin(gpio_port_t port, uint8_t pin) {
    volatile uint32_t *idr = (volatile uint32_t *)GPIO_IDR_TABLE[port];
    return (*idr >> pin) & 1U;
}
