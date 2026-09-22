/*
 * bsp_i2c1.h — I2C1 寄存器级驱动（RM0008 第 24 章）
 * 100kHz 速率 @ 36MHz APB1（72MHz 系统时钟 / 2）
 */
#ifndef BSP_I2C1_H
#define BSP_I2C1_H

#include <stdint.h>

void     i2c1_init(void);
uint8_t  i2c1_start(void);
void     i2c1_stop(void);
uint8_t  i2c1_send_byte(uint8_t data);
uint8_t  i2c1_receive_byte(uint8_t ack);
uint8_t  i2c1_wait_event(uint32_t event, uint32_t timeout);

#define I2C_TIMEOUT  100000

#endif
