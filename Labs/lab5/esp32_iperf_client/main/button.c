/* EE 250L Lab 05 Starter Code — Fall 2026
 *
 * button.c — SUPPLIED.  No TODOs here.
 *
 * Polled, not interrupt-driven: a task reads the pin every 20 ms and reports
 * a press on a HIGH -> LOW transition between two samples.  The bounce of a
 * mechanical switch lasts a few milliseconds, so it is invisible at 20 ms
 * spacing — debounce for free.  Lab 4's board_sensors.c has the longer
 * discussion of the alternatives.
 */

#include "button.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "button";

#define BUTTON_POLL_MS 20

static button_callback_t s_button_cb;

static void button_task(void *arg)
{
    int prev_level = 1; /* start from "released" */

    while (true) {
        int level = gpio_get_level(BUTTON_GPIO);
        if (prev_level == 1 && level == 0) {
            ESP_LOGI(TAG, "BOOT button pressed");
            if (s_button_cb != NULL) {
                s_button_cb();
            }
        }
        prev_level = level;
        vTaskDelay(pdMS_TO_TICKS(BUTTON_POLL_MS));
    }
}

esp_err_t button_init(button_callback_t cb)
{
    s_button_cb = cb;

    gpio_config_t io = {
        .pin_bit_mask = 1ULL << BUTTON_GPIO,
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    esp_err_t err = gpio_config(&io);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "gpio_config failed: %s", esp_err_to_name(err));
        return err;
    }

    /* The callback only hands a semaphore to the measure task, so a small
     * stack is plenty. */
    BaseType_t ok = xTaskCreate(button_task, "button", 2048, NULL, 5, NULL);
    if (ok != pdPASS) {
        return ESP_ERR_NO_MEM;
    }
    ESP_LOGI(TAG, "BOOT button (GPIO %d) polled every %d ms", BUTTON_GPIO,
             BUTTON_POLL_MS);
    return ESP_OK;
}
