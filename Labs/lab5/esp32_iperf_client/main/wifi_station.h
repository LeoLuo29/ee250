/* EE 250L Lab 05 Starter Code — Fall 2026 (unchanged from Lab 04)
 *
 * wifi_station.h — SUPPLIED.  Bring the ESP32-S3 up as a Wi-Fi station
 * (a "client" that joins an existing access point) and report on the link.
 *
 * There are no TODOs in this module.  It is the same job Lab 3's Wi-Fi setup
 * did, packaged behind three functions so that main.c reads as a sequence of
 * intentions rather than a wall of event-handler boilerplate.
 */

#pragma once

#include <stddef.h>
#include "esp_err.h"

/**
 * @brief Initialise the Wi-Fi driver, associate with WIFI_SSID, and block
 *        until the DHCP server has given us an IPv4 address.
 *
 * Safe to call exactly once, from app_main(), before anything that needs the
 * network. On failure the radio is left running and retrying in the
 * background, so a later call to wifi_get_ip_str() may still succeed.
 *
 * @return ESP_OK on success, ESP_ERR_TIMEOUT if no IP arrived in time,
 *         ESP_FAIL if the association itself kept being refused (wrong
 *         password, or the AP is not there).
 */
esp_err_t wifi_station_connect(void);

/**
 * @brief Copy our current IPv4 address into @p dst as a dotted-quad string.
 *
 * @param dst  destination buffer; 16 bytes is always enough ("255.255.255.255"
 *             plus the terminating NUL).
 * @param n    sizeof(dst).
 * @return ESP_OK, or ESP_ERR_INVALID_STATE if we have no address yet.
 */
esp_err_t wifi_get_ip_str(char *dst, size_t n);

/**
 * @brief Read the received signal strength of the access point we are
 *        associated with, in dBm (a negative number; -30 is excellent,
 *        -80 is barely usable).
 *
 * @param out_rssi  written on success.
 * @return ESP_OK, or an error if we are not associated.
 */
esp_err_t wifi_get_rssi(int *out_rssi);
