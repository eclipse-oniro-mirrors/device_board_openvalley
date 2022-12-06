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
#include "hcs_macro.h"
#include "hdf_config_macro.h"
#include "hdf_device_desc.h"
#include "hdf_log.h"
#include "i2c_core.h"
#include "i2c_if.h"
#include "osal_mutex.h"
#include "driver/i2c.h"

#define HDF_LOG_TAG "ESP32U4_HDF_I2C"

struct RealI2cResource {
    uint8_t port;    // I2C端口号
    uint8_t mode;    // I2C主从模式  0:master 1:slave
    uint8_t scl_pin; // SCL引脚号
    uint8_t sda_pin; // SDA引脚号
    uint32_t speed;  // I2C时钟频率
};

static int32_t I2cDriverBind(struct HdfDeviceObject *device);
static int32_t I2cDriverInit(struct HdfDeviceObject *device);
static void I2cDriverRelease(struct HdfDeviceObject *device);
static int32_t I2cDataTransfer(struct I2cCntlr *cntlr, struct I2cMsg *msgs, int16_t count);

struct HdfDriverEntry gI2cHdfDriverEntry = {
    .moduleVersion = 1,
    .moduleName = "ESP32U4_HDF_I2C",
    .Bind = I2cDriverBind,
    .Init = I2cDriverInit,
    .Release = I2cDriverRelease,
};
HDF_INIT(gI2cHdfDriverEntry);

struct I2cMethod gI2cHostMethod = {
    .transfer = I2cDataTransfer,
};

#define I2C_FIND_CONFIG(node, name, resource)                       \
    do {                                                            \
        if (strcmp(HCS_PROP(node, match_attr), name) == 0) {        \
            (resource)->mode = HCS_PROP(node, mode);                \
            (resource)->port = HCS_PROP(node, port);                \
            (resource)->scl_pin = HCS_PROP(node, scl_pin);          \
            (resource)->sda_pin = HCS_PROP(node, sda_pin);          \
            (resource)->speed = HCS_PROP(node, speed);              \
            HDF_LOGE("----- I2C%d Config -----", (resource)->port); \
            HDF_LOGE("mode = %d", (resource)->mode);                \
            HDF_LOGE("scl_pin = %d", (resource)->scl_pin);          \
            HDF_LOGE("sda_pin = %d", (resource)->sda_pin);          \
            HDF_LOGE("speed = %d", (resource)->speed);              \
            result = HDF_SUCCESS;                                   \
        }                                                           \
    } while (0)
#define PLATFORM_CONFIG HCS_NODE(HCS_ROOT, platform)
#define PLATFORM_I2C_CONFIG HCS_NODE(HCS_NODE(HCS_ROOT, platform), i2c_config)
static uint32_t GetI2cDeviceResource(struct RealI2cResource *i2cResource, const char *deviceMatchAttr)
{
    int32_t result = HDF_FAILURE;
    struct RealI2cResource *resource = NULL;
    if (i2cResource == NULL || deviceMatchAttr == NULL) {
        HDF_LOGE("device or deviceMatchAttr is NULL\r\n");
        return HDF_ERR_INVALID_PARAM;
    }
    resource = i2cResource;
#if HCS_NODE_HAS_PROP(PLATFORM_CONFIG, i2c_config)
    HCS_FOREACH_CHILD_VARGS(PLATFORM_I2C_CONFIG, I2C_FIND_CONFIG, deviceMatchAttr, resource);
#endif
    if (result != HDF_SUCCESS) {
        HDF_LOGE("resourceNode %s is NULL\r\n", deviceMatchAttr);
    }
    return result;
}
#define TICKS_SECOND 1000
unsigned int DeviceI2cWrite(int id, unsigned char addr, const unsigned char *data, unsigned int dataLen, int ack)
{
    unsigned int ret;
    i2c_cmd_handle_t cmd;
    if (dataLen < 1) {
        return 0;
    }
    cmd = i2c_cmd_link_create();
    if ((ret = i2c_master_start(cmd)) != 0) {
        HDF_LOGE("DeviceI2cWrite i2c_master_start failed ret = %x", ret);
        return ret;
    }
    if ((ret = i2c_master_write_byte(cmd, addr | 0x00, ack)) != 0) {
        HDF_LOGE("DeviceI2cWrite i2c_master_write_addr failed ret = %x", ret);
        return ret;
    }

    if ((ret = i2c_master_write(cmd, data, dataLen, 1)) != 0) {
        HDF_LOGE("DeviceI2cWrite i2c_master_write failed ret = %x", ret);
        return ret;
    }

    if ((ret = i2c_master_stop(cmd)) != 0) {
        HDF_LOGE("DeviceI2cWrite i2c_master_stop failed ret = %x", ret);
        return ret;
    }
    if ((ret = i2c_master_cmd_begin(id, cmd, TICKS_SECOND / portTICK_RATE_MS)) != 0) {
        HDF_LOGE("DeviceI2cWrite i2c_master_cmd_begin failed ret = %d", ret);
        return ret;
    }
    i2c_cmd_link_delete(cmd);
    return ret;
}

