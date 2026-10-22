#include "adc.h"
#include "esp_err.h"

adc_oneshot_unit_handle_t MyADC::unit_handles[2] = {nullptr, nullptr};
int MyADC::unit_users[2] = {0, 0};

MyADC::MyADC(gpio_num_t pin) : pin(pin), cali_handle(nullptr) {
    // look up which ADC unit and channel this GPIO belongs to
    ESP_ERROR_CHECK(adc_oneshot_io_to_channel(pin, &unit, &channel));

    if (unit_users[unit] == 0) {
        adc_oneshot_unit_init_cfg_t unit_cfg = {};
        unit_cfg.unit_id = unit;
        ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &unit_handles[unit]));
    }
    unit_users[unit]++;

    adc_oneshot_chan_cfg_t chan_cfg = {};
    chan_cfg.atten = ADC_ATTEN_DB_12;          // ~0-3.1 V input range
    chan_cfg.bitwidth = ADC_BITWIDTH_DEFAULT;  // 12-bit on S3
    ESP_ERROR_CHECK(adc_oneshot_config_channel(unit_handles[unit], channel, &chan_cfg));

    adc_cali_curve_fitting_config_t cali_cfg = {};
    cali_cfg.unit_id = unit;
    cali_cfg.chan = channel;
    cali_cfg.atten = ADC_ATTEN_DB_12;
    cali_cfg.bitwidth = ADC_BITWIDTH_DEFAULT;
    ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(&cali_cfg, &cali_handle));
}

MyADC::~MyADC() {
    adc_cali_delete_scheme_curve_fitting(cali_handle);

    unit_users[unit]--;
    if (unit_users[unit] == 0) {
        adc_oneshot_del_unit(unit_handles[unit]);
        unit_handles[unit] = nullptr;
    }
}

int MyADC::read_raw() {
    int raw = 0;
    ESP_ERROR_CHECK(adc_oneshot_read(unit_handles[unit], channel, &raw));
    return raw;
}

int MyADC::read_mv() {
    int mv = 0;
    ESP_ERROR_CHECK(adc_oneshot_get_calibrated_result(unit_handles[unit], cali_handle, channel, &mv));
    return mv;
}
