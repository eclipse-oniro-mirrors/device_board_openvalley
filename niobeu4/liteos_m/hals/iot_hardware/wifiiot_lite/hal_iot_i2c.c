/*
 * Copyright (c) 2022 Hunan OpenValley Digital Industry Development Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "iot_errno.h"
#include "iot_i2c.h"

#include "driver/i2c.h"
#include "esp_err.h"

#define DATA_LENGTH (512) /*!< Data buffer length for test buffer */

#define I2C_SLAVE_NUM (0)  /*!< I2C port number for slave dev */
#define I2C_MASTER_NUM (1) /*!< I2C port number for master dev */

#define I2C_SLAVE_SCL_IO (5)        /*!< gpio number for i2c slave clock */
#define I2C_SLAVE_SDA_IO (4)        /*!< gpio number for i2c slave data */
#define I2C_MASTER_SCL_IO (4)       /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO (9)       /*!< gpio number for I2C master data */
#define I2C_MASTER_FREQ_HZ (100000) /*!< I2C master clock frequency */

#define I2C_SLAVE_ADDRESS (0x28) /*!< ESP32 slave address, you can set any 7bit value */
#define ACK_CHECK_EN (0x1)       /*!< I2C master will check ack from slave */
#define ACK_VAL (0x0)            /*!< I2C ack value */
#define NACK_VAL (0x1)           /*!< I2C nack value */

#define I2C_MASTER_BUF_LEN (0)              /*!< I2C master do not need buffer */
#define I2C_SLAVE_BUF_LEN (2 * DATA_LENGTH) /*!< I2C slave rx buffer size */

#define I2C_READ_WRITE_TIMEOUT (1000)

#define I2C_SLAVE_DEFAULT                        \
    {                                            \
        .mode = (I2C_MODE_SLAVE),                \
        .sda_io_num = (I2C_SLAVE_SDA_IO),        \
        .scl_io_num = (I2C_SLAVE_SCL_IO),        \
        .sda_pullup_en = (GPIO_PULLUP_ENABLE),   \
        .scl_pullup_en = (GPIO_PULLUP_ENABLE),   \
        .slave.addr_10bit_en = (0),              \
        .slave.slave_addr = (I2C_SLAVE_ADDRESS), \
    }

#define I2C_MASTER_DEFAULT                        \
    {                                             \
        .mode = (I2C_MODE_MASTER),                \
        .sda_io_num = (I2C_MASTER_SDA_IO),        \
        .scl_io_num = (I2C_MASTER_SCL_IO),        \
        .sda_pullup_en = (GPIO_PULLUP_ENABLE),    \
        .scl_pullup_en = (GPIO_PULLUP_ENABLE),    \
        .master.clk_speed = (I2C_MASTER_FREQ_HZ), \
        .clk_flags = (I2C_MASTER_FREQ_HZ),        \
    }

typedef enum {
    I2C_UNINIT = 0,
    I2C_INIT = 1
} i2c_status_e;

typedef struct {
    i2c_port_t i2c_port;
    i2c_status_e i2c_state;
    i2c_config_t i2c_conf;
    size_t buf_len;
} i2c_driver_data_t;

static i2c_driver_data_t i2c_conf_0 = {
    .i2c_port = I2C_SLAVE_NUM,
    .i2c_state = I2C_UNINIT,
    .i2c_conf = I2C_SLAVE_DEFAULT,
    .buf_len = I2C_SLAVE_BUF_LEN,
};

static i2c_driver_data_t i2c_conf_1 = {
    .i2c_port = I2C_MASTER_NUM,
    .i2c_state = I2C_UNINIT,
    .i2c_conf = I2C_MASTER_DEFAULT,
    .buf_len = I2C_MASTER_BUF_LEN,
};

static i2c_driver_data_t *g_i2c_conf[I2C_MODE_MAX] = {&i2c_conf_0, &i2c_conf_1};

static uint32_t ESPErrToHoErr(esp_err_t ret)
{
    if (ret == ESP_OK) {
        return IOT_SUCCESS;
    } else {
        return IOT_FAILURE;
    }
}

