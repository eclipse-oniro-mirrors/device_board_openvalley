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
#include "osal_mutex.h"
#include "osal_sem.h"
#include "spi_core.h"
#ifdef LOSCFG_DRIVERS_HDF_CONFIG_MACRO
#include "hcs_macro.h"
#include "hdf_config_macro.h"
#else
#include "device_resource_if.h"
#endif
#include "hdf_log.h"
#include "osal_mem.h"
#include "gpio_types.h"
#include "driver/spi_common.h"
#include "driver/spi_master.h"

#ifndef LOSCFG_DRIVERS_HDF_CONFIG_MACRO
#error "LOSCFG_DRIVERS_HDF_CONFIG_MACRO"
#endif

typedef struct {
    uint8_t spi_num;
    uint8_t miso_pin;
    uint8_t mosi_pin;
    uint8_t sck_pin;
    uint8_t cs_pin;
    uint8_t mode;
    uint8_t queue_size;
    uint8_t dma_chn;
    uint8_t transferMode;
    uint8_t bitsPerWord;
    uint16_t openCnt;
    uint16_t max_transfer_size;
    uint32_t speed;
    spi_device_handle_t spi_handle;
} SpiDeviceSt;

static int32_t SpiDevGetCfg(struct SpiCntlr *spiCntlr, struct SpiCfg *spiCfg);
static int32_t SpiDevSetCfg(struct SpiCntlr *spiCntlr, struct SpiCfg *spiCfg);
static int32_t SpiDevTransfer(struct SpiCntlr *spiCntlr, struct SpiMsg *spiMsg, uint32_t count);
static int32_t SpiDevOpen(struct SpiCntlr *spiCntlr);
static int32_t SpiDevClose(struct SpiCntlr *spiCntlr);

static int32_t SpiDriverBind(struct HdfDeviceObject *device);
static int32_t SpiDriverInit(struct HdfDeviceObject *device);
static void SpiDriverRelease(struct HdfDeviceObject *device);

struct SpiCntlrMethod g_twSpiCntlrMethod = {
    .GetCfg = SpiDevGetCfg,
    .SetCfg = SpiDevSetCfg,
    .Transfer = SpiDevTransfer,
    .Open = SpiDevOpen,
    .Close = SpiDevClose,
};

struct HdfDriverEntry g_SpiDriverEntry = {
    .moduleVersion = 1,
    .moduleName = "ESP32U4_SPI_MODULE_HDF",
    .Bind = SpiDriverBind,
    .Init = SpiDriverInit,
    .Release = SpiDriverRelease,
};

HDF_INIT(g_SpiDriverEntry);

#define SPI_FIND_CONFIG(node, name, dev)                                  \
    do {                                                                  \
        if (strcmp(HCS_PROP(node, match_attr), name) == 0) {              \
            (dev)->spi_num = HCS_PROP(node, spi_num);                     \
            (dev)->miso_pin = HCS_PROP(node, miso_pin);                   \
            (dev)->mosi_pin = HCS_PROP(node, mosi_pin);                   \
            (dev)->sck_pin = HCS_PROP(node, sck_pin);                     \
            (dev)->cs_pin = HCS_PROP(node, cs_pin);                       \
            (dev)->speed = HCS_PROP(node, speed);                         \
            (dev)->mode = HCS_PROP(node, mode);                           \
            (dev)->max_transfer_size = HCS_PROP(node, max_transfer_size); \
            (dev)->queue_size = HCS_PROP(node, queue_size);               \
            (dev)->dma_chn = HCS_PROP(node, dma_chn);                     \
            result = HDF_SUCCESS;                                         \
        }                                                                 \
    } while (0)

#define PLATFORM_SPI_CONFIG HCS_NODE(HCS_NODE(HCS_ROOT, platform), spi_config)
static int32_t GetSpiDeviceResource(SpiDeviceSt *spiDevice, const char *deviceMatchAttr)
{
    int32_t result = HDF_FAILURE;
    if (spiDevice == NULL || deviceMatchAttr == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }
    HCS_FOREACH_CHILD_VARGS(PLATFORM_SPI_CONFIG, SPI_FIND_CONFIG, deviceMatchAttr, spiDevice);
    return result;
}

static int32_t AttachSpiDevice(struct SpiCntlr *spiCntlr, struct HdfDeviceObject *device)
{
    int32_t ret;
    SpiDeviceSt *spiDevice = NULL;

    if (spiCntlr == NULL || device == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }

    spiDevice = (SpiDeviceSt *)OsalMemCalloc(sizeof(SpiDeviceSt));
    if (spiDevice == NULL) {
        return HDF_ERR_MALLOC_FAIL;
    }

    ret = GetSpiDeviceResource(spiDevice, device->deviceMatchAttr);
    if (ret != HDF_SUCCESS) {
        (void)OsalMemFree(spiDevice);
        return HDF_FAILURE;
    }

    spiCntlr->priv = spiDevice;
    spiCntlr->busNum = spiDevice->spi_num;

    return HDF_SUCCESS;
}

