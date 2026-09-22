/*
 * bsp_spi1.h — SPI1 寄存器级驱动（RM0008 第 23 章）
 * 用途：驱动 NRF24L01+ 模块（PA5/6/7 = SCK/MISO/MOSI）
 * 速率：fPCLK/8 = 9MHz @ 72MHz（NRF24 上限 10MHz）
 */
#ifndef BSP_SPI1_H
#define BSP_SPI1_H

#include <stdint.h>

void     spi1_init(void);
uint8_t  spi1_transfer(uint8_t data);   /* 全双工收发一字节 */
void     spi1_cs_low(void);             /* CSN 拉低（NRF24 选中） */
void     spi1_cs_high(void);            /* CSN 拉高（NRF24 释放） */

#endif
