/* EE 250L Lab 05 Starter Code — Fall 2026
 *
 * main.c — SUPPLIED.  No TODOs here.
 *
 * The ESP32-S3 takes over the Raspberry Pi's job from last year's Lab 5: it
 * is the iperf *client* that you carry away from the router while the VM,
 * fixed next to the router, runs the iperf *server*.  Because the board has
 * no shell, this file gives it one: a serial console (the same `idf.py
 * monitor` window you already use) with three commands.
 *
 *   iperf -c <host> [-u] [-b 10M] [-t 10]   one run, same flags as last
 *                                           year's `iperf3 -c ...` commands
 *   measure [<host>]                        the automated test: 5 TCP runs,
 *                                           then 5 UDP runs, prints a CSV row
 *                                           for each protocol (this replaces
 *                                           the RPi's iperf_logger.sh)
 *   rssi                                    the signal strength the board
 *                                           sees right now, in dBm
 *
 * Pressing the BOOT button runs `measure` too, so the board can go out on a
 * power bank with no laptop attached: the VM's iperf_logger.sh records the
 * results at the server end, which is why the logging lives there.
 *
 * Boot sequence:  NVS -> Wi-Fi (blocks for an IP) -> LED -> button -> console.
 * The LED shows what the board is doing: blue = TCP run, green = UDP run,
 * red = an error, off = idle and waiting for a command.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_console.h"
#include "argtable3/argtable3.h"

#include "credentials.h"
#include "wifi_station.h"
#include "led.h"
#include "button.h"
#include "iperf_client.h"
#include "freertos/semphr.h"

static const char *TAG = "main";

/* The automated test, mirroring last year's iperf_logger.sh exactly:
 * 5 runs x 10 s TCP, 5 runs x 10 s UDP at 10 Mbit/s, 1 s between runs. */
#define MEASURE_RUNS        5
#define MEASURE_RUN_SECS    10
#define MEASURE_UDP_BPS     10000000
#define MEASURE_GAP_MS      1000

/* ---------------------------------------------------------------------------
 * helpers
 * -------------------------------------------------------------------------*/

/* "10M" -> 10000000, "500k" -> 500000, "2g" -> 2e9, "12345" -> 12345.
 * Lower- and upper-case suffixes are both decimal here (iperf2 makes upper
 * case binary; the difference is 5 % and not worth the confusion). */
static long parse_bandwidth(const char *s)
{
    char *end = NULL;
    double v = strtod(s, &end);
    if (end == s) {
        return -1;
    }
    switch (tolower((unsigned char)*end)) {
    case 'k': v *= 1e3; break;
    case 'm': v *= 1e6; break;
    case 'g': v *= 1e9; break;
    case '\0': break;
    default: return -1;
    }
    return (long)v;
}

static void led_error_blink(void)
{
    led_set_color_name("red");
    vTaskDelay(pdMS_TO_TICKS(1500));
    led_set_color_name("off");
}

/* ---------------------------------------------------------------------------
 * `iperf` command
 * -------------------------------------------------------------------------*/

static struct {
    struct arg_str *host;
    struct arg_lit *udp;
    struct arg_str *bw;
    struct arg_int *time;
    struct arg_int *port;
    struct arg_end *end;
} iperf_args;

static int cmd_iperf(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&iperf_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, iperf_args.end, argv[0]);
        return 1;
    }
    const char *host = iperf_args.host->sval[0];
    int secs = iperf_args.time->count ? iperf_args.time->ival[0] : 10;
    int port = iperf_args.port->count ? iperf_args.port->ival[0]
                                      : IPERF_SERVER_PORT;
    long bps = parse_bandwidth(iperf_args.bw->count ? iperf_args.bw->sval[0]
                                                    : "10M");
    if (secs <= 0 || secs > 120 || bps <= 0 || port <= 0 || port > 65535) {
        printf("bad argument (time 1-120 s, bandwidth like 10M, port 1-65535)\n");
        return 1;
    }

    iperf_result_t r;
    esp_err_t err;
    if (iperf_args.udp->count) {
        led_set_color_name("green");
        err = iperf_udp_client(host, (uint16_t)port, secs, (int)bps, &r);
    } else {
        led_set_color_name("blue");
        err = iperf_tcp_client(host, (uint16_t)port, secs, &r);
    }
    if (err != ESP_OK) {
        led_error_blink();
        return 1;
    }
    iperf_print_result(&r);
    led_set_color_name("off");
    return 0;
}

/* ---------------------------------------------------------------------------
 * `measure` command — the RPi's iperf_logger.sh, on the board
 * -------------------------------------------------------------------------*/

static struct {
    struct arg_str *host;
    struct arg_end *end;
} measure_args;

/* One full measurement set.  Called from the console task (`measure`) or
 * from the measure task (BOOT button); `s_busy` stops the two overlapping. */
static volatile bool s_busy = false;

