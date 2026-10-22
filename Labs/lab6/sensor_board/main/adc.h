#ifndef ADC_H
#define ADC_H

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"


class MyADC {
public:
    MyADC(gpio_num_t pin);
    ~MyADC();

    // the ADC hardware handles can't be safely shared between copies
    MyADC(const MyADC&) = delete;
    MyADC& operator=(const MyADC&) = delete;

    int read_raw();   // 0-4095
    int read_mv();    // calibrated millivolts

private:
    gpio_num_t pin;
    adc_unit_t unit;
    adc_channel_t channel;
    adc_cali_handle_t cali_handle;

    // only one oneshot handle may exist per ADC unit, so instances share it
    static adc_oneshot_unit_handle_t unit_handles[2];
    static int unit_users[2];
};


#endif
