/*
   This code is written by Rotom Ltd. but it is in the Public Domain 
   (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

/****************************************************************************
*
* This file is a main code for the controller device.
*
****************************************************************************/

#include "main.h"

#define TAG "rotom app"

typedef enum {
    WORK_INIT = 0,
    WORK_SCANNING,
    WORK_CONNECTED,
    WORK_IDLE,
    WORK_SLEEP,
} main_work_t;

main_work_t work = WORK_INIT;
joystick_t joystick;

xQueueHandle button_queue;

static void IRAM_ATTR button_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    if (!gpio_get_level(gpio_num)) {
        xQueueSendFromISR(button_queue, &gpio_num, NULL);
    }
}

void button_init(void)
{
    gpio_config_t io_conf;

    button_queue = xQueueCreate(10, sizeof(uint32_t));

    io_conf.intr_type = GPIO_PIN_INTR_NEGEDGE;
    io_conf.pin_bit_mask = (1ULL << CONFIG_PAIRING_BUTTON);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = 1;
    io_conf.pull_down_en = 0;
    gpio_config(&io_conf);

    gpio_install_isr_service(ESP_INTR_FLAG_EDGE);
    gpio_isr_handler_add(CONFIG_PAIRING_BUTTON, button_isr_handler, (void*)CONFIG_PAIRING_BUTTON);

    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.pin_bit_mask = (1ULL << CONFIG_BRAKE_BUTTON);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = 1;
    io_conf.pull_down_en = 0;
    gpio_config(&io_conf);
}

bool isBrakePressed(void)
{
    return gpio_get_level(CONFIG_BRAKE_BUTTON) != 0 ? false : true;
}

bool isPairingPressed(void)
{
    return gpio_get_level(CONFIG_PAIRING_BUTTON) != 0 ? false : true;
}

void app_main(void)
{
    uint8_t tx[10];
    char monitor[21];
    int joystick_pos[2];
    uint32_t joystick_btn;
    uint32_t pairing_btn;
    static int pairing_btn_cnt;
    static int scanning_cnt;

    nvs_flash_init();

    while (1) {
        switch (work) {
            case WORK_INIT:
                memset(tx, 0x00, sizeof(tx));
                memset(monitor, 0x00, sizeof(monitor));

                ble_spp_client_init();
                leds_ble_indicator(BLE_INDIC_BLINK_START);

                button_init();
                leds_init();
                max17048_init();
                joystick_init(&joystick);

                scanning_cnt = 0;
                work = WORK_SCANNING;
            break;
            case WORK_SCANNING:
                if (ble_spp_isConnected()) {
                    leds_ble_indicator(BLE_INDIC_BLINK_STOP);
                    leds_ble_indicator(BLE_INDIC_ON);
                    work = WORK_CONNECTED;
                } else {
                    ESP_LOGI(TAG, "%d", scanning_cnt);
                    if (++scanning_cnt > 5000) {
                        scanning_cnt = 0;
                        leds_ble_indicator(BLE_INDIC_BLINK_STOP);
                        leds_ble_indicator(BLE_INDIC_OFF);
                        ESP_LOGI(TAG, "No Device to connect!");

                        ble_spp_stop();
                        work = WORK_IDLE;
                    }
                }
                if (xQueueReceive(button_queue, &pairing_btn, 10/portTICK_RATE_MS)) {
                    max17048_led_indicator();
                }
            break;
            case WORK_CONNECTED:
                if (xQueueReceive(joystick.pos_queue, joystick_pos, 10/portTICK_RATE_MS)) {
                    packet_encoding(PACKET_USER_OPERATION, tx, 0, joystick_pos[0], joystick_pos[1]);
                    ble_spp_send((uint8_t*)tx, 10);
                    // for (int i = 0; i < 10; i++) {
                    //     sprintf(monitor+(2*i), "%02x", tx[i]);
                    // }
                    // ESP_LOGI(TAG, "%s", monitor);
                    // ESP_LOGI(TAG, "%d, %d", joystick_pos[0], joystick_pos[1]);
                    // ble_spp_send((uint8_t*)monitor, 21);
                }
                if (xQueueReceive(joystick.btn_queue, &joystick_btn, 10/portTICK_RATE_MS)) {
                    packet_encoding(PACKET_USER_OPERATION, tx, joystick_btn, joystick_pos[0], joystick_pos[1]);
                    ble_spp_send((uint8_t*)tx, 10);
                    // for (int i = 0; i < 10; i++) {
                    //     sprintf(monitor+(2*i), "%02x", tx[i]);
                    // }
                    // ble_spp_send((uint8_t*)monitor, 21);
                    // ESP_LOGI(TAG, "%d", joystick_btn);
                    // tx[0] = 'A'+joystick_btn;
                    // ble_spp_send(tx, 1);
                }
                if (xQueueReceive(button_queue, &pairing_btn, 10/portTICK_RATE_MS)) {
                    max17048_led_indicator();
                }

                if (joystick.xSemaphore != NULL) {
                    if (xSemaphoreTake(joystick.xSemaphore, (TickType_t)10) == pdTRUE) {
                        if (!ble_spp_isConnected()) {
                            leds_ble_indicator(BLE_INDIC_OFF);
                            ESP_LOGI(TAG, "Disconnection detected!");
                            
                            ble_spp_stop();
                            work = WORK_IDLE;
                        } else {
                            xSemaphoreGive(joystick.xSemaphore);
                        }
                    }

                    if (isBrakePressed()) {
                        if (xSemaphoreTake(joystick.xSemaphore, (TickType_t)10) == pdTRUE) {
                            while (isBrakePressed()) {
                                ESP_LOGI(TAG, "brake!");
                                packet_encoding(PACKET_USER_OPERATION, tx, CONFIG_BRAKE_BUTTON, joystick_pos[0], joystick_pos[1]);
                                ble_spp_send((uint8_t*)tx, 10);
                                vTaskDelay(100/portTICK_PERIOD_MS);
                            }
                            xSemaphoreGive(joystick.xSemaphore);
                        }
                    } else {
                        xSemaphoreGive(joystick.xSemaphore);
                    }
                }

            break;
            case WORK_IDLE:
                ESP_LOGI(TAG, "Entering Sleep Mode");
                vTaskDelay(100/portTICK_PERIOD_MS);
                gpio_intr_disable(CONFIG_PAIRING_BUTTON);
                gpio_wakeup_enable(CONFIG_PAIRING_BUTTON, GPIO_INTR_LOW_LEVEL);
                esp_sleep_enable_gpio_wakeup();
                esp_light_sleep_start();
                work = WORK_SLEEP;
            break;
            case WORK_SLEEP:
                if (isPairingPressed()) {
                    ESP_LOGI(TAG, "%d", pairing_btn_cnt);
                    if (++pairing_btn_cnt > 100) {
                        pairing_btn_cnt = 0;
                        leds_ble_indicator(BLE_INDIC_OFF);

                        // esp_restart();
                        work = WORK_SCANNING;
                    }
                } else {
                    pairing_btn_cnt = 0;
                    work = WORK_IDLE;
                }
                if (xQueueReceive(button_queue, &pairing_btn, 10/portTICK_RATE_MS)) {
                    max17048_led_indicator();
                }
            break;
        }
        
        vTaskDelay(10/portTICK_PERIOD_MS);
    }
}
