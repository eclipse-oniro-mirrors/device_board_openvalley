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

#include <securec.h>

#include "iot_errno.h"
#include "iot_gpio.h"

#include "driver/gpio.h"

typedef enum {
    GPIO_PIN_UNINIT = 0,
    GPIO_PIN_INIT = 1
} gpio_status_e;

typedef struct {
    IotGpioValue output_value;
    gpio_status_e gpio_state;
    gpio_config_t gpio_attr;
} gpio_driver_data_t;

static gpio_driver_data_t g_gpioMap[GPIO_NUM_MAX] = {0};

static void InitIo(gpio_num_t num, gpio_config_t *io_conf)
{
    assert(num < GPIO_NUM_MAX);
    assert(io_conf != NULL);
    io_conf->intr_type = GPIO_INTR_DISABLE;
    io_conf->mode = GPIO_MODE_DISABLE;
    io_conf->pin_bit_mask = (1ULL << num);
    io_conf->pull_down_en = 0;
    io_conf->pull_up_en = 0;
}

static gpio_int_type_t HoIntrToESPIntr(IotGpioIntType intType, IotGpioIntPolarity intPolarity)
{
    gpio_int_type_t intr_type;
    if (intType == IOT_INT_TYPE_LEVEL) {
        switch (intPolarity) {
            case IOT_GPIO_EDGE_FALL_LEVEL_LOW:
                intr_type = GPIO_INTR_LOW_LEVEL;
                break;

            case IOT_GPIO_EDGE_RISE_LEVEL_HIGH:
                intr_type = GPIO_INTR_HIGH_LEVEL;
                break;

            default:
                intr_type = GPIO_INTR_DISABLE;
                break;
        }
    } else {
        switch (intPolarity) {
            case IOT_GPIO_EDGE_FALL_LEVEL_LOW:
                intr_type = GPIO_INTR_NEGEDGE;
                break;

            case IOT_GPIO_EDGE_RISE_LEVEL_HIGH:
                intr_type = GPIO_INTR_POSEDGE;
                break;

            default:
                intr_type = GPIO_INTR_DISABLE;
                break;
            }
    }

    return intr_type;
}

static uint32_t ESPErrToHoErr(esp_err_t ret)
{
    if (ret == ESP_OK) {
        return IOT_SUCCESS;
    } else {
        return IOT_FAILURE;
    }
}

unsigned int IoTGpioInit(unsigned int id)
{
    if (id >= GPIO_NUM_MAX) {
        return IOT_FAILURE;
    }

    gpio_driver_data_t *gpio_data = &g_gpioMap[id];
    if (gpio_data->gpio_state == GPIO_PIN_INIT) {
        return IOT_FAILURE;
    }

    gpio_data->gpio_state = GPIO_PIN_INIT;
    InitIo((gpio_num_t)id, &(gpio_data->gpio_attr));
    gpio_config(&(gpio_data->gpio_attr));
    return IOT_SUCCESS;
}

unsigned int IoTGpioSetDir(unsigned int id, IotGpioDir dir)
{
    if (id >= GPIO_NUM_MAX) {
        return IOT_FAILURE;
    }

    gpio_driver_data_t *gpio_data = &g_gpioMap[id];
    if (dir == IOT_GPIO_DIR_IN) {
        gpio_data->gpio_attr.mode = GPIO_MODE_INPUT;
    } else {
        gpio_data->gpio_attr.mode = GPIO_MODE_OUTPUT;
    }

    esp_err_t ret = gpio_set_direction(id, gpio_data->gpio_attr.mode);
    return ESPErrToHoErr(ret);
}

unsigned int IoTGpioGetDir(unsigned int id, IotGpioDir *dir)
{
    if (id >= GPIO_NUM_MAX) {
        return IOT_FAILURE;
    }

    gpio_driver_data_t *gpio_data = &g_gpioMap[id];
    if (gpio_data->gpio_attr.mode == GPIO_MODE_DISABLE) {
        return IOT_FAILURE;
    } else if (gpio_data->gpio_attr.mode == GPIO_MODE_INPUT) {
        *dir = IOT_GPIO_DIR_IN;
    } else {
        *dir = IOT_GPIO_DIR_OUT;
    }

    return IOT_SUCCESS;
}

unsigned int IoTGpioSetOutputVal(unsigned int id, IotGpioValue val)
{
    if (id >= GPIO_NUM_MAX) {
        return IOT_FAILURE;
    }

    gpio_driver_data_t *gpio_data = &g_gpioMap[id];
    gpio_data->output_value = val;
    esp_err_t ret = gpio_set_level(id, val);
    return ESPErrToHoErr(ret);
}

