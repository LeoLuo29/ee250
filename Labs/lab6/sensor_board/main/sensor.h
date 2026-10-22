#ifndef SENSOR_H
#define SENSOR_H

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "adc.h"

class Sensor{
public:
    Sensor(gpio_num_t pin, int adcOrNot);
    virtual ~Sensor();
    //owns sensorADC_, so copies would delete it twice
    Sensor(const Sensor&) = delete;
    Sensor& operator=(const Sensor&) = delete;
    //
    virtual float sense();
    float voltage() const { return V_node_; } //node voltage from the last ADC read

protected:
    MyADC* sensorADC_;
    float V_node_;
    gpio_num_t pin_;

};


class LightSensor : public Sensor{
public:
    LightSensor();
    ~LightSensor() override;
    //
    float sense() override;

protected:

};


class TempSensor : public Sensor{
public:
    TempSensor();
    ~TempSensor() override;
    //
    float sense() override;
    int lastRaw() const { return lastRaw_; } //raw count from the last sense()


protected:
    int lastRaw_ = 0;

};





#endif