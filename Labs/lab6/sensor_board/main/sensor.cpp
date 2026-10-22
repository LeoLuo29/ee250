#include "sensor.h"
#include <math.h>


Sensor::Sensor(gpio_num_t pin, int adcOrNot){
    if (adcOrNot) sensorADC_ = new MyADC(pin);
    else {
        sensorADC_ = nullptr;
        gpio_reset_pin(pin);
        gpio_set_direction(pin, GPIO_MODE_INPUT);
    }
    //
    pin_ = pin;
    V_node_ = 0; //initialize to zero f
    

}


float Sensor::sense(){
    if (sensorADC_) {
        V_node_ = sensorADC_->read_mv() / 1000.0f; //node voltage in volts
        return sensorADC_->read_raw();             //0-4095
    }
    return gpio_get_level(pin_);                   //0 or 1
}


LightSensor::LightSensor() : Sensor(GPIO_NUM_4, 1) { //LDR on GPIO4 / ADC1_CH3
}


TempSensor::TempSensor() : Sensor(GPIO_NUM_5, 1) { //thermistor on GPIO5 / ADC1_CH4
}


float LightSensor::sense(){
    return Sensor::sense(); //raw count; higher = brighter
}


float TempSensor::sense(){
    const float R2 = 100000.0f;   //fixed resistor, node to GND
    const float R25 = 100000.0f;  //thermistor resistance at 25 C
    const float BETA = 3950.0f;

    float raw = Sensor::sense();
    lastRaw_ = (int)raw;
    if (raw <= 0 || raw >= 4095) return NAN; //open or shorted divider

    float r_ntc = R2 * (4095.0f / raw - 1.0f);
    float inv_t = 1.0f / 298.15f + logf(r_ntc / R25) / BETA;
    return 1.0f / inv_t - 273.15f; //kelvin to C
}


Sensor::~Sensor(){
    delete sensorADC_; //frees the ADC channel; safe if nullptr
}


LightSensor::~LightSensor(){
    //nothing extra to free; ~Sensor() runs after this
}


TempSensor::~TempSensor(){
    //nothing extra to free; ~Sensor() runs after this
}