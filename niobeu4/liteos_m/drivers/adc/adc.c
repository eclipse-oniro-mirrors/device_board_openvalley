/*
 * Copyright (c) 2022 Hunan OpenValley Digital Industry Development Co., Ltd.
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
#include "adc_core.h"
#include "adc_if.h"
#include "cmsis_os2.h"
#include "device_resource_if.h"
#include "hdf_base.h"
#include "hdf_device_desc.h"
#include "hdf_io_service_if.h"
#include "hdf_log.h"
#include "osal_irq.h"

int32_t AdcDevOpen(struct AdcDevice *device, uint32_t number);
void AdcDevClose(struct AdcDevice *device);
int32_t AdcDevRead(struct AdcDevice *device, uint32_t number, uint32_t channel, uint32_t *val);

struct AdcMethod g_AdcCntlrMethod = {
    .read = AdcDevRead,
    .start = AdcDevOpen,
    .stop = AdcDevClose,
};

static int32_t DeviceInit(struct HdfDeviceObject *object);
static int32_t DeviceBind(struct HdfDeviceObject *object);
static void DeviceRelease(struct HdfDeviceObject *object);

static const struct HdfDriverEntry ADC_DriverEntry = {
    .moduleVersion = 1,
    .moduleName = "HDF_ADC_MODULE",
    .Bind = DeviceBind,
    .Init = DeviceInit,
    .Release = DeviceRelease,
};
HDF_INIT(ADC_DriverEntry);

typedef struct {
    uint32_t unit;
    uint32_t channel;
} DeviceInfo_t;

static const DeviceInfo_t DeviceInfoDefault = {
    .unit = -1,
    .channel = -1,
};

static int32_t DeviceInit(struct HdfDeviceObject *object)
{
    DeviceInfo_t *dev;
    int ret;
    if (object == NULL) {
        return HDF_FAILURE;
    }

    dev = (DeviceInfo_t *)OsalMemAlloc(sizeof(DeviceInfo_t));
    if (dev == NULL) {
        HDF_LOGE("%s.malloc\n", ADC_DriverEntry.moduleName);
        return HDF_DEV_ERR_NO_MEMORY;
    }
    object->priv = (void *)dev;
    ret = memcpy_s(dev, sizeof(DeviceInfoDefault), &DeviceInfoDefault, sizeof(DeviceInfoDefault));
    if (ret != 0) {
        HDF_LOGE("memcpy_s fail!\n");
        return HDF_FAILURE;
    }
    if (object->property) {
        const struct DeviceResourceNode *node = object->property;
        struct DeviceResourceIface *resource = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);
        resource->GetUint32(node, "unit", &dev->unit, -1);
        resource->GetUint32(node, "channel", &dev->channel, -1);
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
        AdcDevClose(AdcDevClose);
        OsalMemFree(object->priv);
    }
    object->priv = NULL;
}