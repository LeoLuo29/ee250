/* EE 250L Lab 05 Starter Code — Fall 2026
 *
 * iperf_client.c — SUPPLIED.  No TODOs here, but read it: this is what a
 * throughput measurement *is*.
 *
 * ---------------------------------------------------------------------------
 * How iperf measures things
 * ---------------------------------------------------------------------------
 * TCP.  The client connects and writes dummy bytes as fast as the socket
 * will take them for N seconds, then closes.  Both ends count bytes and
 * divide by time.  Because TCP retransmits anything the Wi-Fi link drops,
 * the receiver never sees loss; it sees the *rate* going down as the link
 * gets worse and TCP spends more of its time waiting and resending.  That is
 * why TCP throughput "gradually decreases with distance".
 *
 * UDP.  The client sends fixed-size datagrams at a fixed rate (-b), and
 * numbers them: the first four bytes of every datagram are a sequence
 * number.  The server counts holes in the sequence — that is packet loss —
 * and, when the client sends a FIN (a datagram with a *negative* sequence
 * number), the server mails its totals back in a "server report".  UDP does
 * not retransmit, so with distance the throughput stays at the offered rate
 * until the link cannot carry it, and then loss appears "suddenly".
 *
 * Both are done here with the same BSD sockets you used in Lab 2, on lwIP.
 * The wire format is iperf 2's (https://iperf.fr), which is why the VM must
 * run `iperf`, not `iperf3`.
 */

#include "iperf_client.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <errno.h>
#include <sys/time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "lwip/sockets.h"
#include "lwip/inet.h"

static const char *TAG = "iperf";

/* Same defaults as the iperf2 command-line client. */
#define TCP_WRITE_LEN     2880   /* two Ethernet-sized TCP segments per write */
#define UDP_DATAGRAM_LEN  1470   /* 1500-byte MTU minus IP and UDP headers    */
#define UDP_FIN_TRIES     10     /* iperf2 also resends its FIN up to 10 times */
#define SOCKET_TIMEOUT_S  5

/* iperf2 UDP datagram header: three big-endian 32-bit words.  iperf 2.1
 * appends a fourth (id2) for 64-bit sequence numbers; both layouts are
 * accepted by every 2.x server. */
typedef struct {
    int32_t  id;       /* sequence number; negative = FIN                    */
    uint32_t tv_sec;   /* send time, seconds (server uses it for jitter)     */
    uint32_t tv_usec;  /* send time, microseconds                            */
} __attribute__((packed)) udp_datagram_hdr_t;

/* iperf2 server report, sent back to us in reply to our FIN.  Ten big-endian
 * 32-bit words.  Fields we use are marked. */
typedef struct {
    int32_t flags;
    int32_t total_len1;    /* high word of bytes received                  */
    int32_t total_len2;    /* low word of bytes received            (used) */
    int32_t stop_sec;      /* test duration as seen by server       (used) */
    int32_t stop_usec;     /*                                       (used) */
    int32_t error_cnt;     /* datagrams lost                        (used) */
    int32_t outorder_cnt;  /* datagrams that arrived out of order          */
    int32_t datagrams;     /* datagrams expected (= our last id + 1) (used) */
    int32_t jitter1;
    int32_t jitter2;
} __attribute__((packed)) udp_server_report_t;

#define IPERF_HEADER_VERSION1 0x80000000u

/* ------------------------------------------------------------------------- */

static double now_s(void)
{
    return (double)esp_timer_get_time() / 1e6;
}

static esp_err_t make_dest(const char *host, uint16_t port,
                           struct sockaddr_in *dest)
{
    memset(dest, 0, sizeof(*dest));
    dest->sin_family = AF_INET;
    dest->sin_port   = htons(port);
    if (inet_pton(AF_INET, host, &dest->sin_addr) != 1) {
        ESP_LOGE(TAG, "\"%s\" is not an IPv4 address (use the VM's "
                      "`hostname -I` address)", host);
        return ESP_ERR_INVALID_ARG;
    }
    return ESP_OK;
}

static void set_timeouts(int sock, int seconds)
{
    struct timeval tv = { .tv_sec = seconds, .tv_usec = 0 };
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
}

/* ------------------------------------------------------------------------- */

