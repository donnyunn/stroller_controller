#include "max17048.h"

static const char *TAG = "max17048";

#define _I2C_NUMBER(num) I2C_NUM_##num
#define I2C_NUMBER(num) _I2C_NUMBER(num)

#define I2C_MASTER_SCL_IO CONFIG_I2C_MASTER_SCL               /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO CONFIG_I2C_MASTER_SDA               /*!< gpio number for I2C master data  */
#define I2C_MASTER_NUM I2C_NUMBER(CONFIG_I2C_MASTER_PORT_NUM) /*!< I2C port number for master dev */
#define I2C_MASTER_FREQ_HZ CONFIG_I2C_MASTER_FREQUENCY        /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE 0                           /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE 0                           /*!< I2C master doesn't need buffer */

#define MAX17048_ADDR 0x36
#define WRITE_BIT I2C_MASTER_WRITE              /*!< I2C master write */
#define READ_BIT I2C_MASTER_READ                /*!< I2C master read */
#define ACK_CHECK_EN 0x1                        /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS 0x0                       /*!< I2C master will not check ack from slave */
#define ACK_VAL 0x0                             /*!< I2C ack value */
#define NACK_VAL 0x1                            /*!< I2C nack value */

typedef enum {
    REG_VCELL = 0x02,
    REG_SOC = 0x04,
    REG_MODE = 0x06,
    REG_VERSION = 0x08,
    REG_HIBRT = 0x0A,
    REG_CONFIG = 0x0C,
    REG_VALRT = 0x14,
    REG_CRATE = 0x16,
    REG_VRESET_ID = 0x18,
    REG_STATUS = 0x1A,
    REG_TABLE = 0x40,
    REG_CMD = 0xFE
} REG;

typedef enum {
    RI = (1 << 0), // Reset indicator
    VH = (1 << 1), // Voltage high alert
    VL = (1 << 2), // Voltage low alert
    VR = (1 << 3), // Voltage reset alert
    HD = (1 << 4), // SOC low alert
    SC = (1 << 5), // SOC change alert
} ALERT;

TaskHandle_t xHandle;
bool isReady = true;

/**
 * @brief i2c master initialization
 */
static esp_err_t i2c_master_init(void)
{
    int i2c_master_port = I2C_MASTER_NUM;
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_MASTER_SDA_IO;
    conf.sda_pullup_en = GPIO_PULLUP_DISABLE;
    conf.scl_io_num = I2C_MASTER_SCL_IO;
    conf.scl_pullup_en = GPIO_PULLUP_DISABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
    conf.clk_flags = 0;
    i2c_param_config(i2c_master_port, &conf);
    return i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}
/*
static esp_err_t max17048_write(const REG reg, uint16_t data)
{
    esp_err_t ret = ESP_OK;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, MAX17048_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, (uint8_t)reg, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, (uint8_t)((data & 0xFF00)>>8), ACK_CHECK_EN);
    i2c_master_write_byte(cmd, (uint8_t)((data & 0x00FF)>>0), ACK_CHECK_EN);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    return ret;
}
*/
static esp_err_t max17048_read(const REG reg, uint16_t * data)
{
    esp_err_t ret = ESP_OK;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, MAX17048_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, (uint8_t)reg, ACK_CHECK_EN);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, MAX17048_ADDR << 1 | READ_BIT, ACK_CHECK_EN);
    i2c_master_read_byte(cmd, ((uint8_t*)data)+1, ACK_VAL);
    i2c_master_read_byte(cmd, ((uint8_t*)data)+0, NACK_VAL);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    return ret;
}

float voltage(void) {
    uint16_t data;
    max17048_read(REG_VCELL, &data);
    return (float)data * 78.125f / 1000000.f;
}

uint8_t percent(void) {
    uint16_t data;
    max17048_read(REG_SOC, &data);
    ESP_LOGI(TAG, "data= %04x", data);
    return (uint8_t)(data / 256);
}

uint8_t version(void) {
    esp_err_t ret;
    uint16_t data;
    ret = max17048_read(REG_VERSION, &data);
    if (ret == ESP_OK)
        return (uint8_t)(data & 0xff);
    else
        return 0xff;
}

void max17048_task(void *arg)
{
    uint8_t value = 0;
    for (;;) {
        if (!isReady) {
            value = percent();
            ESP_LOGW(TAG, "SOC = %d %%", value);
            if (value <= 25) {
                leds_battery_indicator(BATT_25PER);
            } else if (value <= 50) {
                leds_battery_indicator(BATT_50PER);
            } else if (value <= 75) {
                leds_battery_indicator(BATT_75PER);
            } else {
                leds_battery_indicator(BATT_100PER);
            }
            vTaskDelay(5000 / portTICK_PERIOD_MS);
            leds_battery_indicator(BATT_INDICATOR_OFF);
            isReady = true;
        } else {
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }
        // vTaskDelete(xHandle);
    }
}

void max17048_led_indicator(void)
{
    ESP_LOGI(TAG, "ready (%d)", isReady);
    if (isReady) {
        isReady = false;
        // xTaskCreate(max17048_task, "max17048_task", configMINIMAL_STACK_SIZE, NULL, 10, &xHandle);
    }
}

void max17048_init(void)
{
    uint8_t max17048_version;
    esp_err_t err;
    leds_battery_indicator(BATT_INDICATOR_OFF);

    err = i2c_master_init();
    ESP_LOGI(TAG, "i2c master init err (%d)", err);
    max17048_version = version();
    ESP_LOGW(TAG, "max17048 version: %02X", max17048_version);
    if (max17048_version != 0xff) {
        xTaskCreate(max17048_task, "max17048_task", 2048, (void *) 0, 10, NULL);
    } else {
        ESP_LOGE(TAG, "max17048 init fail!!");
    }
}