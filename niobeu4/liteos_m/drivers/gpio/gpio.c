/*
 * Copyright (c) 2022 OpenValley Digital Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdlib.h>
#include <string.h>
#include "cmsis_os2.h"
#include "device_resource_if.h"
#include "gpio_core.h"
#include "gpio_if.h"
#include "hdf_base.h"
#include "hdf_device_desc.h"
#include "hdf_io_service_if.h"
#include "hdf_log.h"
#include "osal_irq.h"

#define GPIO_OUTPUT_IO_0 18
#define GPIO_OUTPUT_IO_1 19
#define GPIO_OUTPUT_PIN_SEL ((1ULL << GPIO_OUTPUT_IO_0) | (1ULL << GPIO_OUTPUT_IO_1))
#define GPIO_INPUT_IO_0 4
#define GPIO_INPUT_IO_1 5
#define GPIO_INPUT_PIN_SEL ((1ULL << GPIO_INPUT_IO_0) | (1ULL << GPIO_INPUT_IO_1))

#define LED_GPIO 5
#define LED_OFF 0x00
#define LED_ON 0x01
typedef void (*gpio_isr_t)(void *);

int32_t InitGpioDevice(int pin);
int32_t GpioDevWrite(int pin, int val);
int32_t GpioDevRead(int pin, unsigned int *val);
int32_t GpioDevSetDir(int pin, int dir);
int32_t GpioDevGetDir(int pin);
int32_t GpioDevSetIrq(int pin, int mode, int pin_bit_mask);
int32_t GpioDevUnSetIrq(int pin);
int32_t GpioDevEnableIrq(int pin, int int_type, gpio_isr_t isr_handler, void *args);
int32_t GpioDevDisableIrq(int pin);

struct GpioMethod g_GpioCntlrMethod = {
    .request = NULL,
    .release = NULL,
    .write = GpioDevWrite,
    .read = GpioDevRead,
    .setDir = GpioDevSetDir,
    .getDir = GpioDevGetDir,
    .toIrq = NULL,
    .setIrq = GpioDevSetIrq,
    .unsetIrq = GpioDevUnSetIrq,
    .enableIrq = GpioDevEnableIrq,
    .disableIrq = GpioDevDisableIrq,
};

static int32_t DeviceInit(struct HdfDeviceObject *object);
static int32_t DeviceBind(struct HdfDeviceObject *object);
static void DeviceRelease(struct HdfDeviceObject *object);

static const struct HdfDriverEntry GPIO_DriverEntry = {
    .moduleVersion = 1,
    .moduleName = "HDF_GPIO_MODULE",
    .Bind = DeviceBind,
    .Init = DeviceInit,
    .Release = DeviceRelease,
};
HDF_INIT(GPIO_DriverEntry);

typedef struct {
    int16_t power_pin;
    int16_t address;
    int8_t port;
    DevHandle handle;
} DeviceInfo_t;
static const DeviceInfo_t DeviceInfoDefault = {
    .power_pin = -1,
    .address = -1,
    .port = -1,
};
/**
 * @brief Enumerates GPIO directions.
 *
 * @since 1.0
 */
enum HDF_GpioDirType {
    HDF_GPIO_DIR_IN = (0x00000001),  /**< Input direction */
    HDF_GPIO_DIR_OUT = (0x00000002), /**< Output direction */
    HDF_GPIO_DIR_ERR,                /**< Invalid direction */
};
typedef enum {
    GPIO_INTR_DISABLE = 0,    /*!< Disable GPIO interrupt                             */
    GPIO_INTR_POSEDGE = 1,    /*!< GPIO interrupt type : rising edge                  */
    GPIO_INTR_NEGEDGE = 2,    /*!< GPIO interrupt type : falling edge                 */
    GPIO_INTR_ANYEDGE = 3,    /*!< GPIO interrupt type : both rising and falling edge */
    GPIO_INTR_LOW_LEVEL = 4,  /*!< GPIO interrupt type : input low level trigger      */
    GPIO_INTR_HIGH_LEVEL = 5, /*!< GPIO interrupt type : input high level trigger     */
    GPIO_INTR_MAX,
} gpio_int_type_t;

static void gpio_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    HDF_LOGE("gpio_isr_handler success !");
}

static int32_t DeviceInit(struct HdfDeviceObject *object)
{
    DeviceInfo_t *dev;
    int ret;
    static unsigned int g_irqData = 100;
    struct GpioCntlr *gpioCntlr = NULL;
    if (object == NULL) {
        return HDF_FAILURE;
    }
    dev = (DeviceInfo_t *)OsalMemAlloc(sizeof(DeviceInfo_t));
    if (dev == NULL) {
        HDF_LOGE("%s.malloc\n", GPIO_DriverEntry.moduleName);
        return HDF_DEV_ERR_NO_MEMORY;
    }
    object->priv = (void *)dev;
    ret = memcpy_s(dev, sizeof(DeviceInfoDefault), &DeviceInfoDefault, sizeof(DeviceInfoDefault));
    if (ret != 0) {
        HDF_LOGE("memcpy_s fail!\r\n");
        return HDF_FAILURE;
    }

    if (!OsalRegisterIrq(LED_GPIO, 0, gpio_isr_handler, "test", &g_irqData)) {
        HDF_LOGE("OsalRegisterIrq success!...\n\r");
    } else {
        HDF_LOGE("OsalRegisterIrq error!...\n\r");
    }
    if (!OsalDisableIrq(LED_GPIO)) {
        HDF_LOGE("OsalDisableIrq success!...\n\r");
    } else {
        HDF_LOGE("OsalDisableIrq error!...\n\r");
    }
    return HDF_SUCCESS;
}

static int32_t DeviceBind(struct HdfDeviceObject *object)
{
    return HDF_SUCCESS;
}

static void DeviceRelease(struct HdfDeviceObject *object)
{
    if (!object) {
        return;
    }

    if (object->priv) {
        DeviceInfo_t *dev = (DeviceInfo_t *)object->priv;
        InitGpioDevice(dev->port);
        OsalMemFree(object->priv);
    }
    object->priv = NULL;
}
