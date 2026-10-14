/* EE 250L Lab 05 Starter Code — Fall 2026 (unchanged from Lab 04)
 *
 * led.c — SUPPLIED.  No TODOs here.
 *
 * ---------------------------------------------------------------------------
 * Driving a WS2812 with the RMT peripheral
 * ---------------------------------------------------------------------------
 * The "RGB LED" on the DevKitC-1 is a WS2812B: a tiny RGB LED with a
 * controller chip inside it, driven over ONE wire with no clock.  Bits are
 * encoded as pulse widths:
 *
 *      '0'  __|‾|________          0.4 us high, 0.85 us low
 *      '1'  __|‾‾‾|______          0.8 us high, 0.45 us low
 *      reset ____________         >50 us low: "latch what I just sent"
 *
 * Twenty-four bits per LED, green-red-blue order, most significant bit
 * first.  Tolerances are about +/-150 ns, which is far tighter than a
 * FreeRTOS task can bit-bang reliably, so we hand the job to the RMT
 * (Remote Control Transceiver) peripheral.  RMT was designed for IR remotes:
 * you give it a list of (level, duration) pairs and it clocks them out in
 * hardware, with no jitter and no CPU involvement.
 *
 * ESP-IDF's RMT driver calls each of those pairs an `rmt_symbol_word_t`, and
 * it needs an *encoder* to turn your payload into symbols.  There are two
 * ways to do this:
 *
 *   1. A "bytes encoder" that is told the two pulse shapes and converts a
 *      byte array on the fly.  This is what Espressif's `led_strip` managed
 *      component does, and it is the right answer for a 300-pixel strip.
 *
 *   2. Build the 24 symbols ourselves and hand them to a "copy encoder",
 *      which just shovels a pre-made symbol array into the peripheral.
 *
 * We do (2).  For a single pixel the whole frame is 25 symbols (100 bytes),
 * it fits in one RMT memory block, and it means this file is self-contained:
 * no download from the component registry, nothing to break when that
 * component's major version changes.
 *
 * Aside on brightness: we use 32/255 rather than 255/255 because the onboard
 * WS2812 at full output is genuinely painful to look at from 40 cm away, and
 * because you will be staring at it for a couple of hours.
 */

#include "led.h"

#include <string.h>
#include <strings.h> /* strcasecmp */

#include "driver/rmt_tx.h"
#include "driver/rmt_encoder.h"
#include "esp_log.h"

static const char *TAG = "led";

/* We clock RMT at 10 MHz, so one "tick" is 100 ns and the pulse widths above
 * become small integers. */
#define LED_RMT_RESOLUTION_HZ 10000000

#define WS2812_T0H_TICKS 4   /* 0.4 us high  */
#define WS2812_T0L_TICKS 8   /* 0.8 us low   */
#define WS2812_T1H_TICKS 8   /* 0.8 us high  */
#define WS2812_T1L_TICKS 4   /* 0.4 us low   */
#define WS2812_RESET_TICKS 800 /* 80 us low; the datasheet wants >50 us */

#define WS2812_BITS 24
/* 24 data symbols + 1 reset symbol. */
#define WS2812_FRAME_SYMBOLS (WS2812_BITS + 1)

/* Moderate brightness: bright enough to read across the bench, dim enough to
 * look at. */
#define LED_LEVEL 32

static rmt_channel_handle_t s_chan;
static rmt_encoder_handle_t s_encoder;
static rmt_symbol_word_t    s_frame[WS2812_FRAME_SYMBOLS];
static const char          *s_current = "off";

/* Name -> GRB triple.  The order of the fields here is the order the WS2812
 * expects on the wire, which is *not* RGB. */
static const struct {
    const char *name;
    uint8_t g, r, b;
} k_colors[] = {
    { "off",   0,         0,         0         },
    { "red",   0,         LED_LEVEL, 0         },
    { "green", LED_LEVEL, 0,         0         },
    { "blue",  0,         0,         LED_LEVEL },
    { "white", LED_LEVEL, LED_LEVEL, LED_LEVEL },
};

