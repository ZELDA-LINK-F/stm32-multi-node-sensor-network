idf_component_register(
    SRCS "gateway_main.c"
         "bsp_nrf24.c"
         "bsp_nrf24_spi.c"
         "../../common/protocol.c"
    INCLUDE_DIRS "."
                "../../common"
    REQUIRES mqtt esp_wifi esp_event esp_netif driver
)
