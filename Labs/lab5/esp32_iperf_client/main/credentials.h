/* EE 250L Lab 05 Starter Code — Fall 2026
 *
 * credentials.h — the only file in this project that holds configuration
 * specific to YOU and to the network you are sitting on.  Same idea as in
 * Labs 3 and 4: fill in the three values before your first build.
 *
 * DO NOT commit your real Wi-Fi password to a public repository, and remove
 * it from source files and screenshots before you submit.
 */

#pragma once

/* ---------------------------------------------------------------------------
 * Wi-Fi
 * -------------------------------------------------------------------------*/

/* The ESP32-S3 has a 2.4 GHz-only radio and this code only speaks WPA2-PSK,
 * so USC Guest / eduroam (WPA2-Enterprise) will NOT work.  Use a home router
 * or a phone hotspot — and note that the whole point of this lab is to walk
 * *away* from the access point, so a hotspot that you carry in your pocket
 * will not show you anything.  Leave the phone next to the laptop. */
#define WIFI_SSID       "YOUR_WIFI_SSID"
#define WIFI_PASSWORD   "YOUR_WIFI_PASSWORD"

/* ---------------------------------------------------------------------------
 * iperf server
 * -------------------------------------------------------------------------*/

/* The IPv4 address of your Ubuntu VM (`hostname -I`), which runs the iperf2
 * server and stays next to the router.  The VM must be in *bridged*
 * networking mode so that it has an address on the same network as the
 * board, exactly as in Lab 2 Part 3 and Lab 4 Part 3.  This is the address
 * the `measure` command sends to; the `iperf -c <host>` command lets you
 * override it for a single run. */
#define IPERF_SERVER_IP "192.168.1.42"

/* iperf2's default port.  Change it only if you also change it on the VM
 * (`iperf -s -p <port>`). */
#define IPERF_SERVER_PORT 5001