static int run_measure(const char *host)
{
    if (s_busy) {
        printf("measure: already running, wait for it to finish\n");
        return 1;
    }
    s_busy = true;

    int rssi = 0;
    wifi_get_rssi(&rssi);
    printf("=============================================\n");
    printf(" measure: %d TCP runs then %d UDP runs, %d s each, to %s:%d\n",
           MEASURE_RUNS, MEASURE_RUNS, MEASURE_RUN_SECS, host, IPERF_SERVER_PORT);
    printf(" RSSI at this spot: %d dBm\n", rssi);
    printf(" (the VM must be running ./iperf_logger.sh <distance>)\n");
    printf("=============================================\n");

    double tcp[MEASURE_RUNS] = {0}, udp[MEASURE_RUNS] = {0}, loss[MEASURE_RUNS] = {0};
    iperf_result_t r = {0};

    printf("Starting TCP Tests...\n");
    led_set_color_name("blue");
    for (int i = 0; i < MEASURE_RUNS; i++) {
        printf("TCP Run %d...\n", i + 1);
        if (iperf_tcp_client(host, IPERF_SERVER_PORT, MEASURE_RUN_SECS, &r) != ESP_OK) {
            printf("Error: TCP run failed. Logging 0.\n");
            led_error_blink();
            led_set_color_name("blue");
        } else {
            tcp[i] = r.sender_mbps;
        }
        printf("TCP Sender Throughput (Run %d): %.3f Mbps\n", i + 1, tcp[i]);
        vTaskDelay(pdMS_TO_TICKS(MEASURE_GAP_MS));
    }

    printf("Starting UDP Tests...\n");
    led_set_color_name("green");
    for (int i = 0; i < MEASURE_RUNS; i++) {
        printf("UDP Run %d...\n", i + 1);
        if (iperf_udp_client(host, IPERF_SERVER_PORT, MEASURE_RUN_SECS,
                             MEASURE_UDP_BPS, &r) != ESP_OK) {
            printf("Error: UDP run failed. Logging 0 throughput and 100%% loss.\n");
            loss[i] = 100.0;
            led_error_blink();
            led_set_color_name("green");
        } else {
            udp[i]  = r.sender_mbps;
            loss[i] = r.have_report ? r.loss_percent : 100.0;
        }
        printf("UDP Sender Throughput (Run %d): %.3f Mbps   Packet Loss: %.3f %%%s\n",
               i + 1, udp[i], loss[i],
               (r.have_report || udp[i] == 0.0) ? "" : "  (no server report)");
        vTaskDelay(pdMS_TO_TICKS(MEASURE_GAP_MS));
    }
    led_set_color_name("off");

    double tcp_avg = 0, udp_avg = 0, loss_avg = 0;
    for (int i = 0; i < MEASURE_RUNS; i++) {
        tcp_avg += tcp[i]; udp_avg += udp[i]; loss_avg += loss[i];
    }
    tcp_avg /= MEASURE_RUNS; udp_avg /= MEASURE_RUNS; loss_avg /= MEASURE_RUNS;

    /* Sender-side rows in last year's CSV layout, so you can compare them
     * with the receiver-side rows iperf_logger.sh wrote on the VM.  Those
     * are the ones you plot; these are here so you can see both ends. */
    printf("---------------------------------------------\n");
    printf("Sender-side summary (the VM's CSV files hold the receiver side):\n");
    printf("TCP: Run1,Run2,Run3,Run4,Run5,Avg = %.3f,%.3f,%.3f,%.3f,%.3f,%.3f\n",
           tcp[0], tcp[1], tcp[2], tcp[3], tcp[4], tcp_avg);
    printf("UDP: Run1,Run2,Run3,Run4,Run5,Avg = %.3f,%.3f,%.3f,%.3f,%.3f,%.3f\n",
           udp[0], udp[1], udp[2], udp[3], udp[4], udp_avg);
    printf("UDP loss %%: %.3f,%.3f,%.3f,%.3f,%.3f,Avg %.3f\n",
           loss[0], loss[1], loss[2], loss[3], loss[4], loss_avg);
    printf("measure completed.\n");
    printf("---------------------------------------------\n");

    s_busy = false;
    return 0;
}

static int cmd_measure(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&measure_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, measure_args.end, argv[0]);
        return 1;
    }
    const char *host = measure_args.host->count ? measure_args.host->sval[0]
                                                : IPERF_SERVER_IP;
    return run_measure(host);
}

/* ---------------------------------------------------------------------------
 * BOOT button -> measure, from a task of its own
 * -------------------------------------------------------------------------*/

static SemaphoreHandle_t s_button_sem;

static void on_button(void)
{
    /* Runs in the button task.  Do not measure here: hand the work to the
     * measure task, whose stack is sized for it. */
    xSemaphoreGive(s_button_sem);
}

static void measure_task(void *arg)
{
    while (true) {
        if (xSemaphoreTake(s_button_sem, portMAX_DELAY) == pdTRUE) {
            printf("\nBOOT button: starting measure against %s\n", IPERF_SERVER_IP);
            run_measure(IPERF_SERVER_IP);
        }
    }
}

