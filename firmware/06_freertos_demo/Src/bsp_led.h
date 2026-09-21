/*
 * bsp_led.h — RGB LED 驱动（野火指南者：红 PB5 / 绿 PB0 / 蓝 PB1）
 *
 * 共阴极，低电平点亮。
 */
#ifndef BSP_LED_H
#define BSP_LED_H

void bsp_led_init(void);
void bsp_led_red_toggle(void);
void bsp_led_green_toggle(void);
void bsp_led_blue_toggle(void);

#endif
