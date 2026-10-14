/* EE 250L Lab 05 Starter Code — Fall 2026 (unchanged from Lab 04)
 *
 * led.h — SUPPLIED.  The single addressable RGB LED on the DevKitC-1.
 *
 * No TODOs here.  You call led_set_color_name() from main.c; how the bits
 * get onto the wire is led.c's problem.
 */

#pragma once

#include <stdbool.h>
#include "esp_err.h"

/* -----------------------------------------------------------------------
 * Which pin is the LED on?
 * -----------------------------------------------------------------------
 * The onboard WS2812 ("NeoPixel") moved between board revisions:
 *
 *     ESP32-S3-DevKitC-1  v1.1 and later  ->  GPIO 38   (default below)
 *     ESP32-S3-DevKitC-1  v1.0            ->  GPIO 48
 *
 * The revision is silkscreened on the underside of the board, near the USB
 * connectors. If you set a colour and nothing lights up but the serial log
 * says "LED -> red", change this to 48 and rebuild — that is the single most
 * common cause, and it costs one `idf.py build` to rule out.
 */
#ifndef LED_GPIO
#define LED_GPIO 38
#endif

/**
 * @brief Claim the RMT channel that drives the LED and switch the LED off.
 *
 * Call once from app_main() before any led_set_color_name().
 */
esp_err_t led_init(void);

/**
 * @brief Set the LED from a colour name.
 *
 * Recognised names (case-insensitive): "off", "red", "green", "blue",
 * "white". These five strings are the payload vocabulary of the
 * `<user>/esp32/led` topic, so this function is also the input validator for
 * anything that arrives from the broker — which is exactly why it returns a
 * bool instead of silently ignoring nonsense.
 *
 * @param name NUL-terminated colour name. Must be NUL-terminated: an MQTT
 *             payload is not, so copy it first.
 * @return true if the name was recognised and the LED was updated;
 *         false if the name is unknown (the LED is left as it was).
 */
bool led_set_color_name(const char *name);

/**
 * @brief The name most recently accepted by led_set_color_name(), or "off".
 *
 * Useful when you want to publish the LED's state without having to remember
 * what you set it to.
 */
const char *led_get_color_name(void);
