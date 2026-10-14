/* EE 250L Lab 05 Starter Code — Fall 2026 (unchanged from Lab 04)
 *
 * wifi_station.c — SUPPLIED.  No TODOs here.
 *
 * ---------------------------------------------------------------------------
 * How ESP-IDF networking is wired together
 * ---------------------------------------------------------------------------
 * Three cooperating pieces, all of which we start below:
 *
 *   esp_netif   The glue between a driver (the Wi-Fi MAC) and the TCP/IP
 *               stack (lwIP).  `esp_netif_create_default_wifi_sta()` builds
 *               one pre-wired "station" interface, including a DHCP client.
 *               (If you find old tutorials calling `tcpip_adapter_*`, that
 *               was the pre-v4.1 API and it no longer exists.)
 *
 *   esp_event   A publish-subscribe bus *inside the chip*.  The Wi-Fi driver
 *               posts events onto the "default event loop"; we register
 *               handlers for the ones we care about.  It is worth noticing
 *               that this is the same design pattern as MQTT — a producer
 *               that does not know who is listening — one hop earlier in the
 *               stack.
 *
 *   EventGroup  A FreeRTOS primitive: a word of bits that a task can block
 *               on.  Our event handlers run in the event-loop task, not in
 *               app_main(), so we need some way for app_main() to wait for
 *               "we got an IP".  Setting a bit from the handler and having
 *               app_main() call xEventGroupWaitBits() is the idiomatic way.
 *
 * The state machine we implement:
 *
 *   esp_wifi_start()
 *        -> WIFI_EVENT_STA_START        : we call esp_wifi_connect()
 *        -> WIFI_EVENT_STA_DISCONNECTED : back off, call esp_wifi_connect()
 *        -> IP_EVENT_STA_GOT_IP         : set WIFI_CONNECTED_BIT, unblock
 */

#include "wifi_station.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"

#include "credentials.h"

static const char *TAG = "wifi";

/* Bits in our event group. */
#define WIFI_CONNECTED_BIT BIT0 /* set once we hold an IPv4 address    */
#define WIFI_FAIL_BIT      BIT1 /* set once we have given up retrying  */

/* How long wifi_station_connect() is willing to wait for DHCP. */
#define WIFI_CONNECT_TIMEOUT_MS 30000

/* How many association attempts before we report failure to the caller.
 * We keep retrying in the background after that; this only bounds the wait. */
#define WIFI_MAX_RETRY 10

static EventGroupHandle_t s_wifi_events;
static esp_netif_t       *s_sta_netif;
static int                s_retry_count;
static esp_ip4_addr_t     s_ip_addr;

/* ---------------------------------------------------------------------------
 * Event handler
 * ---------------------------------------------------------------------------
 * One handler for both event "bases" (WIFI_EVENT and IP_EVENT).  ESP-IDF
 * identifies an event by the pair (base, id); `base` is a pointer to a string
 * constant, so comparing it with == is correct and is what the SDK examples
 * do.
 *
 * This runs in the event-loop task.  Keep it short: anything slow here delays
 * every other event in the system.  The backoff delay below is a deliberate,
 * commented-on exception — it is at most a couple of seconds and there is
 * nothing else competing for the loop at that point in boot.
 */
static void wifi_event_handler(void *arg, esp_event_base_t base,
                               int32_t event_id, void *event_data)
{
    if (base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        /* The driver is up but has not tried to associate yet.  Associating
         * is a separate, explicit step so that an application could scan
         * first and pick an AP. */
        ESP_LOGI(TAG, "station started, associating with \"%s\"", WIFI_SSID);
        esp_wifi_connect();

    } else if (base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        const wifi_event_sta_disconnected_t *d =
            (const wifi_event_sta_disconnected_t *)event_data;

        /* We get here both when association is refused (bad password, AP not
         * present) and when an established link drops.  Either way the cure
         * is the same: wait a bit and ask again.  The wait matters — hammering
         * esp_wifi_connect() in a tight loop floods the air and, on a busy
         * band, makes it *less* likely you ever get on. */
        s_retry_count++;

        /* Exponential-ish backoff: 0.5 s, 1 s, 2 s, 4 s, capped at 8 s. */
        int delay_ms = 500 << (s_retry_count > 4 ? 4 : s_retry_count - 1);
        if (delay_ms > 8000) {
            delay_ms = 8000;
        }

        ESP_LOGW(TAG, "disconnected (reason=%d), retry %d in %d ms",
                 d->reason, s_retry_count, delay_ms);

        if (s_retry_count == WIFI_MAX_RETRY) {
            /* Unblock wifi_station_connect() with a failure.  Note we do NOT
             * stop retrying: a student who mistyped the SSID wants the error
             * message, but a student whose router rebooted wants the ESP32 to
             * come back on its own. */
            ESP_LOGE(TAG, "%d failed attempts. Check WIFI_SSID / WIFI_PASSWORD "
                          "in credentials.h. Still retrying in background.",
                     WIFI_MAX_RETRY);
            xEventGroupSetBits(s_wifi_events, WIFI_FAIL_BIT);
        }

        xEventGroupClearBits(s_wifi_events, WIFI_CONNECTED_BIT);
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
        esp_wifi_connect();

    } else if (base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        const ip_event_got_ip_t *e = (const ip_event_got_ip_t *)event_data;

        /* Associated *and* the DHCP server has leased us an address.  Only
         * now is it safe to open a socket — which is why the iperf client is
         * called after this, not before. */
        s_ip_addr = e->ip_info.ip;
        s_retry_count = 0;

        ESP_LOGI(TAG, "got IP " IPSTR " (gw " IPSTR ", mask " IPSTR ")",
                 IP2STR(&e->ip_info.ip), IP2STR(&e->ip_info.gw),
                 IP2STR(&e->ip_info.netmask));

        xEventGroupClearBits(s_wifi_events, WIFI_FAIL_BIT);
        xEventGroupSetBits(s_wifi_events, WIFI_CONNECTED_BIT);
    }
}

