/*
 * bsp_nrf24.c — NRF24L01+ 应用层（ESP32-S3 移植版）
 * 算法与 STM32 04_nrf24 完全一致，只换了 SPI 实现
 */
#include "bsp_nrf24.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "NRF24";

/* === 底层 SPI 操作 === */
static uint8_t nrf24_read_reg(uint8_t reg) {
    uint8_t val;
    nrf24_csn_low();
    nrf24_spi_transfer(NRF24_CMD_R_REGISTER | reg);
    val = nrf24_spi_transfer(0xFF);
    nrf24_csn_high();
    return val;
}

static void nrf24_write_reg(uint8_t reg, uint8_t val) {
    nrf24_csn_low();
    nrf24_spi_transfer(NRF24_CMD_W_REGISTER | reg);
    nrf24_spi_transfer(val);
    nrf24_csn_high();
}

static void nrf24_write_multi(uint8_t reg, const uint8_t *data, uint8_t len) {
    nrf24_csn_low();
    nrf24_spi_transfer(NRF24_CMD_W_REGISTER | reg);
    for (uint8_t i = 0; i < len; i++) nrf24_spi_transfer(data[i]);
    nrf24_csn_high();
}

static uint8_t nrf24_cmd(uint8_t cmd) {
    uint8_t status;
    nrf24_csn_low();
    status = nrf24_spi_transfer(cmd);
    nrf24_csn_high();
    return status;
}

void nrf24_set_tx_address(const uint8_t *addr5) {
    nrf24_write_multi(NRF24_REG_TX_ADDR, addr5, NRF24_ADDR_LEN);
    nrf24_write_multi(NRF24_REG_RX_ADDR_P0, addr5, NRF24_ADDR_LEN);
}

void nrf24_set_rx_address_p0(const uint8_t *addr5) {
    nrf24_write_multi(NRF24_REG_RX_ADDR_P0, addr5, NRF24_ADDR_LEN);
}

void nrf24_set_mode(nrf24_mode_t mode) {
    uint8_t cfg = nrf24_read_reg(NRF24_REG_CONFIG);
    if (mode == NRF24_MODE_PRX) cfg |=  (1U << 0);
    else                        cfg &= ~(1U << 0);
    nrf24_write_reg(NRF24_REG_CONFIG, cfg);

    if (mode == NRF24_MODE_PRX) {
        nrf24_ce_high();
        vTaskDelay(pdMS_TO_TICKS(1));   /* >10us 才进入监听 */
    } else {
        nrf24_ce_low();
    }
}

uint8_t nrf24_read_status(void) {
    return nrf24_cmd(NRF24_CMD_NOP);
}

/* === nrf24_init — 上电配置（与 STM04 一致）=== */
uint8_t nrf24_init(nrf24_mode_t mode, uint8_t channel) {
    /* 初始化 SPI 总线 */
    nrf24_spi_init();

    nrf24_ce_low();
    vTaskDelay(pdMS_TO_TICKS(5));   /* 上电稳定 */

    /* 探测：读 CONFIG 不应为 0xFF/0x00 */
    uint8_t cfg = nrf24_read_reg(NRF24_REG_CONFIG);
    if (cfg == 0xFF || cfg == 0x00) {
        ESP_LOGE(TAG, "NRF24 not detected (CONFIG=0x%02X)", cfg);
        return 0;
    }

    /* 清状态标志 */
    nrf24_write_reg(NRF24_REG_STATUS, 0x70);

    /* 关 auto-ack + 开 pipe0 + 5字节地址 + 不重传 */
    nrf24_write_reg(NRF24_REG_EN_AA,        0x00);
    nrf24_write_reg(NRF24_REG_EN_RXADDR,    0x01);
    nrf24_write_reg(NRF24_REG_SETUP_AW,     0x03);
    nrf24_write_reg(NRF24_REG_SETUP_RETR,   0x00);

    /* 2 Mbps + 0 dBm */
    nrf24_write_reg(NRF24_REG_RF_SETUP,     0x0E);

    /* 信道 */
    if (channel > 125) channel = 76;
    nrf24_write_reg(NRF24_REG_RF_CH, channel);

    /* CONFIG：PWR_UP + CRC(2字节) + PTX/PRX */
    uint8_t config_val = 0x0A;
    if (mode == NRF24_MODE_PRX) config_val |= 0x01;
    nrf24_write_reg(NRF24_REG_CONFIG, config_val);
    vTaskDelay(pdMS_TO_TICKS(2));   /* PWR_UP -> standby 需 >1.5ms */

    /* 地址（默认 E7E7E7E7E7）*/
    uint8_t addr[NRF24_ADDR_LEN] = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7};
    nrf24_set_tx_address(addr);
    nrf24_write_reg(NRF24_REG_RX_PW_P0, NRF24_MAX_PAYLOAD);

    /* 清 FIFO */
    nrf24_cmd(NRF24_CMD_FLUSH_TX);
    nrf24_cmd(NRF24_CMD_FLUSH_RX);

    if (mode == NRF24_MODE_PRX) {
        nrf24_ce_high();
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    ESP_LOGI(TAG, "NRF24 initialized, mode=%d, channel=%d", mode, channel);
    return 1;
}

/* === nrf24_send — PTX 发送 === */
uint8_t nrf24_send(const uint8_t *payload, uint8_t len) {
    if (len > NRF24_MAX_PAYLOAD) return 0;

    nrf24_cmd(NRF24_CMD_FLUSH_TX);
    nrf24_write_reg(NRF24_REG_STATUS, 0x70);

    /* 写 TX payload */
    nrf24_csn_low();
    nrf24_spi_transfer(NRF24_CMD_W_TX_PAYLOAD);
    for (uint8_t i = 0; i < len; i++) nrf24_spi_transfer(payload[i]);
    nrf24_csn_high();

    /* 脉冲 CE */
    nrf24_ce_high();
    vTaskDelay(pdMS_TO_TICKS(1));
    nrf24_ce_low();

    /* 轮询 STATUS */
    uint32_t timeout = 5000;   /* 5ms 超时 */
    uint8_t status;
    do {
        status = nrf24_read_status();
    } while (((status & (NRF24_STATUS_TX_DS | NRF24_STATUS_MAX_RT)) == 0) && --timeout);

    /* 清标志 */
    nrf24_write_reg(NRF24_REG_STATUS,
                    NRF24_STATUS_TX_DS | NRF24_STATUS_MAX_RT);

    if (status & NRF24_STATUS_MAX_RT) {
        nrf24_cmd(NRF24_CMD_FLUSH_TX);
        return 0;
    }
    if (status & NRF24_STATUS_TX_DS) return 1;
    return 0;
}

/* === nrf24_receive — PRX 读取 === */
uint8_t nrf24_receive(uint8_t *payload, uint8_t max_len) {
    uint8_t status = nrf24_read_status();
    if (!(status & NRF24_STATUS_RX_DR)) return 0;

    uint8_t len = (max_len > NRF24_MAX_PAYLOAD) ? NRF24_MAX_PAYLOAD : max_len;

    nrf24_csn_low();
    nrf24_spi_transfer(NRF24_CMD_R_RX_PAYLOAD);
    for (uint8_t i = 0; i < len; i++) payload[i] = nrf24_spi_transfer(0xFF);
    nrf24_csn_high();

    nrf24_write_reg(NRF24_REG_STATUS, NRF24_STATUS_RX_DR);

    return len;
}