unsigned int IoTGpioGetOutputVal(unsigned int id, IotGpioValue *val)
{
    if (id >= GPIO_NUM_MAX) {
        return IOT_FAILURE;
    }

    gpio_driver_data_t *gpio_data = &g_gpioMap[id];
    *val = gpio_data->output_value;
    return IOT_SUCCESS;
}

unsigned int IoTGpioGetInputVal(unsigned int id, IotGpioValue *val)
{
    if (id >= GPIO_NUM_MAX) {
        return IOT_FAILURE;
    }

    *val = gpio_get_level(id);
    return IOT_SUCCESS;
}

unsigned int IoTGpioRegisterIsrFunc(unsigned int id, IotGpioIntType intType, IotGpioIntPolarity intPolarity,
                                    GpioIsrCallbackFunc func, char *arg)
{
    if (id >= GPIO_NUM_MAX) {
        return IOT_FAILURE;
    }

    gpio_driver_data_t *gpio_data = &g_gpioMap[id];
    gpio_int_type_t intr_type = HoIntrToESPIntr(intType, intPolarity);
    if ((gpio_data->gpio_attr.intr_type != GPIO_INTR_DISABLE) || (intr_type == GPIO_INTR_DISABLE)) {
        return IOT_FAILURE;
    }

    gpio_data->gpio_attr.intr_type = intr_type;
    gpio_set_intr_type(id, intr_type);
    gpio_install_isr_service(0);
    esp_err_t ret = gpio_isr_handler_add(id, func, (void *)arg);
    return ESPErrToHoErr(ret);
}

unsigned int IoTGpioUnregisterIsrFunc(unsigned int id)
{
    if (id >= GPIO_NUM_MAX) {
        return IOT_FAILURE;
    }

    gpio_driver_data_t *gpio_data = &g_gpioMap[id];
    if (gpio_data->gpio_attr.intr_type == GPIO_INTR_DISABLE) {
        return IOT_FAILURE;
    }

    gpio_data->gpio_attr.intr_type = GPIO_INTR_DISABLE;
    gpio_intr_disable(id);
    esp_err_t ret = gpio_isr_handler_remove(id);
    gpio_uninstall_isr_service();
    return ESPErrToHoErr(ret);
}

unsigned int IoTGpioSetIsrMask(unsigned int id, unsigned char mask)
{
    if (id >= GPIO_NUM_MAX) {
        return IOT_FAILURE;
    }

    gpio_driver_data_t *gpio_data = &g_gpioMap[id];
    if (mask == true) {
        if (gpio_data->gpio_attr.intr_type == GPIO_INTR_DISABLE) {
            return IOT_SUCCESS;
        }

        gpio_data->gpio_attr.intr_type = GPIO_INTR_DISABLE;
        gpio_config(&gpio_data->gpio_attr);
        gpio_intr_disable(id);
    }

    return IOT_SUCCESS;
}

unsigned int IoTGpioSetIsrMode(unsigned int id, IotGpioIntType intType, IotGpioIntPolarity intPolarity)
{
    if (id >= GPIO_NUM_MAX) {
        return IOT_FAILURE;
    }

    gpio_driver_data_t *gpio_data = &g_gpioMap[id];
    gpio_int_type_t intr_type = HoIntrToESPIntr(intType, intPolarity);
    gpio_data->gpio_attr.intr_type = intr_type;
    gpio_config(&gpio_data->gpio_attr);
    return IOT_SUCCESS;
}

unsigned int IoTGpioDeinit(unsigned int id)
{
    if (id >= GPIO_NUM_MAX) {
        return IOT_FAILURE;
    }

    gpio_driver_data_t *gpio_data = &g_gpioMap[id];
    if (gpio_data->gpio_state == GPIO_PIN_INIT) {
        gpio_data->gpio_state = GPIO_PIN_UNINIT;
        if (gpio_data->gpio_attr.intr_type != GPIO_INTR_DISABLE) {
            gpio_intr_disable(id);
            esp_err_t ret = gpio_isr_handler_remove(id);
            gpio_uninstall_isr_service();
            gpio_data->gpio_attr.intr_type = GPIO_INTR_DISABLE;
            return ESPErrToHoErr(ret);
        }
    } else {
        return IOT_FAILURE;
    }

    if (memset_s(gpio_data, sizeof(gpio_driver_data_t),
                 0, sizeof(gpio_driver_data_t)) != EOK) {
        return IOT_FAILURE;
    }

    gpio_config(&gpio_data->gpio_attr);
    return IOT_SUCCESS;
}
