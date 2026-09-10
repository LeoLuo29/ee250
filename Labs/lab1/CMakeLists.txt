idf_component_register(
    SRCS "hello_wifi.c"
    PRIV_REQUIRES esp_wifi esp_event esp_netif nvs_flash
    INCLUDE_DIRS "."
)