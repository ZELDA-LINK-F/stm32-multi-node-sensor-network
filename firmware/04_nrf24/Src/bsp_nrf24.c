/*
 * bsp_nrf24.c — NRF24L01+ 寄存器级应用层
 * 基于 bsp_spi1.h 的全双工 SPI 通信
 */
#include "bsp_nrf24.h"
#include "bsp_spi1.h"

/* === GPIO 操作宏（PB0 = CE）=== */
#define NRF24_CE_HIGH()      do { *(volatile uint32_t *)0x40010C0C |=  (1U << 0); } while (0)
#define NRF24_CE_LOW()       do { *(volatile uint32_t *)0x40010C0C &= ~(1U << 0); } while (0)

/* === 简单延时（约几微秒）=== */
static void delay_us(volatile uint32_t n) { while (n--) __asm__ volatile ("nop"); }

/* ============================================================
 * 底层 SPI 操作
 * ============================================================ */
static uint8_t nrf24_read_reg(uint8_t reg) {
    uint8_t val;
    spi1_cs_low();
    spi1_transfer(NRF24_CMD_R_REGISTER | reg);
    val = spi1_transfer(0xFF);
    spi1_cs_high();
    return val;
}

static void nrf24_write_reg(uint8_t reg, uint8_t val) {
    spi1_cs_low();
    spi1_transfer(NRF24_CMD_W_REGISTER | reg);
    spi1_transfer(val);
    spi1_cs_high();
}

static void nrf24_write_multi(uint8_t reg, const uint8_t *data, uint8_t len) {
    spi1_cs_low();
    spi1_transfer(NRF24_CMD_W_REGISTER | reg);
    for (uint8_t i = 0; i < len; i++) spi1_transfer(data[i]);
    spi1_cs_high();
}


static uint8_t nrf24_cmd(uint8_t cmd) {
    uint8_t status;
    spi1_cs_low();
    status = spi1_transfer(cmd);
    spi1_cs_high();
    return status;
}

/* ============================================================
 * 公共 API
 * ============================================================ */
uint8_t nrf24_read_status(void) {
    return nrf24_cmd(NRF24_CMD_NOP);
}

void nrf24_set_tx_address(const uint8_t *addr5) {
    nrf24_write_multi(NRF24_REG_TX_ADDR, addr5, NRF24_ADDR_LEN);
    nrf24_write_multi(NRF24_REG_RX_ADDR_P0, addr5, NRF24_ADDR_LEN);  /* P0 同地址以支持 auto-ack */
}

void nrf24_set_rx_address_p0(const uint8_t *addr5) {
    nrf24_write_multi(NRF24_REG_RX_ADDR_P0, addr5, NRF24_ADDR_LEN);
}

void nrf24_set_mode(nrf24_mode_t mode) {
    uint8_t cfg = nrf24_read_reg(NRF24_REG_CONFIG);
    if (mode == NRF24_MODE_PRX) {
        cfg |=  (1U << 0);    /* PRIM_RX = 1 */
    } else {
        cfg &= ~(1U << 0);    /* PRIM_RX = 0 */
    }
    nrf24_write_reg(NRF24_REG_CONFIG, cfg);

    /* CE 高 >10us 才进入 RX/TX */
    if (mode == NRF24_MODE_PRX) {
        NRF24_CE_HIGH();
        delay_us(20);
    } else {
        NRF24_CE_LOW();
    }
}

/* ============================================================
 * nrf24_init — 上电配置 NRF24
 *
 * 关键配置：
 *   - 地址长度 5 字节（datasheet 推荐）
 *   - 关掉 auto-ack（简化第一版；后续 B.4 加回来）
 *   - 2 Mbps（RF_DR_HIGH=1）
 *   - 0 dBm 发射功率（RF_PWR=11）
 *   - 16 位 CRC（CRC enabled by CONFIG）
 *
 * 返回：1 = 探测成功，0 = 失败
 * ============================================================ */
