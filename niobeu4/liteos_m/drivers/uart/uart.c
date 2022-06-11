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
#include "uart_core.h"
#include "uart_if.h"

typedef struct {
    uint8_t port;
    uint8_t tx_io;
    uint8_t rx_io;
    uint8_t flow_ctrl;
    uint8_t rts_io;
    uint8_t cts_io;
} uartInfo_t;

int32_t uartInit(struct UartHost *host);

int32_t uartDeinit(struct UartHost *host);

int32_t uartRead(struct UartHost *host, uint8_t *data, uint32_t size);

int32_t uartWrite(struct UartHost *host, uint8_t *data, uint32_t size);

int32_t uartGetBaud(struct UartHost *host, uint32_t *baudRate);

int32_t uartSetBaud(struct UartHost *host, uint32_t baudRate);

int32_t uartGetAttribute(struct UartHost *host, struct UartAttribute *attribute);

int32_t uartSetAttribute(struct UartHost *host, struct UartAttribute *attribute);

int32_t uartSetTransMode(struct UartHost *host, enum UartTransMode mode);

int32_t uartpollEvent(struct UartHost *host, void *filep, void *table);

int32_t UartDeviceBind(struct HdfDeviceObject *object);

int32_t UartDeviceInit(struct HdfDeviceObject *object);

void UartDeviceRelease(struct HdfDeviceObject *object);

static const struct UartHostMethod g_uartMethodDefault = {
    .Init = uartInit,
    .Deinit = uartDeinit,
    .Read = uartRead,
    .Write = uartWrite,
    .GetAttribute = uartGetAttribute,
    .GetBaud = uartGetBaud,
    .pollEvent = uartpollEvent,
    .SetAttribute = uartSetAttribute,
    .SetBaud = uartSetBaud,
    .SetTransMode = uartSetTransMode,
};

static const uartInfo_t uartInfoDefault = {
    .port = -1,
    .tx_io = -1,
    .rx_io = -1,
    .flow_ctrl = -1,
    .rts_io = -1,
    .cts_io = -1,
};

static const struct HdfDriverEntry UartDriverEntry = {
    .moduleVersion = 1,
    .moduleName = "HDF_PLATFORM_UART",
    .Bind = UartDeviceBind,
    .Init = UartDeviceInit,
    .Release = UartDeviceRelease,
};

HDF_INIT(UartDriverEntry);

int32_t UartDeviceBind(struct HdfDeviceObject *object)
{
    esp_rom_printf("%s come in\n", __func__);
    if (object == NULL) {
        return HDF_ERR_INVALID_OBJECT;
    }
    return (UartHostCreate(object) == NULL) ? HDF_FAILURE : HDF_SUCCESS;
}

int32_t UartDeviceInit(struct HdfDeviceObject *object)
{
    int ret;
    uartInfo_t *dev;
    struct UartHost *host = NULL;
    esp_rom_printf("%s come in\n", __func__);

    if (object == NULL) {
        return HDF_FAILURE;
    }

    host = UartHostFromDevice(object);
    if (host == NULL) {
        esp_rom_printf("%s: host is null", __func__);
        return HDF_FAILURE;
    }

    dev = (uartInfo_t *)OsalMemAlloc(sizeof(uartInfo_t));
    if (dev == NULL) {
        esp_rom_printf("%s.malloc\n", UartDriverEntry.moduleName);
        return HDF_DEV_ERR_NO_MEMORY;
    }

    ret = memcpy_s(dev, sizeof(uartInfoDefault), &uartInfoDefault, sizeof(uartInfoDefault));
    if (ret != 0) {
        HDF_LOGE("memcpy_s fail!\r\n");
        return HDF_FAILURE;
    }
    if (object->property) {
        const struct DeviceResourceNode *node = object->property;
        struct DeviceResourceIface *resource = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);
        resource->GetUint8(node, "port", &dev->port, -1);
        resource->GetUint8(node, "tx_io", &dev->tx_io, -1);
        resource->GetUint8(node, "rx_io", &dev->rx_io, -1);
        resource->GetUint8(node, "flow_ctrl", &dev->flow_ctrl, -1);
        resource->GetUint8(node, "rts_io", &dev->rts_io, -1);
        resource->GetUint8(node, "cts_io", &dev->cts_io, -1);
    }

    host->priv = dev;
    host->num = dev->port;
    host->method = &g_uartMethodDefault;

    return HDF_SUCCESS;
}

void UartDeviceRelease(struct HdfDeviceObject *object)
{
    esp_rom_printf("%s come in\n", __func__);
    struct UartHost *host = NULL;
    if (!object) {
        return;
    }
    host = UartHostFromDevice(object);
    if (host == NULL) {
        esp_rom_printf("%s: host is null", __func__);
        return;
    }
    if (host->priv != NULL) {
        host->priv = NULL;
    }
    OsalMemFree(host->priv);
}
