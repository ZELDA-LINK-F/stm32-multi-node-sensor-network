/*
 * bsp_gpio.h — GPIO 统一 API
 *
 * 不再写裸寄存器！直接调：
 *   gpio_config_output(GPIOA, 3, GPIO_OUT_OD_50M);
 *   gpio_write(GPIOA, 3, 1);   // 高
 *   uint8_t v = gpio_read(GPIOB, 5);
 */
#ifndef BSP_GPIO_H
#define BSP_GPIO_H

#include <stdint.h>
#include "stm32f1xx.h"

/* GPIO 端口 */
typedef enum {
    GPIO_PORT_A = 0,
    GPIO_PORT_B = 1,
} gpio_port_t;

/* GPIO 配置类型 */
typedef enum {
    GPIO_MODE_OUTPUT_PP_50M = GPIO_OUT_PP_50M,  /* 推挽输出 */
    GPIO_MODE_OUTPUT_OD_50M = GPIO_OUT_OD_50M,  /* 开漏输出 */
    GPIO_MODE_INPUT_FLOAT   = GPIO_IN_FLOAT,    /* 浮空输入 */
} gpio_mode_t;

/* API */
void     gpio_enable_clock(gpio_port_t port);
void     gpio_config_pin(gpio_port_t port, uint8_t pin, gpio_mode_t mode);
void     gpio_set_pin(gpio_port_t port, uint8_t pin, uint8_t level);
uint8_t  gpio_get_pin(gpio_port_t port, uint8_t pin);

#endif
