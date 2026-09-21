/*
 * dht11.h — DHT11 温湿度传感器驱动
 * 协议：单总线（开漏 + 外部上拉）
 * 引脚：PA3（CRL 控制 bit[12:15]，开漏输出 50MHz）
 */
#ifndef DHT11_H
#define DHT11_H

#include <stdint.h>

/* === 引脚定义 === */
#define DHT11_GPIO_CRL_REG     (*(volatile uint32_t *)0x40010800UL)  /* PA0-7 用 CRL */
#define DHT11_GPIO_ODR_REG     (*(volatile uint32_t *)0x4001080CUL)
#define DHT11_GPIO_IDR_REG     (*(volatile uint32_t *)0x40010808UL)
#define DHT11_GPIO_BSRR_REG    (*(volatile uint32_t *)0x40010810UL)
#define DHT11_RCC_IOPAEN_BIT   (1U << 2)
#define DHT11_PIN_NUM          3   /* PA3 - CRL bit[12:15] */

/* === API === */
void dht11_init(void);
uint8_t dht11_read(uint8_t *temp_int, uint8_t *temp_dec,
                   uint8_t *humi_int, uint8_t *humi_dec);

#endif /* DHT11_H */
