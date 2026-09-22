/*
 * bsp_nrf24.h — NRF24L01+ 应用层 API
 * 数据手册：nRF24L01P_Product_Specification.pdf 第 6/8/9 章
 */
#ifndef BSP_NRF24_H
#define BSP_NRF24_H

#include <stdint.h>

/* === SPI 命令字（datasheet 表 16）=== */
#define NRF24_CMD_R_REGISTER       0x00   /* + reg_addr */
#define NRF24_CMD_W_REGISTER       0x20   /* + reg_addr + data */
#define NRF24_CMD_R_RX_PAYLOAD     0x61
#define NRF24_CMD_W_TX_PAYLOAD     0xA0
#define NRF24_CMD_FLUSH_TX         0xE1
#define NRF24_CMD_FLUSH_RX         0xE2
#define NRF24_CMD_NOP              0xFF   /* 读 STATUS 用 */

/* === 关键寄存器地址 === */
#define NRF24_REG_CONFIG           0x00
#define NRF24_REG_EN_AA            0x01
#define NRF24_REG_EN_RXADDR        0x02
#define NRF24_REG_SETUP_AW         0x03
#define NRF24_REG_SETUP_RETR       0x04
#define NRF24_REG_RF_CH            0x05
#define NRF24_REG_RF_SETUP         0x06
#define NRF24_REG_STATUS           0x07
#define NRF24_REG_RX_ADDR_P0       0x0A
#define NRF24_REG_TX_ADDR          0x10
#define NRF24_REG_RX_PW_P0         0x11
#define NRF24_REG_FIFO_STATUS      0x17

/* === STATUS 标志位 === */
#define NRF24_STATUS_TX_FULL       (1U << 0)
#define NRF24_STATUS_RX_P_NO       (0x7U << 1)
#define NRF24_STATUS_MAX_RT        (1U << 4)   /* 达到最大重传 */
#define NRF24_STATUS_TX_DS         (1U << 5)   /* 发送完成 */
#define NRF24_STATUS_RX_DR         (1U << 6)   /* 收到数据 */

#define NRF24_ADDR_LEN             5
#define NRF24_MAX_PAYLOAD          32

typedef enum {
    NRF24_MODE_PTX = 0,   /* 主动发送 */
    NRF24_MODE_PRX = 1,   /* 被动接收 */
} nrf24_mode_t;

/* === API === */
uint8_t nrf24_init(nrf24_mode_t mode, uint8_t channel);
void    nrf24_set_mode(nrf24_mode_t mode);
void    nrf24_set_tx_address(const uint8_t *addr5);
void    nrf24_set_rx_address_p0(const uint8_t *addr5);
uint8_t nrf24_read_status(void);
uint8_t nrf24_send(const uint8_t *payload, uint8_t len);
uint8_t nrf24_receive(uint8_t *payload, uint8_t max_len);

#endif
