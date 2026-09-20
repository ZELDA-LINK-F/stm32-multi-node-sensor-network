/*
 * bsp_nrf24_spi.c — ESP-IDF SPI 适配层（NRF24 专用）
 *
 * 引脚（GATEWAY_DESIGN.md §2.1）：
 *   GPIO11 = SCK   GPIO13 = MOSI  GPIO12 = MISO
 *   GPIO10 = CSN   GPIO9  = CE    GPIO8  = IRQ (未用)
 */
#include "bsp_nrf24.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "NRF24_SPI";

#define SPI_BUS_HOST    SPI2_HOST
#define PIN_NUM_MOSI    13
#define PIN_NUM_MISO    12
#define PIN_NUM_SCK     11
#define PIN_NUM_CSN     10
#define PIN_NUM_CE      9

static spi_device_handle_t spi_dev = NULL;

void nrf24_spi_init(void) {
    ESP_LOGI(TAG, "Initializing SPI2 for NRF24...");

    /* 1. 初始化 SPI 总线 */
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_SCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 32,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI_BUS_HOST, &bus_cfg, SPI_DMA_CH_AUTO));

    /* 2. 添加 NRF24 设备（9 MHz，软件 CS）*/
    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = 9 * 1000 * 1000,  /* 9 MHz（NRF24 上限 10MHz）*/
        .mode = 0,                          /* Mode 0（CPOL=0, CPHA=0）*/
        .spics_io_num = -1,                 /* 软件控制 CSN */
        .queue_size = 1,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(SPI_BUS_HOST, &dev_cfg, &spi_dev));

    /* 3. CE 引脚配置 */
    gpio_config_t ce_cfg = {
        .pin_bit_mask = (1ULL << PIN_NUM_CE),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&ce_cfg);
    nrf24_ce_low();

    ESP_LOGI(TAG, "SPI2 + NRF24 ready");
}

uint8_t nrf24_spi_transfer(uint8_t data) {
    uint8_t rx_byte = 0;
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &data,
        .rx_buffer = &rx_byte,
    };
    ESP_ERROR_CHECK(spi_device_polling_transmit(spi_dev, &t));
    return rx_byte;
}

void nrf24_ce_high(void) {
    gpio_set_level(PIN_NUM_CE, 1);
}

void nrf24_ce_low(void) {
    gpio_set_level(PIN_NUM_CE, 0);
}

void nrf24_csn_high(void) {
    /* CSN 软件控制 - 用一个未用的 GPIO 模拟
     * 实际硬件：CSN 接 GPIO10，但 SPI 设备不用硬件 CS
     * 我们直接控制 GPIO10
     */
    static bool csn_gpio_inited = false;
    if (!csn_gpio_inited) {
        gpio_config_t csn_cfg = {
            .pin_bit_mask = (1ULL << PIN_NUM_CSN),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        gpio_config(&csn_cfg);
        csn_gpio_inited = true;
    }
    gpio_set_level(PIN_NUM_CSN, 1);
}

void nrf24_csn_low(void) {
    nrf24_csn_high();   /* 确保 GPIO 初始化 */
    gpio_set_level(PIN_NUM_CSN, 0);
}
