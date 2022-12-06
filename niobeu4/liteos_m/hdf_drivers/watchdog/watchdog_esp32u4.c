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

#include <stdio.h>
#include <stdlib.h>
#include "hcs_macro.h"
#include "hdf_config_macro.h"
#include "hdf_device_desc.h"
#include "hdf_log.h"
#include "watchdog_core.h"
#include "watchdog_if.h"

#define REG32_READ(reg) (*(volatile uint32_t *)(reg))
#define REG32_WRITE(reg, value) (*(volatile uint32_t *)(reg) = value)

#define RWDT_PROTECT_KEY 0x50d83aa1
#define RWDT_PROTECT 0x3ff480a4
#define RWDT_CFG0 0x3ff4808c
#define RWDT_CFG0_VALUE ((1 << 10) | (7 << 11) | (3 << 28) | (1 << 31))
#define RWDT_CFG1 0x3ff48090
#define RWDT_FEED 0x3ff480a0
#define RWDT_TICK_BASE 0x3ff48070
#define RWDT_TICK ((REG32_READ(RWDT_TICK_BASE) >> 30) & 3)
#define RWDT_WOG() REG32_WRITE(RWDT_FEED, (1 << 31))
#define RWDT_IS_RUN() (REG32_READ(RWDT_CFG0) & (1 << 31))

#define SLOW_CK_TICK (150 * 1000)
#define XTAL_32K_TICK (32768)
#define CK8M_D256_OUT_TICK (8 * 1000 * 1000 / 256)

typedef struct {
    int watchdogId;
    int timeout; // Maximum interval between watchdog feeding, unit: ms
} WatchdogDeviceInfo;

static int g_watchdogStart = 0;
static int g_watchdogTimeout = 0;

static int32_t WatchdogDevStart(struct WatchdogCntlr *watchdogCntlr);
static int32_t WatchdogDevStop(struct WatchdogCntlr *watchdogCntlr);
static int32_t WatchdogDevSetTimeout(struct WatchdogCntlr *watchdogCntlr, uint32_t seconds);
static int32_t WatchdogDevGetTimeout(struct WatchdogCntlr *watchdogCntlr, uint32_t *seconds);
static int32_t WatchdogDevGetStatus(struct WatchdogCntlr *watchdogCntlr, uint32_t *status);
static int32_t WatchdogDevFeed(struct WatchdogCntlr *watchdogCntlr);

struct WatchdogMethod g_WatchdogCntlrMethod = {
    .getStatus = WatchdogDevGetStatus,
    .setTimeout = WatchdogDevSetTimeout,
    .getTimeout = WatchdogDevGetTimeout,
    .start = WatchdogDevStart,
    .stop = WatchdogDevStop,
    .feed = WatchdogDevFeed,
    .getPriv = NULL,
    .releasePriv = NULL,
};

#define WATCHDOG_FIND_CONFIG(node, name, device)             \
    do {                                                     \
        if (strcmp(HCS_PROP(node, match_attr), name) == 0) { \
            (device)->watchdogId = HCS_PROP(node, id);         \
            (device)->timeout = HCS_PROP(node, timeout);       \
            result = HDF_SUCCESS;                            \
        }                                                    \
    } while (0)

#define PLATFORM_CONFIG HCS_NODE(HCS_ROOT, platform)
#define PLATFORM_WATCHDOG_CONFIG HCS_NODE(HCS_NODE(HCS_ROOT, platform), watchdog_config)

static uint32_t GetWatchdogDeviceInfoResource(WatchdogDeviceInfo *device, const char *deviceMatchAttr)
{
    int32_t result = HDF_FAILURE;
    if (device == NULL || deviceMatchAttr == NULL) {
        HDF_LOGE("device or deviceMatchAttr is NULL");
        return HDF_ERR_INVALID_PARAM;
    }

    HCS_FOREACH_CHILD_VARGS(PLATFORM_WATCHDOG_CONFIG, WATCHDOG_FIND_CONFIG, deviceMatchAttr, device);

    if (result != HDF_SUCCESS) {
        HDF_LOGE("resourceNode %s is NULL", deviceMatchAttr);
        return result;
    }

    return HDF_SUCCESS;
}

