/**
 * @file    drv_dht11.h
 * @brief   DHT11 温湿度传感器驱动
 * @details 单总线协议（开漏 + 外部上拉 4.7kΩ）
 *          引脚：PA3（CRL 控制 bit[12:15]）
 * @author  ZELDA-LINK-F
 * @date    2026-09-22
 * @version 1.0
 * 
 * @section 协议时序
 * - 起始：主机拉低 18ms，释放
 * - 响应：DHT11 拉低 80us + 拉高 80us
 * - 数据：40 bit（5 字节），每 bit = 50us低 + 26/70us高（0/1）
 * 
 * @section 已知问题
 * - PA3 不是 5V tolerant（DHT11 必须用 3.3V 供电）
 */

#ifndef DRV_DHT11_H
#define DRV_DHT11_H

#include <stdint.h>

/* === 引脚定义 === */
#define DHT11_PORT  GPIO_PORT_A
#define DHT11_PIN   3   /* PA3 - CRL bit[12:15] */

/**
 * @brief  初始化 DHT11（配置 PA3 开漏输出，外部上拉 4.7kΩ）
 * @note   必须先调用 hal_gpio 启用 GPIOA 时钟
 */
void dht11_init(void);

/**
 * @brief  读一次温湿度数据
 * @param  t_int  输出温度整数（°C）
 * @param  t_dec  输出温度小数（0.1°C）
 * @param  h_int  输出湿度整数（%）
 * @param  h_dec  输出湿度小数（0.1%）
 * @return 0 = 成功，1 = 失败（超时 / 校验和错）
 * @note   调用间隔 ≥ 2 秒（DHT11 采样率限制）
 */
uint8_t dht11_read(uint8_t *t_int, uint8_t *t_dec,
                   uint8_t *h_int, uint8_t *h_dec);

#endif
