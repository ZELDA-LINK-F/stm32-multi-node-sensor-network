idf_component_register(
    SRCS "gateway_main.c"
         "../../common/protocol.c"
    INCLUDE_DIRS "."
                "../../common"
    REQUIRES mqtt esp_wifi esp_event esp_netif
)