unsigned int DeviceI2cRead(int id, unsigned char addr, const unsigned char *data, unsigned int dataLen, int ack)
{
    unsigned int ret;
    i2c_cmd_handle_t cmd;
    if (dataLen < 1) {
        return 0;
    }
    cmd = i2c_cmd_link_create();
    if ((ret = i2c_master_start(cmd)) != 0) {
        HDF_LOGE("DeviceI2cRead i2c_master_start failed ret = %x", ret);
        return ret;
    }

    if ((ret = i2c_master_write_byte(cmd, addr | 0x01, ack)) != 0) {
        HDF_LOGE("DeviceI2cRead i2c_master_write_addr failed ret = %x", ret);
        return ret;
    }

    if (dataLen > 1) {
        if ((ret = i2c_master_read(cmd, data, dataLen - 1, 0)) != 0) {
            HDF_LOGE("DeviceI2cRead i2c_master_read failed ret = %x", ret);
            return ret;
        }
    }

    if ((ret = i2c_master_read_byte(cmd, data + dataLen - 1, 1)) != 0) {
        HDF_LOGE("DeviceI2cRead i2c_master_read_byte failed ret = %x", ret);
        return ret;
    }

    if ((ret = i2c_master_stop(cmd)) != 0) {
        HDF_LOGE("DeviceI2cRead i2c_master_stop failed ret = %x", ret);
        return ret;
    }

    if ((ret = i2c_master_cmd_begin(id, cmd, TICKS_SECOND / portTICK_RATE_MS)) != 0) {
        HDF_LOGE("DeviceI2cRead i2c_master_cmd_begin failed ret = %x", ret);
        return ret;
    }
    i2c_cmd_link_delete(cmd);
    return ret;
}
#define MODE2 2
#define MODE4 4
static int DeviceI2cInit(int id, int scl_pin, int sda_pin, int speed, int mode)
{
    i2c_config_t conf;
    (void)memset_s(&conf, sizeof(conf), 0, sizeof(conf));
    conf.mode = (mode & 1) ? I2C_MODE_SLAVE : I2C_MODE_MASTER;
    conf.sda_io_num = sda_pin;
    conf.scl_io_num = scl_pin;
    conf.sda_pullup_en = (mode & MODE2) ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE;
    conf.scl_pullup_en = (mode & MODE4) ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE;
    conf.master.clk_speed = speed;
    i2c_param_config(id, &conf);
    return i2c_driver_install(id, conf.mode, 0, 0, 0);
}