/* Fill s_frame with the pulse train for one pixel. */
static void ws2812_build_frame(uint8_t g, uint8_t r, uint8_t b)
{
    const uint8_t grb[3] = { g, r, b };

    int i = 0;
    for (int byte = 0; byte < 3; byte++) {
        for (int bit = 7; bit >= 0; bit--) { /* MSB first */
            bool one = (grb[byte] >> bit) & 1;
            s_frame[i].level0    = 1;
            s_frame[i].duration0 = one ? WS2812_T1H_TICKS : WS2812_T0H_TICKS;
            s_frame[i].level1    = 0;
            s_frame[i].duration1 = one ? WS2812_T1L_TICKS : WS2812_T0L_TICKS;
            i++;
        }
    }

    /* The latch.  Split across both halves of the symbol simply because a
     * symbol always has two halves; both are low, so the LED sees one long
     * low period and commits the 24 bits it just received. */
    s_frame[i].level0    = 0;
    s_frame[i].duration0 = WS2812_RESET_TICKS / 2;
    s_frame[i].level1    = 0;
    s_frame[i].duration1 = WS2812_RESET_TICKS / 2;
}

static esp_err_t ws2812_send(uint8_t g, uint8_t r, uint8_t b)
{
    ws2812_build_frame(g, r, b);

    rmt_transmit_config_t tx_cfg = {
        .loop_count = 0, /* send the frame once; no hardware looping */
    };

    esp_err_t err = rmt_transmit(s_chan, s_encoder, s_frame,
                                 sizeof(s_frame), &tx_cfg);
    if (err != ESP_OK) {
        return err;
    }

    /* rmt_transmit() is asynchronous — it queues the frame.  We block until
     * the peripheral is idle again so that two back-to-back colour changes
     * cannot overlap, and so the caller knows the LED is actually updated by
     * the time this function returns. */
    return rmt_tx_wait_all_done(s_chan, 100);
}

esp_err_t led_init(void)
{
    rmt_tx_channel_config_t chan_cfg = {
        .gpio_num          = LED_GPIO,
        .clk_src           = RMT_CLK_SRC_DEFAULT,
        .resolution_hz     = LED_RMT_RESOLUTION_HZ,
        .mem_block_symbols = 64, /* one RMT memory block; we need 25 */
        .trans_queue_depth = 4,
    };
    esp_err_t err = rmt_new_tx_channel(&chan_cfg, &s_chan);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "rmt_new_tx_channel failed: %s", esp_err_to_name(err));
        return err;
    }

    /* A copy encoder performs no transformation: it copies our symbol array
     * straight into the peripheral's memory. */
    rmt_copy_encoder_config_t enc_cfg = {};
    err = rmt_new_copy_encoder(&enc_cfg, &s_encoder);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "rmt_new_copy_encoder failed: %s", esp_err_to_name(err));
        return err;
    }

    err = rmt_enable(s_chan);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "rmt_enable failed: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "WS2812 ready on GPIO %d (set LED_GPIO to 48 for a "
                  "DevKitC-1 v1.0 board)", LED_GPIO);

    /* Known state at boot: off.  Without this the LED holds whatever the
     * bootloader or the previous firmware left in it. */
    s_current = "off";
    return ws2812_send(0, 0, 0);
}

bool led_set_color_name(const char *name)
{
    if (name == NULL) {
        return false;
    }

    for (size_t i = 0; i < sizeof(k_colors) / sizeof(k_colors[0]); i++) {
        /* strcasecmp so that "RED" from a hand-typed mosquitto_pub works.
         * Note this requires `name` to be NUL-terminated — see led.h. */
        if (strcasecmp(name, k_colors[i].name) == 0) {
            esp_err_t err = ws2812_send(k_colors[i].g, k_colors[i].r,
                                        k_colors[i].b);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "failed to update LED: %s", esp_err_to_name(err));
                return false;
            }
            /* Store the table's copy of the name, not the caller's pointer,
             * which may be a stack buffer that is about to go out of scope. */
            s_current = k_colors[i].name;
            ESP_LOGI(TAG, "LED -> %s", s_current);
            return true;
        }
    }

    ESP_LOGW(TAG, "unknown colour \"%s\" (expected off/red/green/blue/white)",
             name);
    return false;
}

const char *led_get_color_name(void)
{
    return s_current;
}
