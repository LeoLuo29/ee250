/*
 * EE 250L Fall 2026 - Lab 2, Part 3
 * ESP32-S3 TCP Client Starter
 *
 * This starter reuses the Wi-Fi setup pattern from Lab 1 and leaves the
 * important TCP-client socket operations as TODOs.
 *
 * Configure:
 *   WIFI_SSID
 *   WIFI_PASS
 *   SERVER_IP
 *   SERVER_PORT
 *
 * Then complete the TCP TODOs in tcp_client_task().
 *
 * Credential safety:
 * Do not submit or commit a real Wi-Fi password.
 */

#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

#define WIFI_SSID      "ee250"
#define WIFI_PASS      "ee250@vhe205"

/*
 * Use the Ubuntu VM's reachable IPv4 address here, NOT 127.0.0.1.
 * Example only:
 *   #define SERVER_IP "192.168.1.50"
 */
#define SERVER_IP      "192.168.64.5"
#define SERVER_PORT    5000

#define CLIENT_MESSAGE "Hello from ESP32-S3"

static const char *TAG = "lab2_tcp_client";

static EventGroupHandle_t wifi_event_group;
static const int WIFI_CONNECTED_BIT = BIT0;

static void wifi_event_handler(void *arg,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT &&
               event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "Wi-Fi disconnected; retrying...");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT &&
               event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Wi-Fi connected");
        ESP_LOGI(TAG, "ESP32-S3 IP address: " IPSTR,
                 IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void wifi_init_sta(void)
{
    wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;

    ESP_ERROR_CHECK(
        esp_event_handler_instance_register(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            &wifi_event_handler,
            NULL,
            &instance_any_id));

    ESP_ERROR_CHECK(
        esp_event_handler_instance_register(
            IP_EVENT,
            IP_EVENT_STA_GOT_IP,
            &wifi_event_handler,
            NULL,
            &instance_got_ip));

    wifi_config_t wifi_config = {0};
    
    
    //
    strlcpy((char *)wifi_config.sta.ssid,
            WIFI_SSID,
            sizeof(wifi_config.sta.ssid));
    strlcpy((char *)wifi_config.sta.password,
            WIFI_PASS,
            sizeof(wifi_config.sta.password));

    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Connecting to Wi-Fi SSID: %s", WIFI_SSID);

    xEventGroupWaitBits(wifi_event_group,
                        WIFI_CONNECTED_BIT,
                        pdFALSE,
                        pdTRUE,
                        portMAX_DELAY);
}

static void tcp_client_task(void *pvParameters)
{
    char rx_buffer[256];

    ESP_LOGI(TAG, "TCP server target: %s:%d", SERVER_IP, SERVER_PORT);

    /*
     * TODO 1: Create an IPv4 TCP socket.
     *
     * Python Part 2 equivalent:
     *     socket.socket(socket.AF_INET, socket.SOCK_STREAM)
     *
     * C functions/constants you will likely use:
     *     socket()
     *     AF_INET
     *     SOCK_STREAM
     *     IPPROTO_IP
     *
     * Store the returned file descriptor in sock.
     */
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);

    if (sock < 0) {
        ESP_LOGE(TAG, "TODO: socket has not been created yet");
        vTaskDelete(NULL);
        return;
    }

    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(SERVER_PORT);

    /*
     * TODO 2: Convert SERVER_IP from dotted-decimal text into the binary
     * IPv4 address stored in dest_addr.sin_addr.
     *
     * Function to investigate:
     *     inet_pton()
     */
    if (inet_pton(AF_INET, SERVER_IP, &dest_addr.sin_addr) != 1) {
        ESP_LOGE(TAG, "Invalid SERVER_IP address: %s", SERVER_IP);
        close(sock);
        vTaskDelete(NULL);
        return;
    }

    /*
     * TODO 3: Connect the socket to the Python server.
     *
     * Python Part 2 equivalent:
     *     sock.connect((server_host, server_port))
     *
     * Function to investigate:
     *     connect()
     */
    if (connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) != 0) {
        ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
        close(sock);
        vTaskDelete(NULL);
        return;
    }
    ESP_LOGI(TAG, "Connected to %s:%d", SERVER_IP, SERVER_PORT);

    /*
     * TODO 4: Send CLIENT_MESSAGE to the server.
     *
     * Python Part 2 equivalent:
     *     sock.sendall(message.encode("utf-8"))
     *
     * Function to investigate:
     *     send()
     *
     * On success, print the message you sent with ESP_LOGI().
     */
    const char *msg = CLIENT_MESSAGE;
    size_t total_len = strlen(msg);
    size_t sent_total = 0;
    while (sent_total < total_len) {
        int sent = send(sock, msg + sent_total, total_len - sent_total, 0);
        if (sent < 0) {
            ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
            close(sock);
            vTaskDelete(NULL);
            return;
        }
        sent_total += sent;
    }
    ESP_LOGI(TAG, "Sent: %s", msg);

    /*
     * TODO 5: Receive the server acknowledgment.
     *
     * Python Part 2 equivalent:
     *     response = sock.recv(256)
     *
     * Function to investigate:
     *     recv()
     *
     * Important:
     * recv() returns a byte count. If space remains in rx_buffer, add
     * a '\0' terminator before printing it as a C string.
     */
    int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
    if (len < 0) {
        ESP_LOGE(TAG, "recv failed: errno %d", errno);
    } else if (len == 0) {
        ESP_LOGW(TAG, "Server closed the connection without a response");
    } else {
        rx_buffer[len] = '\0';
        ESP_LOGI(TAG, "Received %d bytes: %s", len, rx_buffer);
    }

    /*
     * TODO 6: Close the TCP socket.
     *
     * Function to investigate:
     *     close()
     */
    close(sock);

    ESP_LOGI(TAG, "TCP client task finished");
    vTaskDelete(NULL);
}

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi_init_sta();

    xTaskCreate(tcp_client_task,
                "tcp_client",
                4096,
                NULL,
                5,
                NULL);
}
