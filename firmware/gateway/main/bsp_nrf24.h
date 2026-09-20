/*
 * bsp_nrf24.h — NRF24L01+ 寄存器级驱动（ESP32-S3 移植版）
 *
 * 与 firmware/04_nrf24/Src/bsp_nrf24.h API 完全一致
 * 区别：底层 SPI 用 ESP-IDF spi_device_polling_transmit
 *       不再用 STM32 寄存器宏，而是函数指针
 */
#ifndef BSP_NRF24_H
#define BSP_NRF24_H

#include <stdint.h>

/* === SPI 命令字（datasheet 表 16）=== */
#define NRF24_CMD_R_REGISTER       0x00
#define NRF24_CMD_W_REGISTER       0x20
#define NRF24_CMD_R_RX_PAYLOAD     0x61
#define NRF24_CMD_W_TX_PAYLOAD     0xA0
#define NRF24_CMD_FLUSH_TX         0xE1
#define NRF24_CMD_FLUSH_RX         0xE2
#define NRF24_CMD_NOP              0xFF

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

/* === STATUS 标志 === */
#define NRF24_STATUS_MAX_RT        (1U << 4)
#define NRF24_STATUS_TX_DS         (1U << 5)
#define NRF24_STATUS_RX_DR         (1U << 6)

#define NRF24_ADDR_LEN             5
#define NRF24_MAX_PAYLOAD          32

typedef enum {
    NRF24_MODE_PTX = 0,
    NRF24_MODE_PRX = 1,
} nrf24_mode_t;

/* === 平台抽象层 === */
void     nrf24_spi_init(void);          /* 初始化 SPI 总线 */
uint8_t  nrf24_spi_transfer(uint8_t data); /* 全双工收发一字节 */
void     nrf24_ce_high(void);
void     nrf24_ce_low(void);
void     nrf24_csn_high(void);
void     nrf24_csn_low(void);

/* === NRF24 API === */
uint8_t nrf24_init(nrf24_mode_t mode, uint8_t channel);
void    nrf24_set_mode(nrf24_mode_t mode);
void    nrf24_set_tx_address(const uint8_t *addr5);
void    nrf24_set_rx_address_p0(const uint8_t *addr5);
uint8_t nrf24_read_status(void);
uint8_t nrf24_send(const uint8_t *payload, uint8_t len);
uint8_t nrf24_receive(uint8_t *payload, uint8_t max_len);

#endif /* BSP_NRF24_H */