/* ------------------------------------------------------------------------- */

esp_err_t wifi_station_connect(void)
{
    s_wifi_events = xEventGroupCreate();
    if (s_wifi_events == NULL) {
        return ESP_ERR_NO_MEM;
    }

    /* lwIP + the esp_netif glue layer. */
    ESP_ERROR_CHECK(esp_netif_init());

    /* The default event loop.  The Wi-Fi driver posts onto
     * it, so it must exist before the driver is initialised.  The rest of the firmware relies
     * on us having created it here. */
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    s_sta_netif = esp_netif_create_default_wifi_sta();
    if (s_sta_netif == NULL) {
        return ESP_FAIL;
    }

    wifi_init_config_t init_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init_cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));

    /* Designated initialisers zero every field we do not mention, which is
     * what we want: the defaults are sane and the struct is large. */
    wifi_config_t sta_cfg = {
        .sta = {
            /* `threshold.authmode` is the *weakest* security we will accept.
             * Leaving it at the default makes ESP-IDF refuse anything below
             * WPA2 once a password is set, which trips up older hotspots, so
             * we relax it one notch and accept WPA-PSK. */
            .threshold.authmode = WIFI_AUTH_WPA_PSK,
            /* Protected Management Frames: offer, do not require.  Some APs
             * refuse association if we demand PMF and they cannot do it. */
            .pmf_cfg = {
                .capable  = true,
                .required = false,
            },
        },
    };

    /* The SSID/password fields are fixed-size uint8_t arrays, not pointers,
     * so we copy into them.  strncpy with sizeof() bounds is the safe idiom;
     * an SSID is at most 32 bytes and is *not* required to be NUL-terminated
     * in the struct, which is why we do not need sizeof()-1. */
    strncpy((char *)sta_cfg.sta.ssid, WIFI_SSID, sizeof(sta_cfg.sta.ssid));
    strncpy((char *)sta_cfg.sta.password, WIFI_PASSWORD,
            sizeof(sta_cfg.sta.password));

    /* WIFI_MODE_STA = station only (no soft-AP).  The ESP32-S3's radio is
     * 2.4 GHz-only, so there is nothing to configure to "force 2.4 GHz": a
     * 5 GHz AP is simply invisible to it. */
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_cfg));
    ESP_ERROR_CHECK(esp_wifi_start()); /* -> WIFI_EVENT_STA_START */

    ESP_LOGI(TAG, "waiting up to %d s for an IP address...",
             WIFI_CONNECT_TIMEOUT_MS / 1000);

    /* Block this task until a handler sets one of the two bits, or we run out
     * of patience.  pdFALSE, pdFALSE = "do not clear on exit, any bit will
     * do". */
    EventBits_t bits = xEventGroupWaitBits(
        s_wifi_events, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE, pdFALSE, pdMS_TO_TICKS(WIFI_CONNECT_TIMEOUT_MS));

    if (bits & WIFI_CONNECTED_BIT) {
        char ip[16];
        wifi_get_ip_str(ip, sizeof(ip));
        ESP_LOGI(TAG, "Wi-Fi connected, IP %s", ip);
        return ESP_OK;
    }
    if (bits & WIFI_FAIL_BIT) {
        return ESP_FAIL;
    }
    ESP_LOGE(TAG, "timed out waiting for an IP address");
    return ESP_ERR_TIMEOUT;
}

esp_err_t wifi_get_ip_str(char *dst, size_t n)
{
    if (dst == NULL || n == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_ip_addr.addr == 0) {
        dst[0] = '\0';
        return ESP_ERR_INVALID_STATE;
    }
    /* esp_ip4addr_ntoa() is the esp_netif spelling of inet_ntoa() that writes
     * into a caller-supplied buffer instead of a static one (so it is safe to
     * call from more than one task).  Its length argument is an `int`, hence
     * the cast. */
    esp_ip4addr_ntoa(&s_ip_addr, dst, (int)n);
    return ESP_OK;
}

esp_err_t wifi_get_rssi(int *out_rssi)
{
    if (out_rssi == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    /* esp_wifi_sta_get_ap_info() describes the AP we are currently associated
     * with — SSID, BSSID, channel, and the RSSI of the last frame we heard
     * from it.  It returns ESP_ERR_WIFI_NOT_CONNECT if we are not associated,
     * which is why the caller must be prepared for an error. */
    wifi_ap_record_t ap;
    esp_err_t err = esp_wifi_sta_get_ap_info(&ap);
    if (err != ESP_OK) {
        return err;
    }

    *out_rssi = ap.rssi; /* int8_t, dBm, negative */
    return ESP_OK;
}
