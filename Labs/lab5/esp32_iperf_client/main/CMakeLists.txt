# EE 250L Lab 05 Starter Code — Fall 2026
#
# Component registration for the `main` component.  Nothing to edit.
#
# Note what is NOT here compared with Lab 4: no `idf_component.yml` and no
# managed component.  The iperf client is ~250 lines of plain lwIP sockets in
# iperf_client.c, so the first build needs no Internet access.

idf_component_register(
    SRCS
        "main.c"
        "wifi_station.c"
        "led.c"
        "button.c"
        "iperf_client.c"
    INCLUDE_DIRS
        "."
    REQUIRES
        nvs_flash            # Wi-Fi driver stores calibration data in NVS
        esp_wifi             # esp_wifi_init / esp_wifi_sta_get_ap_info (RSSI)
        esp_netif            # IP stack glue, esp_netif_create_default_wifi_sta
        esp_event            # the default event loop the Wi-Fi driver uses
        esp_timer            # esp_timer_get_time() for run timing
        esp_driver_gpio      # BOOT button on GPIO 0
        esp_driver_rmt       # drives the WS2812 RGB LED
        lwip                 # lwip/sockets.h — the BSD sockets from Lab 2
        console              # esp_console REPL + argtable3 argument parsing
)
