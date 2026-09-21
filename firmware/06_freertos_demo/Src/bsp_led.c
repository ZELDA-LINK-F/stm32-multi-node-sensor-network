#include <stdint.h>

/*
 * bsp_led.c — RGB LED 寄存器版驱动
 */
#include "bsp_led.h"

#define GPIOB_CRL   (*(volatile uint32_t *)0x40010C00UL)
#define GPIOB_CRH   (*(volatile uint32_t *)0x40010C04UL)
#define GPIOB_ODR   (*(volatile uint32_t *)0x40010C0CUL)

void bsp_led_init(void) {
    /* 开 GPIOB 时钟 */
    *(volatile uint32_t *)0x40021018 |= (1U << 3);

    /* PB0/PB1 (CRL) + PB5 (CRH) 配推挽输出 50MHz (MODE=11) */
    GPIOB_CRL = (GPIOB_CRL & ~((0xFU << 0) | (0xFU << 4)))
              | ((0x3U << 0) | (0x3U << 4));
    GPIOB_CRH = (GPIOB_CRH & ~(0xFU << 20))
              | (0x3U << 20);

    /* 默认高电平（LED 灭）*/
    GPIOB_ODR |= (1U << 0) | (1U << 1) | (1U << 5);
}

void bsp_led_red_toggle(void)   { GPIOB_ODR ^= (1U << 5); }
void bsp_led_green_toggle(void) { GPIOB_ODR ^= (1U << 0); }
void bsp_led_blue_toggle(void)  { GPIOB_ODR ^= (1U << 1); }