esp_err_t iperf_tcp_client(const char *host, uint16_t port, int seconds,
                           iperf_result_t *out)
{
    struct sockaddr_in dest;
    esp_err_t err = make_dest(host, port, &dest);
    if (err != ESP_OK) {
        return err;
    }
    memset(out, 0, sizeof(*out));
    out->udp = false;

    /* The buffer is dummy data — iperf sends letters.  The first bytes must
     * NOT look like an iperf2 "client header" (bit 31 of the first word
     * set), which is why they are zeroed. */
    char *buf = malloc(TCP_WRITE_LEN);
    if (buf == NULL) {
        return ESP_ERR_NO_MEM;
    }
    memset(buf, 0, 64);
    for (int i = 64; i < TCP_WRITE_LEN; i++) {
        buf[i] = 'A' + (i % 26);
    }

    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (sock < 0) {
        free(buf);
        return ESP_FAIL;
    }
    set_timeouts(sock, SOCKET_TIMEOUT_S);

    ESP_LOGI(TAG, "TCP: connecting to %s:%u ...", host, port);
    if (connect(sock, (struct sockaddr *)&dest, sizeof(dest)) != 0) {
        ESP_LOGE(TAG, "TCP connect failed, errno=%d (%s). Is `iperf -s` "
                      "running on the VM, is the VM bridged, is ufw open?",
                 errno, strerror(errno));
        close(sock);
        free(buf);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "TCP: connected, sending for %d s", seconds);

    double t0 = now_s();
    double deadline = t0 + seconds;
    uint64_t sent = 0;
    while (now_s() < deadline) {
        int n = send(sock, buf, TCP_WRITE_LEN, 0);
        if (n < 0) {
            ESP_LOGE(TAG, "TCP send failed, errno=%d (%s)", errno,
                     strerror(errno));
            err = ESP_FAIL;
            break;
        }
        sent += (uint64_t)n;
    }
    double t1 = now_s();

    /* Closing is what makes the server print its line: iperf2 measures a
     * TCP run from the first byte to the FIN. */
    shutdown(sock, SHUT_RDWR);
    close(sock);
    free(buf);

    out->seconds     = t1 - t0;
    out->bytes_sent  = sent;
    out->sender_mbps = (out->seconds > 0)
                       ? (double)sent * 8.0 / out->seconds / 1e6 : 0.0;
    return err;
}

/* ------------------------------------------------------------------------- */