uint8_t nrf24_init(nrf24_mode_t mode, uint8_t channel) {
    /* 1. CE 低，进入待机/配置态 */
    NRF24_CE_LOW();
    delay_us(5000);   /* 上电稳定 */

    /* 2. 探测：读 CONFIG 不应为 0xFF（未接）或 0x00 */
    uint8_t cfg = nrf24_read_reg(NRF24_REG_CONFIG);
    if (cfg == 0xFF || cfg == 0x00) return 0;

    /* 3. 清所有状态标志 */
    nrf24_write_reg(NRF24_REG_STATUS, 0x70);

    /* 4. 关 auto-ack / 关重传 / 开 pipe 0 / 5 字节地址 */
    nrf24_write_reg(NRF24_REG_EN_AA,        0x00);  /* 关闭自动应答 */
    nrf24_write_reg(NRF24_REG_EN_RXADDR,    0x01);  /* 只开 pipe 0 */
    nrf24_write_reg(NRF24_REG_SETUP_AW,     0x03);  /* 5 字节地址 */
    nrf24_write_reg(NRF24_REG_SETUP_RETR,   0x00);  /* 不重传（无 auto-ack）*/

    /* 5. RF：2 Mbps + 0 dBm */
    nrf24_write_reg(NRF24_REG_RF_SETUP,     0x0E);

    /* 6. 信道（0~125，freq = 2400 + channel MHz）*/
    if (channel > 125) channel = 76;
    nrf24_write_reg(NRF24_REG_RF_CH, channel);

    /* 7. CONFIG：上电 + CRC(2 字节) + PTX/PRX  */
    uint8_t config_val = 0x0A;   /* PWR_UP=1, CRC=2bytes, PRIM_RX=0 */
    if (mode == NRF24_MODE_PRX) config_val |= 0x01;
    nrf24_write_reg(NRF24_REG_CONFIG, config_val);
    delay_us(1500);   /* PWR_UP -> standby 需 >1.5ms */

    /* 8. 地址配置 */
    uint8_t addr[NRF24_ADDR_LEN] = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7};
    nrf24_set_tx_address(addr);
    nrf24_write_reg(NRF24_REG_RX_PW_P0, NRF24_MAX_PAYLOAD);

    /* 9. 清空 FIFO */
    nrf24_cmd(NRF24_CMD_FLUSH_TX);
    nrf24_cmd(NRF24_CMD_FLUSH_RX);

    /* 10. RX 模式需拉高 CE >10us 进入监听 */
    if (mode == NRF24_MODE_PRX) {
        NRF24_CE_HIGH();
        delay_us(20);
    }

    return 1;
}

/* ============================================================
 * nrf24_send — PTX 模式发送
 *
 * 返回：1 = 发送成功（TX_DS）；0 = 失败（MAX_RT 或超时）
 * ============================================================ */
uint8_t nrf24_send(const uint8_t *payload, uint8_t len) {
    if (len > NRF24_MAX_PAYLOAD) return 0;

    /* 1. 清 TX FIFO，避免残留 */
    nrf24_cmd(NRF24_CMD_FLUSH_TX);
    /* 清状态标志 */
    nrf24_write_reg(NRF24_REG_STATUS, 0x70);

    /* 2. 写 TX payload */
    spi1_cs_low();
    spi1_transfer(NRF24_CMD_W_TX_PAYLOAD);
    for (uint8_t i = 0; i < len; i++) spi1_transfer(payload[i]);
    spi1_cs_high();

    /* 3. 脉冲 CE 高 >10us 触发发送 */
    NRF24_CE_HIGH();
    delay_us(20);
    NRF24_CE_LOW();

    /* 4. 轮询 STATUS：TX_DS 或 MAX_RT 之一置位 */
    uint32_t timeout = 100000;
    uint8_t status;
    do {
        status = nrf24_read_status();
    } while (((status & (NRF24_STATUS_TX_DS | NRF24_STATUS_MAX_RT)) == 0)
             && --timeout);

    /* 5. 清标志 */
    nrf24_write_reg(NRF24_REG_STATUS,
                    NRF24_STATUS_TX_DS | NRF24_STATUS_MAX_RT);

    if (status & NRF24_STATUS_MAX_RT) return 0;  /* 失败 */
    if (status & NRF24_STATUS_TX_DS)  return 1;  /* 成功 */
    return 0;  /* 超时 */
}

/* ============================================================
 * nrf24_receive — PRX 模式读取接收 FIFO
 *
 * 返回：实际读到的字节数，0 = 无数据
 * ============================================================ */
uint8_t nrf24_receive(uint8_t *payload, uint8_t max_len) {
    uint8_t status = nrf24_read_status();
    if (!(status & NRF24_STATUS_RX_DR)) return 0;

    uint8_t len = (max_len > NRF24_MAX_PAYLOAD) ? NRF24_MAX_PAYLOAD : max_len;

    spi1_cs_low();
    spi1_transfer(NRF24_CMD_R_RX_PAYLOAD);
    for (uint8_t i = 0; i < len; i++) payload[i] = spi1_transfer(0xFF);
    spi1_cs_high();

    /* 清 RX_DR */
    nrf24_write_reg(NRF24_REG_STATUS, NRF24_STATUS_RX_DR);

    return len;
}
