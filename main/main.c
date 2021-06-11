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

#include "ble_spp_client.h"
#include "joystick.h"

#define TAG "ROTOM APP"

joystick_t joystick;

void app_main(void)
{
    int joystick_pos[2];
    uint32_t joystick_btn;

    nvs_flash_init();

    ble_spp_client_init();
    joystick_init(&joystick);

    while (1) {
        if (xQueueReceive(joystick.pos_queue, joystick_pos, 10/portTICK_RATE_MS)) {
            // ESP_LOGI(TAG, "%d, %d", joystick_pos[0], joystick_pos[1]);
            ble_spp_send((uint8_t*)joystick_pos, 2);
        }
        if (xQueueReceive(joystick.btn_queue, &joystick_btn, 10/portTICK_RATE_MS)) {
            ESP_LOGI(TAG, "%d", joystick_btn);
        }
    }
}
