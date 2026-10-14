# EE 250L Lab 05 Starter Code — Fall 2026

## ESP32-S3 iperf client

The ESP32-S3 half of Lab 5. Last year the Raspberry Pi was the iperf *client*
that you carried away from the router while a laptop VM next to the router
ran the iperf *server*. This year the ESP32-S3 is the client. The VM's job is
unchanged, except that it runs **iperf 2** (`sudo apt install iperf`), because
that is the protocol the board speaks. iperf3 will not work.

Target: **ESP32-S3-DevKitC-1**, **ESP-IDF v6.0.2** (v5.3 also works), C.

---

## 1. What is supplied, and what you write

Nothing in this project has a TODO. Like last year's `iperf_logger.sh`, the
firmware is supplied complete: fill in `main/credentials.h`, build, flash, and
use it. You are still expected to have read `main/iperf_client.c` — the
interview asks how the measurements are made, and that file is the answer.

| File | Status | What it does |
|---|---|---|
| `main/credentials.h` | **fill in 3 values** | Wi-Fi SSID/password, the VM's IP address |
| `main/wifi_station.c/.h` | supplied (same as Lab 4) | Station mode, retry with backoff, blocks until DHCP gives us an IP; `wifi_get_rssi()` |
| `main/led.c/.h` | supplied (same as Lab 4) | Onboard WS2812 RGB LED; used as a status light |
| `main/button.c/.h` | supplied | BOOT-button task with debounce (Lab 4's `board_sensors.c` without the temperature sensor) |
| `main/iperf_client.c/.h` | supplied | iperf2-compatible TCP and UDP client on plain lwIP sockets |
| `main/main.c` | supplied | `app_main()`: NVS → Wi-Fi → LED → button → serial console with `iperf`, `measure`, `rssi` |
| `main/CMakeLists.txt`, `CMakeLists.txt`, `sdkconfig.defaults` | supplied | Build configuration. No managed components: the first build needs no Internet access |

---

## 2. Before your first build

1. **Fill in `main/credentials.h`.**

   ```c
   #define WIFI_SSID        "YOUR_WIFI_SSID"
   #define WIFI_PASSWORD    "YOUR_WIFI_PASSWORD"
   #define IPERF_SERVER_IP  "192.168.1.42"      // your VM's `hostname -I`
   ```

   The ESP32-S3 radio is **2.4 GHz only** and this code only speaks WPA2-PSK,
   so eduroam / USC Guest will not work. Use a home router or a phone
   hotspot — and leave the hotspot next to the laptop, since the whole lab is
   about walking *away* from the access point.

2. **The VM must be reachable from the board.** Same requirements as Lab 2
   Part 3 and Lab 4 Part 3: bridged networking, `hostname -I` for the
   address, and `sudo ufw allow 5001` if the firewall is on (both TCP and
   UDP; `sudo ufw disable` during the lab is also fine).

3. **Check your board revision.** The RGB LED is on **GPIO 38** on
   DevKitC-1 v1.1 and later and on **GPIO 48** on v1.0. If the log says the
   LED changed colour but nothing lights up, set `LED_GPIO` to 48 in
   `main/led.h`. The LED is only a status light in this lab, so this is
   cosmetic.

---

## 3. Build, flash, monitor

```bash
. $HOME/esp/esp-idf/export.sh        # once per terminal
idf.py set-target esp32s3            # once per project
idf.py build
idf.py -p <PORT> flash               # see the port names below
idf.py -p <PORT> monitor             # then press RESET once; Ctrl-] exits
```

Port names: the board has two USB connectors and either works. **UART**
connector → `/dev/ttyUSB0` (Linux), `/dev/cu.usbserial-XXXX` (macOS), `COMx`
(Windows). **USB** connector → `/dev/ttyACM0`, `/dev/cu.usbmodemXXXX`, `COMx`.
Keep the default `sdkconfig` (do not change the console channel in
menuconfig): the default sends the console to both connectors and the prompt
accepts typed commands on either.

If flashing says `No serial data received` (usual on the USB connector), put
the chip in download mode by hand: hold **BOOT**, tap **RESET**, release
**BOOT**, and run the flash command again within a few seconds. Afterwards
press **RESET** so the board boots normally.

Tested with ESP-IDF v6.0.2 (course install) and v5.3.

---

## 4. Using the board

After Wi-Fi comes up you get a prompt in the monitor window. Type at it.

```
ESP32-S3 iperf client ready.  Board IP 192.168.1.50, server 192.168.1.42:5001
Commands:  iperf -c <host> [-u] [-b 10M] [-t 10]   |   measure [<host>]   |   rssi   |   help
(or press the BOOT button to run measure with no laptop attached)

esp32>
```

| Command | What it does | Last year's equivalent |
|---|---|---|
| `iperf -c 192.168.1.42 -t 10` | one 10 s TCP run | `iperf3 -c <VM IP> -t 10` on the RPi |
| `iperf -c 192.168.1.42 -u -b 10M -t 10` | one 10 s UDP run at 10 Mbit/s | `iperf3 -c <VM IP> -u -b 10M -t 10` on the RPi |
| `measure` | 5 TCP runs, then 5 UDP runs, 10 s each, 1 s apart | `./iperf_logger.sh <distance>` on the RPi |
| `rssi` | current signal strength in dBm | — |
| **BOOT button** | same as `measure` | — |

`measure` prints every run and a sender-side summary. The CSV files you plot
are written by **`iperf_logger.sh` on the VM**, which is where the receiver
throughput and the UDP packet loss are known. Start the VM script *first*
(`./iperf_logger.sh 2`), then run `measure` (or press BOOT); the script
returns when it has seen 5 TCP and 5 UDP runs.

The LED shows what the board is doing: **blue** = TCP run in progress,
**green** = UDP run in progress, **red** (1.5 s) = a run failed, **off** =
idle. With the board on a power bank this is the only feedback you get, and
it is enough: wait for the green to go off, walk to the next spot, press
BOOT again.

---

## 5. Expected serial output

```
I (315)  main: EE 250L Lab 05 - ESP32-S3 iperf client
I (335)  wifi: station started, associating with "my-router"
I (2951) wifi: got IP 192.168.1.50 (gw 192.168.1.1, mask 255.255.255.0)
I (2961) led:  WS2812 ready on GPIO 38 (set LED_GPIO to 48 for a DevKitC-1 v1.0 board)
I (2971) button: BOOT button (GPIO 0) polled every 20 ms

ESP32-S3 iperf client ready.  Board IP 192.168.1.50, server 192.168.1.42:5001
...
esp32> iperf -c 192.168.1.42 -t 10
I (15210) iperf: TCP: connecting to 192.168.1.42:5001 ...
I (15242) iperf: TCP: connected, sending for 10 s
[TCP] 0.0-10.0 sec  22.61 MBytes  18.09 Mbits/sec  sender
esp32> iperf -c 192.168.1.42 -u -b 10M -t 10
I (31102) iperf: UDP: sending 1470-byte datagrams to 192.168.1.42:5001 at 10.0 Mbit/s for 10 s
[UDP] 0.0-10.0 sec  12.50 MBytes  10.00 Mbits/sec  sender  (8504 datagrams)
[UDP] server report: 9.96 Mbits/sec  receiver   lost 31/8504 datagrams (0.36%)
```

The numbers are yours; the shape is what matters. TCP on an ESP32-S3 over a
good link lands somewhere in the 10-30 Mbit/s range. That is far below the
Raspberry Pi's figures from last year, which is expected: a microcontroller
with a 240 MHz core and a single-stream 2.4 GHz radio is not a Linux box.

---

## 6. Troubleshooting

| Symptom | Likely cause |
|---|---|
| `TCP connect failed, errno=113` (host unreachable) or `errno=104/111` | The VM is not reachable: NAT mode instead of bridged, wrong `IPERF_SERVER_IP`, `iperf -s` not running, or `ufw` blocking port 5001. Test from the VM: `iperf -c <VM IP> -t 2` against itself must work first. |
| TCP works, UDP reports `(none received)` every time | The UDP server is not running (`iperf_logger.sh` starts both; a manual `iperf -s` is TCP only — add `-u`). Or `ufw` allows 5001/tcp but not 5001/udp. |
| UDP loss is 100 % and the VM logged nothing | Same as above, or the link really is dead at that distance — check `rssi`. |
| The VM logs a run the board never printed | An earlier, aborted attempt is still connected. Ctrl-C `iperf_logger.sh`, delete the last row of the CSV, start again. |
| `iperf -s` on the VM says `iperf3`, or `-y C` is an unknown option | You have iperf3. `sudo apt install iperf` installs iperf 2 alongside it. |
| `disconnected (reason=201)` repeatedly | AP not found: 5 GHz-only SSID, or a typo in `WIFI_SSID`. |
| `disconnected (reason=15)` repeatedly | Wrong `WIFI_PASSWORD`. |
| Nothing happens when I type in the monitor | Press Enter once to get a fresh prompt. With the default `sdkconfig` the prompt works on both USB connectors; if you changed the console channel in menuconfig, run `idf.py fullclean`, delete `sdkconfig`, rebuild and reflash. |
| Monitor shows only the `ESP-ROM:esp32s3-...` lines | Press RESET again with the monitor open. If it persists, `idf.py fullclean && idf.py build`, then flash in download mode (BOOT+RESET). |
| `measure: already running` | A BOOT press or a previous `measure` is still going (it takes about 2 minutes). Wait for the LED to go off. |

---

## 7. Notes on the code

- **The client is ~250 lines of plain sockets**, not Espressif's `iperf`
  component. The component only implements the sender side of UDP and never
  sends iperf2's FIN, so the server cannot close a run or report loss. Doing
  it by hand also means you can read exactly what a "throughput test" is:
  `iperf_client.c` has the explanation at the top.
- **Packet loss is a receiver-side quantity.** The sender only knows what it
  sent. iperf2's trick is a datagram with a *negative* sequence number, which
  asks the server to send back its counts. That report is what the board
  prints after a UDP run; the VM's `-y C` log has the same numbers.
- **Rate control without a fine timer.** The UDP sender keeps
  `bytes_sent` tracking `rate × elapsed` and yields for one 1 ms tick whenever
  it is ahead. Over a 10 s run this hits 10 Mbit/s to a fraction of a percent.
- **Two ways to start a measurement, one function.** The console command and
  the BOOT button both call `run_measure()`; the button hands the work to a
  task with a large enough stack rather than running sockets inside the
  20 ms polling loop. `s_busy` keeps the two from overlapping.
