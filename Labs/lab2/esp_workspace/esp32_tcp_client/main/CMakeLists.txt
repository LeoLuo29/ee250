idf_component_register(
    SRCS "esp32_tcp_client.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES esp_wifi esp_event esp_netif nvs_flash lwip
)
