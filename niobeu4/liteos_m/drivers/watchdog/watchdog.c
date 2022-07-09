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
#include "cmsis_os2.h"
#include "device_resource_if.h"
#include "hdf_base.h"
#include "hdf_device_desc.h"
#include "hdf_io_service_if.h"
#include "hdf_log.h"
#include "watchdog_core.h"

#define RWDT_PROTECT_KEY 0x50d83aa1
#define RWDT_PROTECT 0x3ff480a4
#define RWDT_CFG0 0x3ff4808c
#define RWDT_CFG0_VALUE ((1 << 10) | (7 << 11) | (3 << 28) | (1 << 31))
#define RWDT_CFG1 0x3ff48090
#define RWDT_FEED 0x3ff480a0
#define RWDT_TICKS_TYPE 0x3ff48070
#define RWDT_WOG_VALUE (1 << 31)
#define RWDT_IS_RUN_BIT (1 << 31)
#define OFFSET_30   30
#define BIT_3   3
#define SLOW_CK_TICKS (150 * 1000)
#define XTAL_32K_TICKS (32768)
#define CK8M_D256_OUT_CK_TICKS (8 * 1000 * 1000 / 256)
#define WATCHDOG_TIMEOUT 5
static int32_t WatchdogDeviceInit(struct HdfDeviceObject *object);
static int32_t WatchdogDeviceBind(struct HdfDeviceObject *object);
static void WatchdogDeviceRelease(struct HdfDeviceObject *object);

static int32_t WatchdogDevGetStatus(struct WatchdogCntlr *wdt, int32_t *status);
static int32_t WatchdogDevSetTimeout(struct WatchdogCntlr *wdt, uint32_t seconds);
static int32_t WatchdogDevGetTimeout(struct WatchdogCntlr *wdt, uint32_t *seconds);
static int32_t WatchdogDevStart(struct WatchdogCntlr *wdt);
static int32_t WatchdogDevStop(struct WatchdogCntlr *wdt);
static int32_t WatchdogDevFeed(struct WatchdogCntlr *wdt);
static int32_t WatchdogDevGetPriv(struct WatchdogCntlr *wdt);
static void WatchdogDevReleasePriv(struct WatchdogCntlr *wdt);

typedef struct {
    struct WatchdogCntlr cntlr;
    uint32_t timeout;
} PrivWatchdog_t;

static struct WatchdogMethod WatchdogCntlrMethod = {
    .getStatus = WatchdogDevGetStatus,
    .setTimeout = WatchdogDevSetTimeout,
    .getTimeout = WatchdogDevGetTimeout,
    .start = WatchdogDevStart,
    .stop = WatchdogDevStop,
    .feed = WatchdogDevFeed,
    .getPriv = NULL,
    .releasePriv = NULL};

static const struct HdfDriverEntry WatchdogDriverEntry = {
    .moduleVersion = 1,
    .moduleName = "HDF_WATCHDOG_MODULE",
    .Bind = WatchdogDeviceBind,
    .Init = WatchdogDeviceInit,
    .Release = WatchdogDeviceRelease,
};

#define DriverEntry WatchdogDriverEntry
HDF_INIT(WatchdogDriverEntry);

static uint32_t GetRwdtTicksType()
{
    volatile uint32_t *ptr = RWDT_TICKS_TYPE;
    return (((*ptr) >> OFFSET_30) & BIT_3);
}

static void RWDT_WOG(void)
{
    *(volatile uint32_t *)RWDT_FEED = RWDT_WOG_VALUE;
}

static uint32_t RWDT_IS_RUN(void)
{
    return (*(volatile uint32_t *)RWDT_CFG0) & RWDT_IS_RUN_BIT;
}

