#ifndef __LEDS_H
#define __LEDS_H

#include "main.h"

typedef enum {
    BLE_INDIC_OFF,
    BLE_INDIC_ON,
    BLE_INDIC_BLINK_START,
    BLE_INDIC_BLINK_STOP,
} ble_indicator_t;

typedef enum {
    BATT_25PER,
    BATT_50PER,
    BATT_75PER,
    BATT_100PER,
    BATT_INDICATOR_OFF,
} battery_indicator_t;

void leds_ble_indicator(ble_indicator_t mode);
void leds_battery_indicator(battery_indicator_t num);
void leds_init(void);

#endif /* __LEDS_H */