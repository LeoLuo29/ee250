/* EE 250L Lab 05 Starter Code — Fall 2026
 *
 * button.h — SUPPLIED.  The BOOT button, exactly as in Lab 4's
 * board_sensors.c (the temperature sensor is not needed here).
 *
 * No TODOs here.
 */

#pragma once

#include "esp_err.h"

/* The BOOT button is wired to GPIO 0 with a pull-up: HIGH at rest, LOW while
 * held ("active low").  GPIO 0 is also a strapping pin — holding it through a
 * reset enters download mode — so press it *after* boot, not during. */
#define BUTTON_GPIO 0

/** Called from the button task after a debounced press (not from an ISR). */
typedef void (*button_callback_t)(void);

/**
 * @brief Configure GPIO 0 as an input and start the 20 ms polling task.
 */
esp_err_t button_init(button_callback_t cb);
