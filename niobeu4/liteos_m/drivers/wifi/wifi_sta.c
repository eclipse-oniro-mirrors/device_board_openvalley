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
#include <unistd.h>
#include "cmsis_os2.h"
#include "device_resource_if.h"
#include "hdf_base.h"
#include "hdf_device_desc.h"
#include "hdf_io_service_if.h"
#include "hdf_log.h"
#include "los_debug.h"
#include "los_task.h"
#include "lwip/netifapi.h"
#include "lwip/sockets.h"
#include "ohos_init.h"
#include "ohos_types.h"
#include "stdio.h"
#include "wifi.h"
#include "wifi_device.h"

#define TICK_COUNT 1000

#define INDEX_0 0
#define INDEX_1 1
#define INDEX_2 2
#define INDEX_3 3
#define INDEX_4 4
#define INDEX_5 5
#define INDEX_6 6
#define INDEX_7 7
#define INDEX_8 8
#define INDEX_9 9

#define MOV_8 8
#define MOV_16 16
#define MOV_24 24


INT32 app_main2(void);
int32_t EnableWifi(void);
int32_t DisableWifi(void);
int32_t Scan(void);
int32_t GetScanInfoList(WifiScanInfo *result, unsigned int *size);
int32_t AddDeviceConfig(const WifiDeviceConfig *config, int *result);
int32_t ConnectTo(int networkId);
int32_t RegisterWifiEvent_(WifiEvent *arg);
int32_t GetWifiConnectStatus(void);
void OnWifiConnectDevice(const char *wifi_name, const char *wifi_pwd);

static int32_t DeviceInit(struct HdfDeviceObject *object);
static int32_t DeviceBind(struct HdfDeviceObject *object);
static void DeviceRelease(struct HdfDeviceObject *object);

static const struct HdfDriverEntry WIFI_DriverEntry = {
    .moduleVersion = 1,
    .moduleName = "HDF_WIFI_MODULE",
    .Bind = DeviceBind,
    .Init = DeviceInit,
    .Release = DeviceRelease,
};
HDF_INIT(WIFI_DriverEntry);

typedef struct {
    uint32_t mode;
} DeviceInfo_t;

static const DeviceInfo_t DeviceInfoDefault = {
    .mode = -1,
};

// static const char TAG[]={"WIFI_STA"};
int g_wifiState = 0;

static void Delay(uint32_t ms)
{
    osDelay(ms * osKernelGetTickFreq() / TICK_COUNT);
}

/** Connection state change */
static void OnWifiConnectionChanged(int state, WifiLinkedInfo *info)
{
    if (!info) {
        return;
    }
    HDF_LOGE("%s , state = %d \r\n", __FUNCTION__, state);
    g_wifiState = state;
}

static int print_ap_info(WifiScanInfo *ap_info, int max_num)
{
    int index = -1;
    for (unsigned int i = 0; i < max_num; ++i) {
        HDF_LOGE("SSID\t\t%s\nRSSI\t\t%d\nfrequency\t%d\n", ap_info[i].ssid,
                 ap_info[i].rssi, ap_info[i].frequency);
        HDF_LOGE("MAC \t\t%02X:%02X:%02X:%02X:%02X:%02X\n",
                 ap_info[i].bssid[INDEX_0], ap_info[i].bssid[INDEX_1],
                 ap_info[i].bssid[INDEX_2], ap_info[i].bssid[INDEX_3],
                 ap_info[i].bssid[INDEX_4], ap_info[i].bssid[INDEX_5]);
        if (!strcmp((const char *)ap_info[i].ssid, "HUAWEIP40")) {
            index = i;
        }
    }
    return index;
}

static void OnWifiScanStateChanged(int state, int size)
{
    int ret;
    unsigned int maxi = size;
    if (!state || (size <= 0)) {
        return;
    }
    WifiScanInfo *ap_info = (WifiScanInfo *)malloc(sizeof(WifiScanInfo) * size);
    if (ap_info == NULL) {
        HDF_LOGE("malloc WifiScanInfo fail!\r\n");
        return;
    }
    WifiErrorCode error = GetScanInfoList(ap_info, &maxi);
    if (error != WIFI_SUCCESS || maxi == 0) {
        HDF_LOGE("WIFI NOT SUCCESS in GetScanInfoList\r\n");
        return;
    }
    WifiDeviceConfig config = {0};
    int index = print_ap_info(ap_info, maxi);
    if (index >= 0) {
        int netId = 0;
        HDF_LOGE("connect:%s\n", ap_info[index].ssid);
        config.freq = ap_info[index].frequency;
        config.securityType = ap_info[index].securityType;
        config.wapiPskType = WIFI_PSK_TYPE_ASCII;
        ret = memcpy_s(config.bssid, sizeof(config.bssid), ap_info[index].bssid, WIFI_MAC_LEN);
        if (ret != 0) {
            HDF_LOGE("memcpy_s bssid fail!\r\n");
            return HDF_FAILURE;
        }
        ret = strcpy_s(config.ssid, sizeof(config.bssid), ap_info[index].ssid);
        if (ret != 0) {
            HDF_LOGE("strcpy_s ssid fail!\r\n");
            return HDF_FAILURE;
        }
        free(ap_info);
        ret = strcpy_s(config.preSharedKey, sizeof(config.preSharedKey), "houpengfei8");
        if (ret != 0) {
            HDF_LOGE("strcpy_s password fail!\r\n");
            return HDF_FAILURE;
        }
        AddDeviceConfig(&config, &netId);
        ConnectTo(netId);
    } else {
        free(ap_info);
    }
}

