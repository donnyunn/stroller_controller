#include "joystick.h"

#include "driver/adc.h"
#include "driver/dac.h"
#include "driver/gpio.h"
#include "esp_adc_cal.h"

#define TAG "JOYSTICK"

#define X_AXIS_CHANNEL      CONFIG_JOYSTICK_AXIS_X
#define Y_AXIS_CHANNEL      CONFIG_JOYSTICK_AXIS_Y
#define LED_IO              CONFIG_JOYSTICK_BUTTON

#define SAMPLING_NUM 128

static joystick_t * this = NULL;

static void IRAM_ATTR joystick_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    if (gpio_get_level(gpio_num)) {
        xQueueSendFromISR(this->btn_queue, &gpio_num, NULL);
    }
}

void joystick_task(void* arg)
{
    int buf[2];
    for (;;) {
        if (this->xSemaphore != NULL) {
            if (xSemaphoreTake(this->xSemaphore, (TickType_t) 10) == pdTRUE) {
                buf[0] = 0;
                buf[1] = 0;
                for (int i = 0; i < SAMPLING_NUM; i++) {
                    buf[0] += adc1_get_raw(X_AXIS_CHANNEL);
                    buf[1] += adc1_get_raw(Y_AXIS_CHANNEL);
                }
                buf[0] = buf[0] / SAMPLING_NUM - this->x;
                buf[1] = buf[1] / SAMPLING_NUM - this->y;
                xQueueSend(this->pos_queue, buf, 0);

                xSemaphoreGive(this->xSemaphore);
            }
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void joystick_init(joystick_t * joystick)
{
    esp_err_t ret;
    gpio_config_t io_conf;
    gpio_num_t dac_gpio_num;
    int x_mean, y_mean;

    ESP_LOGI(TAG, "%s", __func__);

    this = joystick;
    this->pos_queue = xQueueCreate(10, sizeof(int)*2);
    this->btn_queue = xQueueCreate(10, sizeof(uint32_t));
    this->xSemaphore = xSemaphoreCreateBinary();

    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(X_AXIS_CHANNEL, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(Y_AXIS_CHANNEL, ADC_ATTEN_DB_11);

    io_conf.intr_type = GPIO_PIN_INTR_POSEDGE;
    io_conf.pin_bit_mask = (1ULL<<LED_IO);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = 1;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    // gpio_install_isr_service(ESP_INTR_FLAG_EDGE);
    gpio_isr_handler_add(LED_IO, joystick_isr_handler, (void*)LED_IO);

    ret = dac_pad_get_io_num(DAC_CHANNEL_1, &dac_gpio_num);
    assert(ret == ESP_OK);

    dac_output_enable(DAC_CHANNEL_1);
    dac_output_voltage(DAC_CHANNEL_1, 255);

    x_mean = 0;
    y_mean = 0;
    for (int i = 0; i < SAMPLING_NUM; i++) {
        x_mean += adc1_get_raw(X_AXIS_CHANNEL);
        y_mean += adc1_get_raw(Y_AXIS_CHANNEL);
    }
    this->x = x_mean / SAMPLING_NUM;
    this->y = y_mean / SAMPLING_NUM;

    xTaskCreate(joystick_task, "joystick_task", 2048, NULL, 10, NULL);
}