static int32_t WatchdogDeviceInit(struct HdfDeviceObject *object)
{
    if (!object)
        return HDF_ERR_INVALID_OBJECT;
    PrivWatchdog_t *dev;
    int ret;
    dev = (PrivWatchdog_t *)OsalMemAlloc(sizeof(PrivWatchdog_t));
    if (dev == NULL) {
        HDF_LOGE("%s.malloc\n", DriverEntry.moduleName);
        return HDF_DEV_ERR_NO_MEMORY;
    }
    ret = memset_s(dev, sizeof(PrivWatchdog_t), 0, sizeof(PrivWatchdog_t));
    if (ret != 0) {
        HDF_LOGE("memset_s fail!\r\n");
        return HDF_FAILURE;
    }
    object->priv = (void *)dev;
    object->service = &dev->cntlr.service;
    dev->timeout = WATCHDOG_TIMEOUT;
    dev->cntlr.wdtId = 0;
    dev->cntlr.device = object;
    dev->cntlr.ops = &WatchdogCntlrMethod;
    dev->cntlr.priv = (void *)dev;
    if (WatchdogCntlrAdd(&dev->cntlr) != HDF_SUCCESS) {
        OsalMemFree(dev);
        HDF_LOGE("%s.WatchdogCntlrAdd\n", DriverEntry.moduleName);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}
static int32_t WatchdogDeviceBind(struct HdfDeviceObject *object)
{
    return HDF_SUCCESS;
}
static void WatchdogDeviceRelease(struct HdfDeviceObject *object)
{
    if (!object) {
        return;
    }
    if (object->priv) {
        return;
    }
    WatchdogDevStop(&((PrivWatchdog_t *)object->priv)->cntlr);
    OsalMemFree(object->priv);
    object->priv = NULL;
    return;
}

static int32_t WatchdogDevGetStatus(struct WatchdogCntlr *wdt, int32_t *status)
{
    if (!wdt) {
        return HDF_FAILURE;
    }
    if (status) {
        *status = RWDT_IS_RUN() ? 1 : 0;
    }
    return HDF_SUCCESS;
}
static int32_t WatchdogDevSetTimeout(struct WatchdogCntlr *wdt, uint32_t seconds)
{
    if (!wdt)
        return HDF_FAILURE;
    if (!wdt->priv)
        return HDF_FAILURE;
    ((PrivWatchdog_t *)wdt->priv)->timeout = seconds;
    return HDF_SUCCESS;
}
static int32_t WatchdogDevGetTimeout(struct WatchdogCntlr *wdt, uint32_t *seconds)
{
    if (!wdt)
        return HDF_FAILURE;
    if (seconds) {
        if (!wdt->priv)
            return HDF_FAILURE;
        *seconds = ((PrivWatchdog_t *)wdt->priv)->timeout;
    }
    return HDF_SUCCESS;
}
static int32_t WatchdogDevStart(struct WatchdogCntlr *wdt)
{
    uint32_t ticks;
    if (!wdt)
        return HDF_FAILURE;
    if (!wdt->priv)
        return HDF_FAILURE;
    switch (GetRwdtTicksType()) {
        case 1:
            ticks = XTAL_32K_TICKS;
            break; // XTAL_32K
        case 2:
            ticks = CK8M_D256_OUT_CK_TICKS;
            break; // CK8M_D256_OUT
        default:
            ticks = SLOW_CK_TICKS;
            break; // SLOW_CK
    }
    // ====== RTC_WDT_CFG0
    /* 7:     pause WDT in sleep */
    /* 8:     enable WDT reset APP CPU */
    /* 9:     enable WDT reset PRO CPU */
    /* 10:    enable WDT in flash boot */
    /* 11-13: system reset counter length */
    /* 14-16: CPU reset counter length */
    /* 17:    When set, level type interrupt generation is enabled */
    /* 18:    When set, edge type interrupt generation is enabled */
    /* 19-21: 1: interrupt stage en  2: CPU reset stage en  3: system reset stage en  4: RTC reset stage en */
    /* 22-24: 1: interrupt stage en  2: CPU reset stage en  3: system reset stage en  4: RTC reset stage en */
    /* 25-27: 1: interrupt stage en  2: CPU reset stage en  3: system reset stage en  4: RTC reset stage en */
    /* 28-30: 1: interrupt stage en  2: CPU reset stage en  3: system reset stage en  4: RTC reset stage en */
    /* 31:    enable RTC WDT */
    *(volatile uint32_t *)RWDT_PROTECT = RWDT_PROTECT_KEY;
    RWDT_WOG();
    *(volatile uint32_t *)RWDT_CFG0 = RWDT_CFG0_VALUE;
    // 最大超时时间 4G / (150K 或 32768 或 8M/256)
    *(volatile uint32_t *)RWDT_CFG1 = ((PrivWatchdog_t *)wdt->priv)->timeout * ticks;
    *(volatile uint32_t *)RWDT_PROTECT = 0;
    return HDF_SUCCESS;
}
static int32_t WatchdogDevStop(struct WatchdogCntlr *wdt)
{
    if (!wdt) {
        return HDF_FAILURE;
    }
    if (!RWDT_IS_RUN()) {
        return HDF_SUCCESS;
    }
    *(volatile uint32_t *)RWDT_PROTECT = RWDT_PROTECT_KEY;
    RWDT_WOG();
    *(volatile uint32_t *)RWDT_CFG0 = 0;
    *(volatile uint32_t *)RWDT_PROTECT = 0;
    return HDF_SUCCESS;
}

static int32_t WatchdogDevFeed(struct WatchdogCntlr *wdt)
{
    if (!wdt) {
        return HDF_FAILURE;
    }
    *(volatile uint32_t *)RWDT_PROTECT = RWDT_PROTECT_KEY;
    RWDT_WOG();
    *(volatile uint32_t *)RWDT_PROTECT = 0;
    return HDF_SUCCESS;
}
static int32_t WatchdogDevGetPriv(struct WatchdogCntlr *wdt)
{
    if (!wdt) {
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}
static void WatchdogDevReleasePriv(struct WatchdogCntlr *wdt)
{
}