/** Hotspot state change */
void OnHotspotStateChanged(int state)
{
    HDF_LOGE("%s state = %X\r\n", __FUNCTION__, state);
}
/** Station connected */
void OnHotspotStaJoin(StationInfo *info)
{
    if (!info) {
        HDF_LOGE("%s info=NULL\n", __FUNCTION__);
        return;
    }
    HDF_LOGE("%s name:%s mac:%02X:%02X:%02X:%02X:%02X:%02X ip:%d.%d.%d.%d connect:%d\r\n", __FUNCTION__, info->name,
             info->macAddress[INDEX_0], info->macAddress[INDEX_1], info->macAddress[INDEX_2],
             info->macAddress[INDEX_3], info->macAddress[INDEX_4], info->macAddress[INDEX_5],
             (info->ipAddress >> MOV_24) & 0xFF, (info->ipAddress >> MOV_16) & 0xFF,
             (info->ipAddress >> MOV_8) & 0xFF, (info->ipAddress) & 0xFF,
             info->disconnectedReason);
}
/** Station disconnected */
void OnHotspotStaLeave(StationInfo *info)
{
    if (!info) {
        HDF_LOGE("%s info=NULL\n", __FUNCTION__);
        return;
    }
    HDF_LOGE("%s name:%s mac:%02X:%02X:%02X:%02X:%02X:%02X ip:%d.%d.%d.%d connect:%d\r\n", __FUNCTION__, info->name,
             info->macAddress[INDEX_0], info->macAddress[INDEX_1], info->macAddress[INDEX_2],
             info->macAddress[INDEX_3], info->macAddress[INDEX_4], info->macAddress[INDEX_5],
             (info->ipAddress >> MOV_24) & 0xFF, (info->ipAddress >> MOV_16) & 0xFF,
             (info->ipAddress >> MOV_8) & 0xFF, (info->ipAddress) & 0xFF, info->disconnectedReason);
}

/**
 * @brief WiFi连接程序,CMSIS线程回调函数
 * @param arg 线程参数
 */
static void IotWifiConnectTask(void)
{
    WifiEvent eventListener = {
        .OnWifiConnectionChanged = OnWifiConnectionChanged,
        .OnWifiScanStateChanged = OnWifiScanStateChanged,
        .OnHotspotStateChanged = OnHotspotStateChanged,
        .OnHotspotStaJoin = OnHotspotStaJoin,
        .OnHotspotStaLeave = OnHotspotStaLeave};
    g_wifiState = 0;
    RegisterWifiEvent_(&eventListener);

    WifiErrorCode error = EnableWifi();
    HDF_LOGE("EnableWifi errCode: %d\r\n", error);
    if (error != WIFI_SUCCESS) {
        return;
    }

    error = Scan();
    HDF_LOGE("ScanWifi errCode: %d\r\n", error);
    if (error != WIFI_SUCCESS) {
        return;
    }
    OnWifiConnectDevice("HUAWEIP40", "houpengfei8");
    HDF_LOGE("GetWifiConnectStatus: %d\r\n", GetWifiConnectStatus());
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
        HDF_LOGE("%s.malloc\n", WIFI_DriverEntry.moduleName);
        return HDF_DEV_ERR_NO_MEMORY;
    }
    object->priv = (void *)dev;
    ret = memcpy_s(dev, sizeof(DeviceInfoDefault), &DeviceInfoDefault, sizeof(DeviceInfoDefault));
    if (ret != 0) {
        HDF_LOGE("memcpy_s DeviceInfoDefault fail!\r\n");
        return HDF_FAILURE;
    }
    if (object->property) {
        const struct DeviceResourceNode *node = object->property;
        struct DeviceResourceIface *resource = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);
        resource->GetUint32(node, "mode", &dev->mode, -1);
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
        OsalMemFree(object->priv);
    }
    object->priv = NULL;
}