/* ---------------------------------------------------------------------------
 * `rssi` command
 * -------------------------------------------------------------------------*/

static int cmd_rssi(int argc, char **argv)
{
    int rssi = 0;
    if (wifi_get_rssi(&rssi) != ESP_OK) {
        printf("not associated\n");
        return 1;
    }
    printf("RSSI: %d dBm\n", rssi);
    return 0;
}

/* ---------------------------------------------------------------------------
 * console setup
 * -------------------------------------------------------------------------*/

static void register_commands(void)
{
    iperf_args.host = arg_str1("c", "client", "<host>", "iperf server IPv4 address (your VM)");
    iperf_args.udp  = arg_lit0("u", "udp", "use UDP instead of TCP");
    iperf_args.bw   = arg_str0("b", "bandwidth", "<bw>", "UDP send rate, e.g. 10M (default 10M)");
    iperf_args.time = arg_int0("t", "time", "<secs>", "seconds to send (default 10)");
    iperf_args.port = arg_int0("p", "port", "<port>", "server port (default 5001)");
    iperf_args.end  = arg_end(5);
    const esp_console_cmd_t iperf_cmd = {
        .command  = "iperf",
        .help     = "One throughput test against the VM's iperf2 server. "
                    "TCP: iperf -c <host> -t 10   UDP: iperf -c <host> -u -b 10M -t 10",
        .hint     = NULL,
        .func     = &cmd_iperf,
        .argtable = &iperf_args,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&iperf_cmd));

    measure_args.host = arg_str0(NULL, NULL, "<host>", "iperf server (default: IPERF_SERVER_IP in credentials.h)");
    measure_args.end  = arg_end(2);
    const esp_console_cmd_t measure_cmd = {
        .command  = "measure",
        .help     = "Automated test at one distance: 5 TCP runs then 5 UDP runs "
                    "(run ./iperf_logger.sh <distance> on the VM first)",
        .hint     = NULL,
        .func     = &cmd_measure,
        .argtable = &measure_args,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&measure_cmd));

    const esp_console_cmd_t rssi_cmd = {
        .command = "rssi",
        .help    = "Print the current Wi-Fi signal strength in dBm",
        .hint    = NULL,
        .func    = &cmd_rssi,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&rssi_cmd));
    ESP_ERROR_CHECK(esp_console_register_help_command());
}

static void check_credentials(void)
{
    if (strcmp(WIFI_SSID, "YOUR_WIFI_SSID") == 0) {
        ESP_LOGE(TAG, "WIFI_SSID is still the placeholder - edit main/credentials.h");
    }
    if (strcmp(IPERF_SERVER_IP, "192.168.1.42") == 0) {
        ESP_LOGW(TAG, "IPERF_SERVER_IP is still the example value - set it to your "
                      "VM's `hostname -I` address, or pass the host to `measure`");
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "EE 250L Lab 05 - ESP32-S3 iperf client");
    check_credentials();

    /* NVS: the Wi-Fi driver keeps its calibration data here (see Lab 4). */
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    /* Wi-Fi: blocks until we hold an IPv4 address. */
    err = wifi_station_connect();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Wi-Fi failed (%s). Fix credentials.h and reset the board. "
                      "Halting.", esp_err_to_name(err));
        return;
    }
    ESP_ERROR_CHECK(led_init());

    /* BOOT button: a press queues one `measure` on the measure task. */
    s_button_sem = xSemaphoreCreateBinary();
    ESP_ERROR_CHECK(button_init(on_button));
    if (xTaskCreate(measure_task, "measure", 8192, NULL, 5, NULL) != pdPASS) {
        ESP_LOGE(TAG, "could not create measure task");
    }

    /* Console.  The REPL runs in its own task; app_main() returns after
     * starting it and the idle task takes over.  `idf.py monitor` forwards
     * your keystrokes to the board, so type at the esp32> prompt there. */
    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = "esp32> ";
    repl_config.max_cmdline_length = 128;
    repl_config.task_stack_size = 8192;   /* the iperf runs happen in this task */

#if defined(CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG)
    esp_console_dev_usb_serial_jtag_config_t hw_config =
        ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_usb_serial_jtag(&hw_config, &repl_config, &repl));
#else
    esp_console_dev_uart_config_t hw_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_uart(&hw_config, &repl_config, &repl));
#endif

    register_commands();

    char ip[16] = "?";
    wifi_get_ip_str(ip, sizeof(ip));
    printf("\n"
           "ESP32-S3 iperf client ready.  Board IP %s, server %s:%d\n"
           "Commands:  iperf -c <host> [-u] [-b 10M] [-t 10]   |   measure [<host>]   |   rssi   |   help\n"
           "(or press the BOOT button to run measure with no laptop attached)\n\n",
           ip, IPERF_SERVER_IP, IPERF_SERVER_PORT);

    ESP_ERROR_CHECK(esp_console_start_repl(repl));
}
