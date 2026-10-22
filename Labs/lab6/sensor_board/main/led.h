#ifndef LED_H
#define LED_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

class LED{
public:
    LED(gpio_num_t pin);
    ~LED();
    //
    void on();
    void off();
    void set(bool level);
    void blink(int times, int period_ms); //period_ms on, then period_ms off

protected:
    gpio_num_t pin_;

};

#endif
