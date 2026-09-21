/*
 * dht11.h — DHT11 温湿度传感器驱动
 * 协议：单总线（开漏 + 外部上拉）
 * 引脚：PA8（CRH 控制，开漏输出 50MHz）
 */
#ifndef DHT11_H
#define DHT11_H

#include <stdint.h>

/* === 引脚定义 === */
#define DHT11_GPIO_CRH_REG     (*(volatile uint32_t *)0x40010804UL)
#define DHT11_GPIO_ODR_REG     (*(volatile uint32_t *)0x4001080CUL)
#define DHT11_GPIO_IDR_REG     (*(volatile uint32_t *)0x40010808UL)
#define DHT11_GPIO_BSRR_REG    (*(volatile uint32_t *)0x40010810UL)
#define DHT11_RCC_IOPAEN_BIT   (1U << 2)
#define DHT11_PIN_NUM          3   /* PA3 - 避开 PA8 */

/* === API === */

/* 初始化 PA8 为开漏输出（外部上拉 4.7kΩ）*/
void dht11_init(void);

/* 读一次温湿度
 * @temp_int: 温度整数（°C）
 * @temp_dec: 温度小数（0.1°C 单位）
 * @humi_int: 湿度整数（%）
 * @humi_dec: 湿度小数（0.1% 单位）
 * 返回 0 = 成功，1 = 失败（无响应 / 校验和错）
 */
uint8_t dht11_read(uint8_t *temp_int, uint8_t *temp_dec,
                   uint8_t *humi_int, uint8_t *humi_dec);

#endif /* DHT11_H */
