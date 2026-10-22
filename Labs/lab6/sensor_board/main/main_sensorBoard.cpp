#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sensor.h"

#define LED_PIN GPIO_NUM_8
#define BUTTON_PIN GPIO_NUM_7
#define VIBRATION_SENSOR_PIN GPIO_NUM_6
#define temp_sensor_pin GPIO_NUM_5

#define LIGHT_THRESHOLD 2000 //raw count; tune by covering the LDR, then shining a flashlight on it
#define SAMPLE_PERIOD_MS 100


extern "C" void app_main(void){

    //init
    LightSensor light;
    TempSensor temp;

    // main loop
    TickType_t last_wake = xTaskGetTickCount();
    while(1){
        int light_raw = (int)light.sense();
        float temp_c = temp.sense();

        printf("light=%d (%s) temp_raw=%d (%.1f C)\n",
               light_raw, light_raw > LIGHT_THRESHOLD ? "bright" : "dark",
               temp.lastRaw(), temp_c);

        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(SAMPLE_PERIOD_MS));
    }
}
