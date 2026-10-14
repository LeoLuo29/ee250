/* EE 250L Lab 05 Starter Code — Fall 2026
 *
 * iperf_client.h — SUPPLIED.  A small iperf2-compatible *client* for the
 * ESP32-S3: it plays the role the Raspberry Pi played in this lab last year
 * (`iperf3 -c <server> -t 10` and `iperf3 -c <server> -u -b 10M -t 10`).
 *
 * The server is iperf **2** running on your Ubuntu VM (`sudo apt install
 * iperf`).  iperf3 is a different, incompatible protocol; the ESP32-S3 will
 * not talk to it.
 *
 * No TODOs here.  Read iperf_client.c: it is under 300 lines and it shows,
 * with plain sockets like the ones you wrote in Lab 2, how a throughput test
 * and a packet-loss test actually work.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

/** What one run measured. */
typedef struct {
    bool     udp;             /**< false = TCP run, true = UDP run           */
    double   seconds;         /**< how long we sent for                      */
    uint64_t bytes_sent;      /**< bytes handed to the socket                */
    double   sender_mbps;     /**< bytes_sent * 8 / seconds / 1e6            */

    /* UDP only.  Filled from the server's end-of-test report, which is the
     * only place packet loss can be counted (the sender cannot know what
     * never arrived).  `have_report` is false if the report never came back,
     * in which case the three fields below are meaningless. */
    bool     have_report;
    uint32_t datagrams_sent;  /**< what we sent                              */
    uint32_t datagrams_lost;  /**< server: expected minus received           */
    double   loss_percent;    /**< 100 * lost / sent                         */
    double   receiver_mbps;   /**< server: bytes received * 8 / seconds / 1e6 */
} iperf_result_t;

/**
 * @brief One TCP run: connect to host:port, stream dummy bytes for
 *        @p seconds, close.  Equivalent to `iperf -c host -t seconds`.
 *
 * The iperf2 server prints the receiver-side throughput when we close the
 * connection, and that is what iperf_logger.sh logs on the VM.
 */
esp_err_t iperf_tcp_client(const char *host, uint16_t port, int seconds,
                           iperf_result_t *out);

/**
 * @brief One UDP run: send numbered 1470-byte datagrams at @p bits_per_sec
 *        for @p seconds, then send iperf2's FIN and wait for the server's
 *        report.  Equivalent to `iperf -c host -u -b bits_per_sec -t seconds`.
 */
esp_err_t iperf_udp_client(const char *host, uint16_t port, int seconds,
                           int bits_per_sec, iperf_result_t *out);

/** Print one result the way the iperf command-line tool does. */
void iperf_print_result(const iperf_result_t *r);