esp_err_t iperf_udp_client(const char *host, uint16_t port, int seconds,
                           int bits_per_sec, iperf_result_t *out)
{
    struct sockaddr_in dest;
    esp_err_t err = make_dest(host, port, &dest);
    if (err != ESP_OK) {
        return err;
    }
    memset(out, 0, sizeof(*out));
    out->udp = true;

    uint8_t *buf = calloc(1, UDP_DATAGRAM_LEN);
    if (buf == NULL) {
        return ESP_ERR_NO_MEM;
    }
    udp_datagram_hdr_t *hdr = (udp_datagram_hdr_t *)buf;

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0) {
        free(buf);
        return ESP_FAIL;
    }
    /* A short receive timeout so waiting for the server report is bounded. */
    struct timeval tv = { .tv_sec = 0, .tv_usec = 250 * 1000 };
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    ESP_LOGI(TAG, "UDP: sending %d-byte datagrams to %s:%u at %.1f Mbit/s "
                  "for %d s", UDP_DATAGRAM_LEN, host, port,
             bits_per_sec / 1e6, seconds);

    /* Rate control.  We do not sleep a fixed time between datagrams (the
     * FreeRTOS tick is too coarse for that); instead we keep the number of
     * bytes sent tracking `rate * elapsed`, sending a small burst whenever we
     * fall behind and yielding for a tick when we are ahead.  Over 10 s this
     * hits the target rate to well under 1 %. */
    const double bytes_per_s = bits_per_sec / 8.0;
    double t0 = now_s();
    double deadline = t0 + seconds;
    uint64_t sent = 0;
    int32_t seq = 0;

    while (true) {
        double t = now_s();
        if (t >= deadline) {
            break;
        }
        uint64_t should_have_sent = (uint64_t)((t - t0) * bytes_per_s);
        if (sent >= should_have_sent) {
            vTaskDelay(1); /* ahead of schedule: give the Wi-Fi task a turn */
            continue;
        }

        struct timeval sendtime;
        gettimeofday(&sendtime, NULL);
        hdr->id      = htonl(seq);
        hdr->tv_sec  = htonl((uint32_t)sendtime.tv_sec);
        hdr->tv_usec = htonl((uint32_t)sendtime.tv_usec);

        int n = sendto(sock, buf, UDP_DATAGRAM_LEN, 0,
                       (struct sockaddr *)&dest, sizeof(dest));
        if (n < 0) {
            /* ENOMEM/ENOBUFS = the driver's transmit queue is full, which is
             * expected on a bad link: back off a tick and try the SAME
             * sequence number again.  Anything else is a real error. */
            if (errno == ENOMEM || errno == ENOBUFS) {
                vTaskDelay(1);
                continue;
            }
            ESP_LOGE(TAG, "UDP sendto failed, errno=%d (%s)", errno,
                     strerror(errno));
            err = ESP_FAIL;
            break;
        }
        sent += (uint64_t)n;
        seq++;
    }
    double t1 = now_s();

    out->seconds        = t1 - t0;
    out->bytes_sent     = sent;
    out->datagrams_sent = (uint32_t)seq;
    out->sender_mbps    = (out->seconds > 0)
                          ? (double)sent * 8.0 / out->seconds / 1e6 : 0.0;

    /* FIN: a datagram whose id is the NEGATIVE of the next sequence number.
     * The server answers with its report; if the answer is lost (it is UDP,
     * after all) we resend the FIN, up to UDP_FIN_TRIES times. */
    uint8_t reply[256];
    int got = -1;
    for (int attempt = 0; attempt < UDP_FIN_TRIES && err == ESP_OK; attempt++) {
        struct timeval sendtime;
        gettimeofday(&sendtime, NULL);
        hdr->id      = htonl(-seq);
        hdr->tv_sec  = htonl((uint32_t)sendtime.tv_sec);
        hdr->tv_usec = htonl((uint32_t)sendtime.tv_usec);
        sendto(sock, buf, UDP_DATAGRAM_LEN, 0, (struct sockaddr *)&dest,
               sizeof(dest));

        got = recvfrom(sock, reply, sizeof(reply), 0, NULL, NULL);
        if (got > 0) {
            break;
        }
    }
    close(sock);
    free(buf);

    /* The report follows the echoed datagram header.  iperf 2.0.x servers
     * use a 12-byte header, iperf 2.1.x a 16-byte one; the report's first
     * word always has bit 31 set, which tells the two apart. */
    if (got >= 12 + (int)sizeof(udp_server_report_t)) {
        int off = 12;
        uint32_t w;
        memcpy(&w, reply + 12, sizeof(w));
        if ((ntohl(w) & IPERF_HEADER_VERSION1) == 0 &&
            got >= 16 + (int)sizeof(udp_server_report_t)) {
            off = 16;
        }
        udp_server_report_t rep;
        memcpy(&rep, reply + off, sizeof(rep));

        uint64_t rx_bytes = ((uint64_t)(uint32_t)ntohl(rep.total_len1) << 32)
                            | (uint32_t)ntohl(rep.total_len2);
        double   rx_secs  = (int32_t)ntohl(rep.stop_sec)
                            + (int32_t)ntohl(rep.stop_usec) / 1e6;
        uint32_t lost     = (uint32_t)ntohl(rep.error_cnt);
        uint32_t total    = (uint32_t)ntohl(rep.datagrams);
        if (total == 0) {
            total = out->datagrams_sent;
        }

        out->have_report    = true;
        out->datagrams_lost = lost;
        out->loss_percent   = (total > 0) ? 100.0 * lost / total : 0.0;
        out->receiver_mbps  = (rx_secs > 0)
                              ? (double)rx_bytes * 8.0 / rx_secs / 1e6 : 0.0;
    } else {
        ESP_LOGW(TAG, "UDP: no server report received (link too weak, or the "
                      "VM's UDP server is not running). Loss unknown on this "
                      "side; the VM's iperf_logger.sh still has it if any "
                      "datagram got through.");
    }
    return err;
}

/* ------------------------------------------------------------------------- */

void iperf_print_result(const iperf_result_t *r)
{
    if (!r->udp) {
        printf("[TCP] 0.0-%.1f sec  %.2f MBytes  %.2f Mbits/sec  sender\n",
               r->seconds, r->bytes_sent / 1e6, r->sender_mbps);
        return;
    }
    printf("[UDP] 0.0-%.1f sec  %.2f MBytes  %.2f Mbits/sec  sender  "
           "(%" PRIu32 " datagrams)\n",
           r->seconds, r->bytes_sent / 1e6, r->sender_mbps, r->datagrams_sent);
    if (r->have_report) {
        printf("[UDP] server report: %.2f Mbits/sec  receiver   "
               "lost %" PRIu32 "/%" PRIu32 " datagrams (%.2f%%)\n",
               r->receiver_mbps, r->datagrams_lost, r->datagrams_sent,
               r->loss_percent);
    } else {
        printf("[UDP] server report: (none received)\n");
    }
}
