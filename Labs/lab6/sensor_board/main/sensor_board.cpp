#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define LED_PIN GPIO_NUM_8
#define BUTTON_PIN GPIO_NUM_7
#define VIBRATION_SENSOR_PIN GPIO_NUM_6
#define temp_sensor_pin GPIO_NUM_5
void led_init(void) {
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
}

void button_init(void) {
    gpio_reset_pin(BUTTON_PIN);
    gpio_set_direction(BUTTON_PIN, GPIO_MODE_INPUT);
}
void read_sensor(void) {
    
}
extern "C" void app_main(void)
{
    // blink led  5 times
    led_init();
    for (int i = 0; i < 5; i++) {
        gpio_set_level(LED_PIN, 1);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        gpio_set_level(LED_PIN, 0);
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
    TickType_t last_wake = xTaskGetTickCount();
    while(1){
        read_sensor(); // replace with actual sensor reading function

        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(100));
    }

}
