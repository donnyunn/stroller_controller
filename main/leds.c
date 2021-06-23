#include "leds.h"

#define GPIO_BATT_25PER 26
#define GPIO_BATT_50PER 27
#define GPIO_BATT_75PER 14
#define GPIO_BATT_100PER 12
#define GPIO_BATT_INDIC_SEL ((1ULL << GPIO_BATT_25PER) | (1ULL << GPIO_BATT_50PER) | (1ULL << GPIO_BATT_75PER) | (1ULL << GPIO_BATT_100PER))

#define GPIO_BLE_INDICATOR 2

TaskHandle_t xHandle;

void leds_ble_blink_task(void *arg)
{
    for (;;) {
        gpio_set_level(GPIO_BLE_INDICATOR, 1);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        gpio_set_level(GPIO_BLE_INDICATOR, 0);
        vTaskDelay(1500 / portTICK_PERIOD_MS);
    }
}

void leds_ble_indicator(ble_indicator_t mode)
{
    switch (mode) {
        case BLE_INDIC_OFF:
            gpio_set_level(GPIO_BLE_INDICATOR, 0);
        break;
        case BLE_INDIC_ON:
            gpio_set_level(GPIO_BLE_INDICATOR, 1);
        break;
        case BLE_INDIC_BLINK_START:
            xTaskCreate(leds_ble_blink_task, "leds_ble_blink_task", 2048, NULL, 10, &xHandle);
        break;
        case BLE_INDIC_BLINK_STOP:
            vTaskDelete(xHandle);
        break;
    }
}

void leds_battery_indicator(battery_indicator_t num)
{
    switch (num) {
        case BATT_25PER:
            gpio_set_level(GPIO_BATT_25PER, 1);
            gpio_set_level(GPIO_BATT_50PER, 0);
            gpio_set_level(GPIO_BATT_75PER, 0);
            gpio_set_level(GPIO_BATT_100PER, 0);
        break;
        case BATT_50PER:
            gpio_set_level(GPIO_BATT_25PER, 1);
            gpio_set_level(GPIO_BATT_50PER, 1);
            gpio_set_level(GPIO_BATT_75PER, 0);
            gpio_set_level(GPIO_BATT_100PER, 0);
        break;
        case BATT_75PER:
            gpio_set_level(GPIO_BATT_25PER, 1);
            gpio_set_level(GPIO_BATT_50PER, 1);
            gpio_set_level(GPIO_BATT_75PER, 1);
            gpio_set_level(GPIO_BATT_100PER, 0);
        break;
        case BATT_100PER:
            gpio_set_level(GPIO_BATT_25PER, 1);
            gpio_set_level(GPIO_BATT_50PER, 1);
            gpio_set_level(GPIO_BATT_75PER, 1);
            gpio_set_level(GPIO_BATT_100PER, 1);
        break;
        case BATT_INDICATOR_OFF:
            gpio_set_level(GPIO_BATT_25PER, 0);
            gpio_set_level(GPIO_BATT_50PER, 0);
            gpio_set_level(GPIO_BATT_75PER, 0);
            gpio_set_level(GPIO_BATT_100PER, 0);
        break;
    }
}

void leds_init(void)
{
    gpio_config_t io_conf;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = GPIO_BATT_INDIC_SEL;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1UL << GPIO_BLE_INDICATOR);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
}