unsigned int IoTI2cWrite(unsigned int id, unsigned short deviceAddr, const unsigned char *data, unsigned int dataLen)
{
    if (id >= I2C_MODE_MAX || data == NULL || dataLen == 0) {
        return IOT_FAILURE;
    }

    i2c_driver_data_t *i2c_data = g_i2c_conf[id];
    if (i2c_data->i2c_state == I2C_UNINIT) {
        return IOT_FAILURE;
    }

    if (i2c_data->i2c_conf.mode == I2C_MODE_MASTER) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (deviceAddr << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
        i2c_master_write(cmd, data, dataLen, ACK_CHECK_EN);
        i2c_master_stop(cmd);
        esp_err_t ret = i2c_master_cmd_begin(i2c_data->i2c_port, cmd, I2C_READ_WRITE_TIMEOUT / portTICK_RATE_MS);
        i2c_cmd_link_delete(cmd);
        return ESPErrToHoErr(ret);
    }
    size_t d_size = 0;
    while (1) {
        if (dataLen > d_size) {
            int ret = i2c_slave_write_buffer(i2c_data->i2c_port, data + d_size, dataLen - d_size,
                                             I2C_READ_WRITE_TIMEOUT / portTICK_RATE_MS);
            if (ret < 0) {
                return IOT_FAILURE;
            } else if (ret == 0) {
                break;
            }
            d_size += ret;
        } else {
            break;
        }
    }
    return IOT_SUCCESS;
}

unsigned int IoTI2cRead(unsigned int id, unsigned short deviceAddr, unsigned char *data, unsigned int dataLen)
{
    if (id >= I2C_MODE_MAX || data == NULL || dataLen == 0) {
        return IOT_FAILURE;
    }

    i2c_driver_data_t *i2c_data = g_i2c_conf[id];
    if (i2c_data->i2c_state == I2C_UNINIT) {
        return IOT_FAILURE;
    }

    if (i2c_data->i2c_conf.mode == I2C_MODE_MASTER) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (deviceAddr << 1) | I2C_MASTER_READ, ACK_CHECK_EN);
        if (dataLen > 1) {
            i2c_master_read(cmd, data, dataLen - 1, ACK_VAL);
        }

        i2c_master_read_byte(cmd, data + dataLen - 1, NACK_VAL);
        i2c_master_stop(cmd);
        esp_err_t ret = i2c_master_cmd_begin(i2c_data->i2c_port, cmd, I2C_READ_WRITE_TIMEOUT / portTICK_RATE_MS);
        i2c_cmd_link_delete(cmd);
        return ESPErrToHoErr(ret);
    } else {
        int size_rd = 0;
        int len = 0;
        while (1) {
            len = i2c_slave_read_buffer(I2C_SLAVE_NUM, data + size_rd, dataLen - size_rd,
                                        I2C_READ_WRITE_TIMEOUT / portTICK_RATE_MS);
            if (len == 0) {
                break;
            } else if (len < 0) {
                return IOT_FAILURE;
            }

            size_rd += len;
        }
    }

    return IOT_SUCCESS;
}

unsigned int IoTI2cInit(unsigned int id, unsigned int baudrate)
{
    if (id >= I2C_MODE_MAX) {
        return IOT_FAILURE;
    }

    i2c_driver_data_t *i2c_data = g_i2c_conf[id];
    if (i2c_data->i2c_state == I2C_INIT) {
        return IOT_FAILURE;
    }

    if (id == I2C_MODE_MASTER) {
        if (baudrate == 0) {
            return IOT_FAILURE;
        }

        i2c_data->i2c_conf.master.clk_speed = baudrate;
    }

    i2c_data->i2c_state = I2C_INIT;
    esp_err_t ret = i2c_param_config(i2c_data->i2c_port, &(i2c_data->i2c_conf));
    if (ret != ESP_OK) {
        return IOT_FAILURE;
    }

    ret = i2c_driver_install(i2c_data->i2c_port, i2c_data->i2c_conf.mode, i2c_data->buf_len, i2c_data->buf_len, 0);
    return ESPErrToHoErr(ret);
}

unsigned int IoTI2cDeinit(unsigned int id)
{
    if (id >= I2C_MODE_MAX) {
        return IOT_FAILURE;
    }

    i2c_driver_data_t *i2c_data = g_i2c_conf[id];
    if (i2c_data->i2c_state == I2C_UNINIT) {
        return IOT_FAILURE;
    }

    i2c_data->i2c_state = I2C_UNINIT;
    esp_err_t ret = i2c_driver_delete(i2c_data->i2c_port);
    return ESPErrToHoErr(ret);
}

unsigned int IoTI2cSetBaudrate(unsigned int id, unsigned int baudrate)
{
    if (id >= I2C_MODE_MAX) {
        return IOT_FAILURE;
    }

    return IOT_SUCCESS;
}
