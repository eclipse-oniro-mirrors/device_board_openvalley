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
#include "hdf_base.h"
#include "hdf_device_desc.h"
#include "hdf_io_service_if.h"
#include "hdf_log.h"
#include "i2c_core.h"
#include "i2c.h"

#define LOG_TAG "I2C"
#define I2C_SPEED (400 * 1000)

static int32_t DeviceInit(struct HdfDeviceObject *object);
static int32_t DeviceBind(struct HdfDeviceObject *object);
static void DeviceRelease(struct HdfDeviceObject *object);
static int32_t ClientOpen(struct HdfDeviceIoClient *client);
static int32_t ClientDispatch(struct HdfDeviceIoClient *client, int cmdId,
                              struct HdfSBuf *data, struct HdfSBuf *reply);
static void ClientRelease(struct HdfDeviceIoClient *client);
static int32_t DeviceTransfer(struct I2cCntlr *cntlr, struct I2cMsg *msgs, int16_t count);
int DeviceI2cInit(DeviceI2cParams *params);
unsigned int DeviceI2cWrite(int id, unsigned char addr, const unsigned char *data, unsigned int dataLen, int ack);
unsigned int DeviceI2cRead(int id, unsigned char addr, const unsigned char *data, unsigned int dataLen, int ack);

static const struct HdfDriverEntry I2C_DriverEntry = {
    .moduleVersion = 1,
    .moduleName = "HDF_I2C_MODULE",
    .Bind = DeviceBind,
    .Init = DeviceInit,
    .Release = DeviceRelease,
};
static struct IDeviceIoService DriverService = {
    .object.objectId = 0,
    .Open = ClientOpen,
    .Dispatch = ClientDispatch,
    .Release = ClientRelease,
};

#define DriverEntry I2C_DriverEntry
HDF_INIT(I2C_DriverEntry);

typedef struct {
    struct I2cCntlr cntlr;
    uint8_t scl_pin[3];
    uint8_t sda_pin[3];
    uint8_t mode;
    uint8_t port;
    uint32_t speed;
} DeviceInfo_t;

static const struct I2cMethod I2cOpsDefault = {
    .transfer = DeviceTransfer};
static const DeviceInfo_t DeviceInfoDefault = {
    .cntlr.ops = &I2cOpsDefault,
    .cntlr.lockOps = NULL,
};

static int32_t DeviceTransfer(struct I2cCntlr *cntlr, struct I2cMsg *msgs, int16_t count)
{
    for (; count > 0; --count, ++msgs) {
        if ((msgs->flags) == 0) {
            if (DeviceI2cWrite(cntlr->busId, msgs->addr, msgs->buf, msgs->len, 1)) {
                break;
            }
        } else if ((msgs->flags) == I2C_FLAG_READ) {
            if (DeviceI2cRead(cntlr->busId, msgs->addr, msgs->buf, msgs->len, 1)) {
                break;
            }
        }
    }
    return count;
}

static uint32_t DeviceIRQ(uint32_t irqId, void *dev)
{
    return 0;
}

static int32_t DeviceInit(struct HdfDeviceObject *object)
{
    DeviceInfo_t *dev;
    int ret;
    if (object == NULL) {
        return HDF_FAILURE;
    }
    dev = (DeviceInfo_t *)OsalMemAlloc(sizeof(DeviceInfo_t));
    if (dev == NULL) {
        HDF_LOGE("%s.malloc\n", DriverEntry.moduleName);
        return HDF_DEV_ERR_NO_MEMORY;
    }

    object->priv = (void *)dev;
    ret = memcpy_s(dev, sizeof(DeviceInfoDefault), &DeviceInfoDefault, sizeof(DeviceInfoDefault));
    if (ret != 0) {
        HDF_LOGE("memcpy_s fail!\r\n");
        return HDF_FAILURE;
    }
    if (object->property) {
        struct I2cCntlr *cntlr;
        const struct DeviceResourceNode *node = object->property;
        struct DeviceResourceIface *resource = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);
        cntlr = &dev->cntlr;
        resource->GetUint8(node, "port", &dev->port, 0);
        resource->GetUint8Array(node, "scl_pin", dev->scl_pin, sizeof(dev->scl_pin) / sizeof(dev->scl_pin[0]), 0);
        resource->GetUint8Array(node, "sda_pin", dev->sda_pin, sizeof(dev->sda_pin) / sizeof(dev->sda_pin[0]), 0);
        resource->GetUint8(node, "mode", &dev->mode, 0xFF);
        resource->GetUint32(node, "speed", &dev->speed, 0);
        cntlr->busId = dev->port;
    }

    DeviceI2cParams dev_params = {
        .cmd = 0,
        .id = dev->port,
        .scl_pin = dev->scl_pin[0],
        .sda_pin = dev->sda_pin[0],
        .speed = dev->speed,
        .mode = dev->mode,
    };
    DeviceI2cInit(&dev_params);

    if (I2cCntlrAdd(&dev->cntlr) != HDF_SUCCESS) {
        OsalMemFree(dev);
        HDF_LOGE("%s.I2cCntlrAdd\n", DriverEntry.moduleName);
        return HDF_FAILURE;
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
        I2cCntlrRemove(&dev->cntlr);
        DeviceI2cParams dev_params = {
            .cmd = -1,
            .id = dev->port,
            .scl_pin = dev->scl_pin,
            .sda_pin = dev->sda_pin,
            .speed = I2C_SPEED,
            .mode = dev->mode,
        };
        DeviceI2cInit(&dev_params);
        OsalMemFree(object->priv);
    }
    object->priv = NULL;
}