static int32_t AttachWatchdogDeviceInfo(struct WatchdogCntlr *watchdogCntlr, struct HdfDeviceObject *device)
{
    int32_t ret;
    WatchdogDeviceInfo *watchdogdeviceinfo = NULL;

    if (device == NULL || device->deviceMatchAttr == NULL) {
        HDF_LOGE("%s: param is NULL", __func__);
        return HDF_FAILURE;
    }

    watchdogdeviceinfo = (WatchdogDeviceInfo *)OsalMemAlloc(sizeof(WatchdogDeviceInfo));
    if (watchdogdeviceinfo == NULL) {
        HDF_LOGE("%s: OsalMemAlloc WatchdogDeviceInfo error", __func__);
        return HDF_ERR_MALLOC_FAIL;
    }

    ret = GetWatchdogDeviceInfoResource(watchdogdeviceinfo, device->deviceMatchAttr);
    if (ret != HDF_SUCCESS) {
        (void)OsalMemFree(watchdogdeviceinfo);
        return HDF_FAILURE;
    }

    (void)OsalMutexInit(&watchdogCntlr->lock);

    watchdogCntlr->priv = watchdogdeviceinfo;
    watchdogCntlr->wdtId = watchdogdeviceinfo->watchdogId;

    return HDF_SUCCESS;
}

/* HdfDriverEntry method definitions */
static int32_t WatchdogDriverBind(struct HdfDeviceObject *device);
static int32_t WatchdogDriverInit(struct HdfDeviceObject *device);
static void WatchdogDriverRelease(struct HdfDeviceObject *device);

/* HdfDriverEntry definitions */
struct HdfDriverEntry g_watchdogDriverEntry = {
    .moduleVersion = 1,
    .moduleName = "ESP32U4_WATCHDOG_MODULE_HDF",
    .Bind = WatchdogDriverBind,
    .Init = WatchdogDriverInit,
    .Release = WatchdogDriverRelease,
};

// Initialize HdfDriverEntry
HDF_INIT(g_watchdogDriverEntry);

static int32_t WatchdogDriverBind(struct HdfDeviceObject *device)
{
    struct WatchdogCntlr *watchdogCntlr = NULL;
    if (device == NULL) {
        HDF_LOGE("hdfDevice object is null!");
        return HDF_FAILURE;
    }

    watchdogCntlr = (struct WatchdogCntlr *)OsalMemAlloc(sizeof(struct WatchdogCntlr));
    if (watchdogCntlr == NULL) {
        HDF_LOGE("%s: OsalMemAlloc watchdogCntlr error", __func__);
        return HDF_ERR_MALLOC_FAIL;
    }

    device->service = &watchdogCntlr->service;
    watchdogCntlr->device = device;
    watchdogCntlr->priv = NULL;
    return HDF_SUCCESS;
}