static int32_t AttachI2cDevice(struct I2cCntlr *host, struct HdfDeviceObject *device)
{
    int32_t ret = HDF_FAILURE;

    if (host == NULL || device == NULL) {
        HDF_LOGE("[%s]: param is NULL\r\n", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    struct RealI2cResource *i2cResource = (struct RealI2cResource *)OsalMemAlloc(sizeof(struct RealI2cResource));
    if (i2cResource == NULL) {
        HDF_LOGE("[%s]: OsalMemAlloc RealI2cResource fail\r\n", __func__);
        return HDF_ERR_MALLOC_FAIL;
    }
    memset_s(i2cResource, sizeof(struct RealI2cResource), 0, sizeof(struct RealI2cResource));

    ret = GetI2cDeviceResource(i2cResource, device->deviceMatchAttr);

    DeviceI2cInit(i2cResource->port, i2cResource->scl_pin, i2cResource->sda_pin,
                  i2cResource->speed, i2cResource->mode);

    if (ret != HDF_SUCCESS) {
        OsalMemFree(i2cResource);
        return HDF_FAILURE;
    }

    host->busId = i2cResource->port;
    host->priv = i2cResource;

    return HDF_SUCCESS;
}

static int32_t I2cDataTransfer(struct I2cCntlr *cntlr, struct I2cMsg *msgs, int16_t count)
{
    unsigned int ret;
    if (cntlr == NULL || msgs == NULL || cntlr->priv == NULL) {
        HDF_LOGE("[%s]: I2cDataTransfer param is NULL\r\n", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    if (count <= 0) {
        HDF_LOGE("[%s]: I2cDataTransfer count err\r\n", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    struct RealI2cResource *device = (struct I2cDevice *)cntlr->priv;
    if (device == NULL) {
        HDF_LOGE("%s: I2cDevice is NULL\r\n", __func__);
        return HDF_DEV_ERR_NO_DEVICE;
    }

    for (; count > 0; --count, ++msgs) {
        if ((msgs->flags) == 0) { // write
            if (DeviceI2cWrite(cntlr->busId, msgs->addr, msgs->buf, msgs->len, 1))
                break;
        } else if ((msgs->flags) == I2C_FLAG_READ) { // read
            if (DeviceI2cRead(cntlr->busId, msgs->addr, msgs->buf, msgs->len, 1))
                break;
        }
    }
    return count;
}

static int32_t I2cDriverBind(struct HdfDeviceObject *device)
{
    return HDF_SUCCESS;
}

static int32_t I2cDriverInit(struct HdfDeviceObject *device)
{
    int32_t ret = HDF_FAILURE;
    struct I2cCntlr *host = NULL;
    if (device == NULL) {
        HDF_LOGE("[%s]: I2c device is NULL\r\n", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    host = (struct I2cCntlr *)OsalMemAlloc(sizeof(struct I2cCntlr));
    if (host == NULL) {
        HDF_LOGE("[%s]: malloc host is NULL\r\n", __func__);
        return HDF_ERR_MALLOC_FAIL;
    }

    memset_s(host, sizeof(struct I2cCntlr), 0, sizeof(struct I2cCntlr));
    host->ops = &gI2cHostMethod;
    device->priv = (VOID *)host;

    ret = AttachI2cDevice(host, device);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("[%s]: AttachI2cDevice error, ret = %d\r\n", __func__, ret);
        I2cDriverRelease(device);
        return HDF_DEV_ERR_ATTACHDEV_FAIL;
    }

    ret = I2cCntlrAdd(host);
    if (ret != HDF_SUCCESS) {
        I2cDriverRelease(device);
        return HDF_FAILURE;
    }
    HDF_LOGI("I2cDriverInit success!!");
    return HDF_SUCCESS;
}

static void I2cDriverRelease(struct HdfDeviceObject *device)
{
    if (device == NULL) {
        HDF_LOGE("%s: device is NULL\r\n", __func__);
        return;
    }

    struct I2cCntlr *i2cCntrl = device->priv;
    if (i2cCntrl == NULL || i2cCntrl->priv == NULL) {
        HDF_LOGE("%s: i2cCntrl is NULL\r\n", __func__);
        return;
    }
    i2cCntrl->ops = NULL;
    struct RealI2cResource *i2cDevice = (struct I2cDevice *)i2cCntrl->priv;
    OsalMemFree(i2cCntrl);

    if (i2cDevice != NULL) {
        OsalMemFree(i2cDevice);
    }
}
