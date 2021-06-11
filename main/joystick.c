#include "joystick.h"

#include "driver/adc.h"
#include "driver/dac.h"
#include "driver/gpio.h"
#include "esp_adc_cal.h"

#define TAG "JOYSTICK"

#define X_AXIS_CHANNEL CONFIG_JOYSTICK_AXIS_X
#define Y_AXIS_CHANNEL CONFIG_JOYSTICK_AXIS_Y
#define BUTTON_IO CONFIG_JOYSTICK_BUTTON
#define GPIO_INPUT_PIN_SEL (1ULL<<BUTTON_IO)

#define SAMPLING_NUM 32

static joystick_t * this = NULL;

static void IRAM_ATTR button_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(this->btn_queue, &gpio_num, NULL);
}

void joystick_task(void* arg)
{
    int buf[2];
    for (;;) {
        buf[0] = 0;
        buf[1] = 0;
        for (int i = 0; i < SAMPLING_NUM; i++) {
            buf[0] += adc1_get_raw(X_AXIS_CHANNEL);
            buf[1] += adc1_get_raw(Y_AXIS_CHANNEL);
        }
        buf[0] = buf[0] / 32;
        buf[1] = buf[1] / 32;
        xQueueSend(this->pos_queue, buf, 0);

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void joystick_init(joystick_t * joystick)
{
    esp_err_t ret;
    gpio_config_t io_conf;
    gpio_num_t dac_gpio_num;

    ESP_LOGI(TAG, "%s", __func__);

    this = joystick;
    this->pos_queue = xQueueCreate(10, sizeof(int)*2);
    this->btn_queue = xQueueCreate(10, sizeof(uint32_t));

    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(X_AXIS_CHANNEL, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(Y_AXIS_CHANNEL, ADC_ATTEN_DB_11);

    io_conf.intr_type = GPIO_PIN_INTR_NEGEDGE;
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = 1;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
    gpio_install_isr_service(ESP_INTR_FLAG_EDGE);
    gpio_isr_handler_add(BUTTON_IO, button_isr_handler, (void*)BUTTON_IO);

    ret = dac_pad_get_io_num(DAC_CHANNEL_1, &dac_gpio_num);
    assert(ret == ESP_OK);

    dac_output_enable(DAC_CHANNEL_1);
    dac_output_voltage(DAC_CHANNEL_1, 200);

    xTaskCreate(joystick_task, "joystick_task", 2048, NULL, 10, NULL);
}