static int32_t WatchdogDriverInit(struct HdfDeviceObject *device)
{
    int32_t ret;
    struct WatchdogCntlr *watchdogCntlr = NULL;

    if (device == NULL) {
        HDF_LOGE("%s: device is NULL", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }

    watchdogCntlr = WatchdogCntlrFromDevice(device);
    if (watchdogCntlr == NULL) {
        HDF_LOGE("%s: watchdogCntlr is NULL", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    ret = AttachWatchdogDeviceInfo(watchdogCntlr, device);
    if (ret != HDF_SUCCESS) {
        OsalMemFree(watchdogCntlr);
        HDF_LOGE("%s:attach error", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    watchdogCntlr->ops = &g_WatchdogCntlrMethod;
    HDF_LOGI("WatchdogDriverInit success!");
    return ret;
}

static void WatchdogDriverRelease(struct HdfDeviceObject *device)
{
    struct WatchdogCntlr *watchdogCntlr = NULL;
    WatchdogDeviceInfo *watchdogdeviceinfo = NULL;

    if (device == NULL) {
        HDF_LOGE("device is null");
        return;
    }

    watchdogCntlr = WatchdogCntlrFromDevice(device);
    if (watchdogCntlr == NULL || watchdogCntlr->priv == NULL) {
        HDF_LOGE("%s: watchdogCntlr is NULL", __func__);
        return;
    }

    watchdogdeviceinfo = (WatchdogDeviceInfo *)watchdogCntlr->priv;
    if (watchdogdeviceinfo != NULL) {
        OsalMemFree(watchdogdeviceinfo);
    }
    return;
}

#define XTAL_TICK 1
#define CK8M_TICK 2

static int32_t WatchdogDevStart(struct WatchdogCntlr *watchdogCntlr)
{
    WatchdogDeviceInfo *watchdogdeviceinfo = NULL;
    int32_t watchdogId = 0;
    int32_t timeout = 0;
    int32_t ticks = 0;
    if (watchdogCntlr == NULL || watchdogCntlr->priv == NULL) {
        HDF_LOGE("%s: watchdogCntlr is NULL", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    watchdogdeviceinfo = (WatchdogDeviceInfo *)watchdogCntlr->priv;
    if (watchdogdeviceinfo == NULL) {
        HDF_LOGE("%s: OBJECT is NULL", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }

    watchdogId = watchdogdeviceinfo->watchdogId;
    timeout = watchdogdeviceinfo->timeout;

    switch (RWDT_TICK) {
        case XTAL_TICK:
            ticks = XTAL_32K_TICK;
            break;
        case CK8M_TICK:
            ticks = CK8M_D256_OUT_TICK;
            break;
        default:
            ticks = SLOW_CK_TICK;
            break;
    }
    REG32_WRITE(RWDT_PROTECT, RWDT_PROTECT_KEY);
    RWDT_WOG();
    REG32_WRITE(RWDT_CFG0, RWDT_CFG0_VALUE);
    REG32_WRITE(RWDT_CFG1, (timeout * ticks));
    REG32_WRITE(RWDT_PROTECT, 0);

    HDF_LOGI("Watchdog Started! timeout: %d second", timeout);
    g_watchdogStart = 1;
    return HDF_SUCCESS;
}

static int32_t WatchdogDevStop(struct WatchdogCntlr *watchdogCntlr)
{
    if (!watchdogCntlr) {
        return HDF_FAILURE;
    }
    if (!RWDT_IS_RUN()) {
        return HDF_SUCCESS;
    }
    REG32_WRITE(RWDT_PROTECT, RWDT_PROTECT_KEY);
    RWDT_WOG();
    REG32_WRITE(RWDT_CFG0, 0);
    REG32_WRITE(RWDT_PROTECT, 0);
    return HDF_SUCCESS;
}

static int32_t WatchdogDevSetTimeout(struct WatchdogCntlr *watchdogCntlr, uint32_t seconds)
{
    WatchdogDeviceInfo *watchdogdeviceinfo = NULL;
    if (watchdogCntlr == NULL || watchdogCntlr->priv == NULL) {
        HDF_LOGE("%s: watchdogCntlr is NULL", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    watchdogdeviceinfo = (WatchdogDeviceInfo *)watchdogCntlr->priv;
    if (watchdogdeviceinfo == NULL) {
        HDF_LOGE("%s: OBJECT is NULL", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }

    watchdogdeviceinfo->timeout = seconds;

    return HDF_SUCCESS;
}

static int32_t WatchdogDevGetTimeout(struct WatchdogCntlr *watchdogCntlr, uint32_t *seconds)
{
    WatchdogDeviceInfo *watchdogdeviceinfo = NULL;
    if (watchdogCntlr == NULL || seconds == NULL) {
        HDF_LOGE("%s: PARAM is NULL", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    watchdogdeviceinfo = (WatchdogDeviceInfo *)watchdogCntlr->priv;
    if (watchdogdeviceinfo == NULL) {
        HDF_LOGE("%s: OBJECT is NULL", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }

    *seconds = watchdogdeviceinfo->timeout;
    return HDF_SUCCESS;
}

static int32_t WatchdogDevGetStatus(struct WatchdogCntlr *watchdogCntlr, uint32_t *status)
{
    if (watchdogCntlr == NULL || status == NULL) {
        HDF_LOGE("%s: PARAM is NULL", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    if (g_watchdogStart == 1) {
        *status = WATCHDOG_START;
    } else {
        *status = WATCHDOG_STOP;
    }
    return HDF_SUCCESS;
}

static int32_t WatchdogDevFeed(struct WatchdogCntlr *watchdogCntlr)
{
    if (!watchdogCntlr) {
        return HDF_FAILURE;
    }
    REG32_WRITE(RWDT_PROTECT, RWDT_PROTECT_KEY);
    RWDT_WOG();
    REG32_WRITE(RWDT_PROTECT, 0);
    return HDF_SUCCESS;
}