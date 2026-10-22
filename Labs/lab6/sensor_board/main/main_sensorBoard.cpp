#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sensor.h"

#define LED_PIN        GPIO_NUM_8
#define BUTTON_PIN     GPIO_NUM_7
#define VIB_PIN        GPIO_NUM_6
#define VIB_THRESHOLD  3            // edges per 100 ms that count as "vibration"

#define LIGHT_THRESHOLD 2000 //raw count; tune by covering the LDR, then shining a flashlight on it
#define SAMPLE_PERIOD_MS 100


static volatile int vib_count = 0;
static portMUX_TYPE vib_mux = portMUX_INITIALIZER_UNLOCKED;

// Runs every time GPIO6 changes (0->1 or 1->0)
static void IRAM_ATTR vib_isr(void *arg)
{
    portENTER_CRITICAL_ISR(&vib_mux);
    vib_count = vib_count + 1;
    portEXIT_CRITICAL_ISR(&vib_mux);
}


static void led_init(void)
{
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
}

static void button_init(void)
{
    gpio_reset_pin(BUTTON_PIN);
    gpio_set_direction(BUTTON_PIN, GPIO_MODE_INPUT);
}

static void vibration_init(void)
{
    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << VIB_PIN,
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,   // board already has R3 (10 kOhm)
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_ANYEDGE,     // interrupt on every edge
    };
    gpio_config(&cfg);
    gpio_install_isr_service(0);               // enable GPIO interrupts
    gpio_isr_handler_add(VIB_PIN, vib_isr, NULL);
}


static bool read_button(void)
{
    return gpio_get_level(BUTTON_PIN) == 0;    // active-low: 0 = pressed
}

// Returns edges since the last call, then resets the counter
static int read_vibration(void)
{
    portENTER_CRITICAL(&vib_mux);
    int n = vib_count;
    vib_count = 0;
    portEXIT_CRITICAL(&vib_mux);
    return n;
}


extern "C" void app_main(void){

    //init
    led_init();
    button_init();
    vibration_init();
    LightSensor light;
    TempSensor temp;

    // Startup test: blink 5 times, 500 ms on / 500 ms off
    for (int i = 0; i < 5; i++) {
        gpio_set_level(LED_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(500));
        gpio_set_level(LED_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    read_vibration();   // clear any edges counted during the blinks

    // main loop
    TickType_t last_wake = xTaskGetTickCount();
    while(1){
        int light_raw = (int)light.sense();
        float temp_c = temp.sense();
        bool pressed = read_button();
        int  vib     = read_vibration();

        printf("light=%d (%s) temp_raw=%d (%.1f C)  button=%s  vibration=%d\n",
               light_raw, light_raw > LIGHT_THRESHOLD ? "bright" : "dark",
               temp.lastRaw(), temp_c,
               pressed ? "PRESSED" : "released", vib);

        gpio_set_level(LED_PIN, pressed || vib > VIB_THRESHOLD);

        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(SAMPLE_PERIOD_MS));
    }
}