static int32_t SpiDriverBind(struct HdfDeviceObject *device)
{
    struct SpiCntlr *spiCntlr = NULL;
    if (device == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }
    spiCntlr = SpiCntlrCreate(device);
    if (spiCntlr == NULL) {
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static int32_t SpiDriverInit(struct HdfDeviceObject *device)
{
    int32_t ret;
    struct SpiCntlr *spiCntlr = NULL;

    if (device == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }

    spiCntlr = SpiCntlrFromDevice(device);
    if (spiCntlr == NULL) {
        return HDF_DEV_ERR_NO_DEVICE;
    }

    ret = AttachSpiDevice(spiCntlr, device);
    if (ret != HDF_SUCCESS) {
        return HDF_DEV_ERR_ATTACHDEV_FAIL;
    }

    spiCntlr->method = &g_twSpiCntlrMethod;

    return ret;
}

static void SpiDriverRelease(struct HdfDeviceObject *device)
{
    SpiDeviceSt *dev;
    struct SpiCntlr *spiCntlr = NULL;

    if (device == NULL) {
        return;
    }

    spiCntlr = SpiCntlrFromDevice(device);
    if (spiCntlr == NULL) {
        return;
    }

    dev = (SpiDeviceSt *)spiCntlr->priv;
    if (dev == NULL) {
        return;
    }
    if (dev->openCnt) {
        dev->openCnt = 1;
        (void)SpiDevClose(spiCntlr);
    }
    OsalMemFree(spiCntlr->priv);
    spiCntlr->priv = NULL;
    return;
}
static int32_t SpiDevOpen(struct SpiCntlr *spiCntlr)
{
    esp_err_t err;
    SpiDeviceSt *dev;
    if (spiCntlr == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }
    dev = (SpiDeviceSt *)spiCntlr->priv;
    if (dev == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }
    if (dev->openCnt == 0) {
        spi_bus_config_t buscfg = {0};
        buscfg.miso_io_num = dev->miso_pin;
        buscfg.mosi_io_num = dev->mosi_pin;
        buscfg.sclk_io_num = dev->sck_pin;
        buscfg.quadwp_io_num = GPIO_NUM_NC;
        buscfg.quadhd_io_num = GPIO_NUM_NC;
        buscfg.max_transfer_sz = dev->max_transfer_size;
        err = spi_bus_initialize(dev->spi_num, &buscfg, dev->dma_chn);
        if (err != ESP_OK) {
            return HDF_ERR_IO;
        }
    }
    if (dev->openCnt == 0) {
        spi_device_interface_config_t devcfg = {0};
        devcfg.clock_speed_hz = dev->speed;
        devcfg.mode = dev->mode;
        devcfg.spics_io_num = dev->cs_pin;
        devcfg.queue_size = dev->queue_size;
        err = spi_bus_add_device(dev->spi_num, &devcfg, &dev->spi_handle);
        if (err != ESP_OK) {
            return HDF_ERR_DEVICE_BUSY;
        }
    }
    dev->openCnt++;
    return HDF_SUCCESS;
}

static int32_t SpiDevClose(struct SpiCntlr *spiCntlr)
{
    esp_err_t err;
    SpiDeviceSt *dev;
    if (spiCntlr == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }
    dev = (SpiDeviceSt *)spiCntlr->priv;
    if (dev == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }

    if (dev->openCnt > 0) {
        if (--dev->openCnt > 0) {
            return HDF_SUCCESS;
        }
        err = spi_bus_remove_device(dev->spi_handle);
        if (err != ESP_OK) {
            return HDF_ERR_DEVICE_BUSY;
        }
        dev->spi_handle = NULL;

        err = spi_bus_free(dev->spi_num);
        if (err != ESP_OK) {
            return HDF_ERR_IO;
        }
    }
    return HDF_SUCCESS;
}

static int32_t SpiDevGetCfg(struct SpiCntlr *spiCntlr, struct SpiCfg *spiCfg)
{
    SpiDeviceSt *dev;
    if (spiCntlr == NULL || spiCfg == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }
    dev = (SpiDeviceSt *)spiCntlr->priv;
    if (dev == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }

    spiCfg->maxSpeedHz = dev->speed;
    spiCfg->mode = dev->mode;
    spiCfg->transferMode = dev->transferMode;
    spiCfg->bitsPerWord = dev->bitsPerWord;
    return HDF_SUCCESS;
}
static int32_t SpiDevSetCfg(struct SpiCntlr *spiCntlr, struct SpiCfg *spiCfg)
{
    SpiDeviceSt *dev;
    if (spiCntlr == NULL || spiCfg == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }
    dev = (SpiDeviceSt *)spiCntlr->priv;
    if (dev == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }

    dev->speed = spiCfg->maxSpeedHz;
    dev->mode = spiCfg->mode;
    dev->transferMode = spiCfg->transferMode;
    dev->bitsPerWord = spiCfg->bitsPerWord;
    return HDF_SUCCESS;
}

static int32_t SpiDevTransfer(struct SpiCntlr *spiCntlr, struct SpiMsg *spiMsg, uint32_t count)
{
    esp_err_t err;
    SpiDeviceSt *dev;
    if (spiCntlr == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }
    dev = (SpiDeviceSt *)spiCntlr->priv;
    if (dev == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }
    for (size_t i = 0; i < count; i++) {
        struct SpiMsg *msg = &spiMsg[i];
        spi_transaction_t t = {0};
        if ((msg->wbuf == NULL) && (msg->rbuf == NULL)) {
            continue;
        }
        t.length = msg->len << '\x3';
        t.rx_buffer = msg->rbuf;
        t.tx_buffer = msg->wbuf;
        err = spi_device_polling_transmit(dev->spi_handle, &t);
        if (err != ESP_OK) {
            return HDF_ERR_DEVICE_BUSY;
        }
    }
    return HDF_SUCCESS;
}
