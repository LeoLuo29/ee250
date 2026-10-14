#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "cJSON.h"
#include "esp_crt_bundle.h"
#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "credentials.h"

#define RESPONSE_BUFFER_SIZE 8192
#define URL_BUFFER_SIZE 512

static const char *TAG = "weather_client";

static EventGroupHandle_t wifi_event_group;
static const int WIFI_CONNECTED_BIT = BIT0;

typedef struct {
    char data[RESPONSE_BUFFER_SIZE];
    size_t length;
} response_buffer_t;

/* Keep the large response buffer off the task stack. */
static response_buffer_t response;


static void wifi_event_handler(void *arg,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT &&
               event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
    } else if (event_base == IP_EVENT &&
               event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = event_data;

        ESP_LOGI(TAG, "IP address: " IPSTR,
                 IP2STR(&event->ip_info.ip));

        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}


static void wifi_init(void)
{
    wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(
        esp_event_handler_register(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            wifi_event_handler,
            NULL
        )
    );

    ESP_ERROR_CHECK(
        esp_event_handler_register(
            IP_EVENT,
            IP_EVENT_STA_GOT_IP,
            wifi_event_handler,
            NULL
        )
    );

    wifi_config_t wifi_config = {0};

    strncpy(
        (char *)wifi_config.sta.ssid,
        WIFI_SSID,
        sizeof(wifi_config.sta.ssid) - 1
    );

    strncpy(
        (char *)wifi_config.sta.password,
        WIFI_PASSWORD,
        sizeof(wifi_config.sta.password) - 1
    );

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(
        esp_wifi_set_config(WIFI_IF_STA, &wifi_config)
    );
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Connecting to Wi-Fi...");

    xEventGroupWaitBits(
        wifi_event_group,
        WIFI_CONNECTED_BIT,
        pdFALSE,
        pdTRUE,
        portMAX_DELAY
    );
}


static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    response_buffer_t *buffer = evt->user_data;

    if (evt->event_id == HTTP_EVENT_ON_DATA &&
        evt->data_len > 0) {

        size_t remaining =
            RESPONSE_BUFFER_SIZE - 1 - buffer->length;

        if ((size_t)evt->data_len > remaining) {
            ESP_LOGE(TAG, "Response too large");
            return ESP_FAIL;
        }

        memcpy(
            buffer->data + buffer->length,
            evt->data,
            evt->data_len
        );

        buffer->length += evt->data_len;
        buffer->data[buffer->length] = '\0';
    }

    return ESP_OK;
}


static void parse_weather(const char *json_text)
{
    cJSON *root = cJSON_Parse(json_text);

    if (root == NULL) {
        ESP_LOGE(TAG, "Invalid JSON");
        return;
    }

    // TODO 3:
    // Extract:
    // location.name
    // location.region
    // location.country
    // current.temp_f
    // current.condition.text
    // current.humidity
    cJSON *location = cJSON_GetObjectItem(root, "location");
    cJSON *current = cJSON_GetObjectItem(root, "current");

    cJSON *name = cJSON_GetObjectItem(location, "name");
    cJSON *region = cJSON_GetObjectItem(location, "region");
    cJSON *country = cJSON_GetObjectItem(location, "country");
    cJSON *temp_f = cJSON_GetObjectItem(current, "temp_f");
    cJSON *condition = cJSON_GetObjectItem(current, "condition");
    cJSON *text = cJSON_GetObjectItem(condition, "text");
    cJSON *humidity = cJSON_GetObjectItem(current, "humidity");

    if (!cJSON_IsString(name) ||
        !cJSON_IsString(region) ||
        !cJSON_IsString(country) ||
        !cJSON_IsNumber(temp_f) ||
        !cJSON_IsString(text) ||
        !cJSON_IsNumber(humidity)) {
        ESP_LOGE(TAG, "Unexpected JSON structure");
        cJSON_Delete(root);
        return;
    }

    // TODO 4:
    // Print those values.
    ESP_LOGI(TAG, "Location: %s, %s, %s",
             name->valuestring,
             region->valuestring,
             country->valuestring);
    ESP_LOGI(TAG, "Temperature: %.1f F", temp_f->valuedouble);
    ESP_LOGI(TAG, "Condition: %s", text->valuestring);
    ESP_LOGI(TAG, "Humidity: %d%%", humidity->valueint);

    cJSON_Delete(root);
}


static void weather_task(void *arg)
{
    char url[URL_BUFFER_SIZE];

    // TODO 1:
    // Build this URL with snprintf():
    //
    // https://api.weatherapi.com/v1/current.json
    // ?key=<WEATHER_API_KEY>&q=<WEATHER_QUERY>&aqi=no
    //
    // Do not print the full URL because it contains your API key.
    snprintf(
        url,
        sizeof(url),
        "https://api.weatherapi.com/v1/current.json"
        "?key=%s&q=%s&aqi=no",
        WEATHER_API_KEY,
        WEATHER_QUERY
    );

    if (url[0] == '\0') {
        ESP_LOGE(TAG, "Complete TODO 1 before running the request");
        vTaskDelete(NULL);
        return;
    }

    memset(&response, 0, sizeof(response));

    esp_http_client_config_t config = {
        .url = url,
        .event_handler = http_event_handler,
        .user_data = &response,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .timeout_ms = 10000,
    };

    esp_http_client_handle_t client =
        esp_http_client_init(&config);

    if (client == NULL) {
        ESP_LOGE(TAG, "Could not create HTTP client");
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "Sending WeatherAPI HTTPS GET request...");

    esp_err_t err = esp_http_client_perform(client);

    if (err != ESP_OK) {
        ESP_LOGE(
            TAG,
            "Request failed: %s",
            esp_err_to_name(err)
        );
        esp_http_client_cleanup(client);
        vTaskDelete(NULL);
        return;
    }

    int status =
        esp_http_client_get_status_code(client);

    ESP_LOGI(TAG, "HTTP status: %d", status);

    // TODO 2:
    // If status is 200, call parse_weather(response.data).
    // Otherwise print an HTTP error.
    if (status == 200) {
        parse_weather(response.data);
    } else {
        ESP_LOGE(TAG, "HTTP error %d: %s", status, response.data);
    }

    esp_http_client_cleanup(client);
    vTaskDelete(NULL);
}


void app_main(void)
{
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {

        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    } else {
        ESP_ERROR_CHECK(ret);
    }

    wifi_init();

    BaseType_t created = xTaskCreate(
        weather_task,
        "weather_task",
        8192,
        NULL,
        5,
        NULL
    );

    if (created != pdPASS) {
        ESP_LOGE(TAG, "Could not create weather task");
    }
}
