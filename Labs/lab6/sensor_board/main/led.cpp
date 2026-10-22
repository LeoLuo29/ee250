#include "led.h"


LED::LED(gpio_num_t pin){ //active-high: GPIO -> R5 -> LED -> GND
    pin_ = pin;
    gpio_reset_pin(pin_);
    gpio_set_direction(pin_, GPIO_MODE_OUTPUT);
    off();
}


void LED::on(){
    gpio_set_level(pin_, 1);
}


void LED::off(){
    gpio_set_level(pin_, 0);
}


void LED::set(bool level){
    gpio_set_level(pin_, level);
}


void LED::blink(int times, int period_ms){
    for (int i = 0; i < times; i++) {
        on();
        vTaskDelay(pdMS_TO_TICKS(period_ms));
        off();
        vTaskDelay(pdMS_TO_TICKS(period_ms));
    }
}


LED::~LED(){
    off();
}
