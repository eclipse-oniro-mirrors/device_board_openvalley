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
#include "esp_wifi.h"
#include "cmsis_os2.h"
#include "esp_event.h"
#include "esp_event_legacy.h"
#include "los_memory.h"
#include "los_mux.h"
#include "los_task.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/tcpip.h"
#include "station_info.h"
#include "wifi_error_code.h"
#include "wifi_event.h"
#include "wifi_hotspot.h"
#include "wifi_linked_info.h"
#include "wifi_device.h"

#undef LOG
#undef LOGE

#define LOG
#define LOGE(fmt, ...) printf("Error %s%s." fmt "\r\n", TAG, __func__, ##__VA_ARGS__)

#define CHANNEL_80211B_ONLY 14
#define FREQ_OF_CHANNEL_1 2412
#define FREQ_OF_CHANNEL_80211B_ONLY 2484
#define WIFI_MIN_CHANNEL 1
#define WIFI_FREQ_INTERVAL 5
#define ESP_EXAMPLE_MAX_STA_CONN 4

#define RSSI_LEVEL_4_2_G (-65)
#define RSSI_LEVEL_3_2_G (-75)
#define RSSI_LEVEL_2_2_G (-82)
#define RSSI_LEVEL_1_2_G (-88)
#define RSSI_LEVEL_4_5_G (-65)
#define RSSI_LEVEL_3_5_G (-72)
#define RSSI_LEVEL_2_5_G (-79)
#define RSSI_LEVEL_1_5_G (-85)

#define WIFI_ACTIVE 1
#define WIFI_NOT_ACTIVE 0
#define EPS_STR_LEN 4
#define DELAY_10_TICK 10
#define DELAY_30_TICK 30
#define DELAY_50_TICK 50
#define DELAY_LOOP_TIMES 50
#define MAX_INDEX 60
#define MUX_HANDLE_OFFSET 8
#define ERROR_ESP_WIFI_START (-103)
#define ERROR_NETIF_NULL (-8)
#define ERROR_REGISTER_FAIL (-9)
#define SCANING 0x50

typedef struct {
    volatile uint8_t ip_ok;
    volatile uint8_t scan_ok;
    union {
        uint8_t staStatus;
        uint8_t apStatus;
    };
    WifiEvent *event[WIFI_MAX_EVENT_SIZE];
    esp_netif_t *netif;
    WifiDeviceConfig config[WIFI_MAX_CONFIG_SIZE];
    HotspotConfig hotConfig[1];
    UINT32 muxHandle;
    esp_event_handler_instance_t eventHandle[2];
} DevWifiInfo_t;

static DevWifiInfo_t DevWifiInfo = {0};
static const char TAG[] = {"WifiLite."};
static const char NullBssid[WIFI_MAC_LEN] = {0, 0, 0, 0, 0, 0};

static void MEMCPY_S(VOID *dst, int dstSize, VOID *src, int srcSize)
{
    if ((dst == NULL) || (src == NULL)) {
        return;
    }
    int ret;
    ret = memcpy_s(dst, dstSize, src, (dstSize > srcSize) ? srcSize : dstSize);
    if (ret != 0) {
        LOGE("memcpy_s fail!!\n");
    }
}

static void WifiLock(void)
{
    if (!DevWifiInfo.muxHandle) {
        UINT32 muxHandle = 0;
        if (LOS_OK != LOS_MuxCreate(&muxHandle)) {
            LOGE("LOS_MuxCreate");
            return;
        }
        DevWifiInfo.muxHandle = muxHandle + MUX_HANDLE_OFFSET;
    }
    LOS_MuxPend(DevWifiInfo.muxHandle - MUX_HANDLE_OFFSET, LOS_WAIT_FOREVER);
}

static void WifiUnlock(void)
{
    if (!DevWifiInfo.muxHandle)
        return;
    LOS_MuxPost(DevWifiInfo.muxHandle - MUX_HANDLE_OFFSET);
}

int IsWifiActive(void)
{
    return ((DevWifiInfo.staStatus == WIFI_ACTIVE) ? WIFI_STA_ACTIVE : WIFI_STA_NOT_ACTIVE);
}

WifiErrorCode UnRegisterWifiEvent(const WifiEvent *event)
{
    WifiErrorCode ret = ERROR_WIFI_UNKNOWN;
    if (!event)
        return ERROR_WIFI_INVALID_ARGS;

    WifiLock();
    for (unsigned i = 0; i < WIFI_MAX_EVENT_SIZE; i++) {
        if (DevWifiInfo.event[i] == event) {
            DevWifiInfo.event[i] = NULL;
            ret = WIFI_SUCCESS;
            break;
        }
    }
    WifiUnlock();

    return ret;
}

WifiErrorCode RegisterWifiEvent(WifiEvent *event)
{
    WifiErrorCode ret = ERROR_WIFI_UNKNOWN;

    if (event == NULL) {
        return ERROR_WIFI_INVALID_ARGS;
    }

    WifiLock();
    for (unsigned i = 0; i < WIFI_MAX_EVENT_SIZE; i++) {
        if (DevWifiInfo.event[i] == event) {
            ret = ERROR_WIFI_INVALID_ARGS;
            break;
        }
        if (DevWifiInfo.event[i] == NULL) {
            DevWifiInfo.event[i] = event;
            ret = WIFI_SUCCESS;
            break;
        }
    }
    WifiUnlock();
    return ret;
}

static unsigned int ChannelToFrequency(unsigned int channel)
{
    if (channel <= 0) {
        return 0;
    }
    if (channel == CHANNEL_80211B_ONLY) {
        return FREQ_OF_CHANNEL_80211B_ONLY;
    }
    return (((channel - WIFI_MIN_CHANNEL) * WIFI_FREQ_INTERVAL) + FREQ_OF_CHANNEL_1);
}

static unsigned int FrequencyToChannel(unsigned int frequency)
{
    if (frequency == FREQ_OF_CHANNEL_80211B_ONLY) {
        return CHANNEL_80211B_ONLY;
    }
    if (frequency < FREQ_OF_CHANNEL_1) {
        return 0;
    }
    return (frequency - FREQ_OF_CHANNEL_1) / WIFI_FREQ_INTERVAL + WIFI_MIN_CHANNEL;
}

/** Scan state change */
static void SendOnWifiScanStateChanged(DevWifiInfo_t *info, WifiEventState event, uint16_t size)
{
    WifiEvent **pevent = info->event;
    for (unsigned i = 0; i < WIFI_MAX_EVENT_SIZE; i++) {
        if (pevent[i] == NULL) {
            continue;
        }
        if (pevent[i]->OnWifiScanStateChanged == NULL) {
            continue;
        }
        pevent[i]->OnWifiScanStateChanged(event, size);
    }
}

/** Connection state change */
static void SendOnWifiConnectionChanged(DevWifiInfo_t *info, WifiEventState event, WifiLinkedInfo *linkInfo)
{
    WifiEvent **pevent = info->event;
    for (unsigned i = 0; i < WIFI_MAX_EVENT_SIZE; i++) {
        if (pevent[i] == NULL) {
            continue;
        }
        if (pevent[i]->OnWifiConnectionChanged == NULL) {
            continue;
        }
        pevent[i]->OnWifiConnectionChanged(event, linkInfo);
    }
}

/** Station connected */
static void SendOnHotspotStaJoin(DevWifiInfo_t *info, StationInfo *staInfo)
{
    WifiEvent **pevent = info->event;
    for (unsigned i = 0; i < WIFI_MAX_EVENT_SIZE; i++) {
        if (pevent[i] == NULL) {
            continue;
        }
        if (pevent[i]->OnHotspotStaJoin == NULL) {
            continue;
        }
        pevent[i]->OnHotspotStaJoin(staInfo);
    }
}

/** Station disconnected */
static void SendOnHotspotStaLeave(DevWifiInfo_t *info, StationInfo *staInfo)
{
    WifiEvent **pevent = info->event;
    for (unsigned i = 0; i < WIFI_MAX_EVENT_SIZE; i++) {
        if (pevent[i] == NULL) {
            continue;
        }
        if (pevent[i]->OnHotspotStaLeave == NULL) {
            continue;
        }
        pevent[i]->OnHotspotStaLeave(staInfo);
    }
}

/** Hotspot state change */
static void SendOnHotspotStateChanged(DevWifiInfo_t *info, WifiEventState event)
{
    WifiEvent **pevent = info->event;
    for (unsigned i = 0; i < WIFI_MAX_EVENT_SIZE; i++) {
        if (pevent[i] == NULL) {
            continue;
        }
        if (pevent[i]->OnHotspotStateChanged == NULL) {
            continue;
        }
        pevent[i]->OnHotspotStateChanged(event);
    }
}

static void event_got_ip_handler(VOID *arg, esp_event_base_t event_base,
                                 int32_t event_id, VOID *event_data)
{
    DevWifiInfo.ip_ok = 1;
}

static void wifi_event_scan_down_proc(VOID *event_data)
{
    uint16_t size = 0;
    DevWifiInfo.scan_ok = 1;
    esp_wifi_scan_get_ap_num(&size);
    SendOnWifiScanStateChanged(&DevWifiInfo, WIFI_STATE_AVAILABLE, size);
}

static void wifi_event_sta_connected_proc(VOID *event_data)
{
    WifiLinkedInfo linkInfo = {0};
    wifi_ap_record_t ap_info;
    esp_wifi_sta_get_ap_info(&ap_info);
    MEMCPY_S(&linkInfo.ssid, sizeof(linkInfo.ssid), ap_info.ssid, sizeof(ap_info.ssid));
    MEMCPY_S(&linkInfo.bssid, sizeof(linkInfo.bssid), ap_info.bssid, sizeof(ap_info.bssid));
    linkInfo.rssi = ap_info.rssi;
    linkInfo.connState = WIFI_CONNECTED;
    linkInfo.frequency = ChannelToFrequency(ap_info.primary);
    SendOnWifiConnectionChanged(&DevWifiInfo, WIFI_STATE_AVAILABLE, &linkInfo);
}

static void wifi_event_sta_disconnected_proc(VOID *event_data)
{
    WifiLinkedInfo linkInfo = {0};
    wifi_event_sta_disconnected_t *disconnected = (wifi_event_sta_disconnected_t *)event_data;
    MEMCPY_S(&linkInfo.ssid, sizeof(linkInfo.ssid), disconnected->ssid, sizeof(disconnected->ssid));
    MEMCPY_S(&linkInfo.bssid, sizeof(linkInfo.bssid), disconnected->bssid, sizeof(disconnected->bssid));
    linkInfo.disconnectedReason = disconnected->reason;
    linkInfo.connState = WIFI_DISCONNECTED;
    SendOnWifiConnectionChanged(&DevWifiInfo, WIFI_STATE_NOT_AVAILABLE, &linkInfo);
}

static void wifi_event_ap_connected_proc(VOID *event_data)
{
    StationInfo staInfo = {0};
    wifi_event_ap_staconnected_t *connect_event = (wifi_event_ap_staconnected_t *)event_data;
    MEMCPY_S(&staInfo.macAddress, sizeof(staInfo.macAddress), connect_event->mac, sizeof(connect_event->mac));
    SendOnHotspotStaJoin(&DevWifiInfo, &staInfo);
}

static void wifi_event_ap_disconnected_proc(VOID *event_data)
{
    StationInfo staInfo = {0};
    wifi_event_ap_stadisconnected_t *disconnect_event = (wifi_event_ap_stadisconnected_t *)event_data;
    MEMCPY_S(&staInfo.macAddress, sizeof(staInfo.macAddress),
        disconnect_event->mac, sizeof(disconnect_event->mac));
    staInfo.disconnectedReason = WIFI_REASON_UNSPECIFIED;
    SendOnHotspotStaLeave(&DevWifiInfo, &staInfo);
}

static void wifi_event_ap_start_proc(VOID *event_data)
{
    SendOnHotspotStateChanged(&DevWifiInfo, WIFI_STATE_AVAILABLE);
}

static void event_handler(VOID *arg, esp_event_base_t event_base,
                          int32_t event_id, VOID *event_data)
{
    LOG("event=%d", event_id);
    switch (event_id) {
        case WIFI_EVENT_SCAN_DONE:
            wifi_event_scan_down_proc(event_data);
            break;
        case WIFI_EVENT_STA_CONNECTED:
            wifi_event_sta_connected_proc(event_data);
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            wifi_event_sta_disconnected_proc(event_data);
            break;
        case WIFI_EVENT_AP_STACONNECTED:
            wifi_event_ap_connected_proc(event_data);
            break;
        case WIFI_EVENT_AP_STADISCONNECTED:
            wifi_event_ap_disconnected_proc(event_data);
            break;
        case WIFI_EVENT_AP_START:
            wifi_event_ap_start_proc(event_data);
            break;
        default:
            break;
    }
}

int DeviceWifiStart(void)
{
    esp_err_t err;
    DevWifiInfo_t *info = &DevWifiInfo;
    MEMCPY_S(info, sizeof(DevWifiInfo_t), 0, sizeof(*info));
    for (unsigned i = 0; i < WIFI_MAX_CONFIG_SIZE; i++) {
        info->config[i].netId = WIFI_CONFIG_INVALID;
    }

    err = esp_netif_init();
    if (err != ESP_OK) {
        LOGE("esp_netif_init.err=0x%X", err);
    }

    err = esp_event_loop_create_default();
    if (err != ESP_OK) {
        LOGE("esp_event_loop_create_default.err=0x%X", err);
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    cfg.nvs_enable = 0;
    err = esp_wifi_init(&cfg);
    if (err != ESP_OK) {
        LOGE("esp_wifi_init.err=0x%X", err);
    }

    esp_wifi_set_storage(WIFI_STORAGE_RAM);

    return 0;
}

static void UnregisterEspEvent(void)
{
    DevWifiInfo_t *info = &DevWifiInfo;
    WifiLock();
    DevWifiInfo.ip_ok = 0;
    DevWifiInfo.scan_ok = 0;
    if (info->eventHandle[0] != NULL) {
        esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, info->eventHandle[0]);
        info->eventHandle[0] = NULL;
    }
    if (info->eventHandle[1] != NULL) {
        esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, info->eventHandle[1]);
        info->eventHandle[1] = NULL;
    }
    if (info->netif != NULL) {
        esp_netif_destroy(info->netif);
        info->netif = NULL;
    }
    WifiUnlock();
}

static int RegisterEspEvent(int apMode)
{
    esp_err_t err;
    DevWifiInfo_t *info = &DevWifiInfo;
    WifiLock();
    if (apMode) {
        info->netif = esp_netif_create_default_wifi_ap();
    } else {
        info->netif = esp_netif_create_default_wifi_sta();
    }
    err = esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, event_handler,
                                              NULL, &info->eventHandle[0]);
    if (err != ESP_OK) {
        LOGE("WIFI_EVENT err=0x%X", err);
    }
    err = esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, event_got_ip_handler,
                                                                      NULL, &info->eventHandle[1]);
    if (err != ESP_OK) {
        LOGE("IP_EVENT err=0x%X", err);
    }
    err = esp_wifi_set_storage(WIFI_STORAGE_RAM);
    if (err != ESP_OK) {
        LOGE("set_storage err=0x%X", err);
    }
    err = esp_wifi_set_mode(apMode ? WIFI_MODE_AP : WIFI_MODE_STA);
    if (err != ESP_OK) {
        LOGE("set_mode(%d) err=0x%X", apMode, err);
    }
    WifiUnlock();
    return (info->netif == NULL) ? ERROR_NETIF_NULL : (err ? ERROR_REGISTER_FAIL : 0);
}

WifiErrorCode EnableWifi(void)
{
    DevWifiInfo_t *info = &DevWifiInfo;
    int err = 0;

    if (info->staStatus == WIFI_ACTIVE) {
        return ERROR_WIFI_BUSY;
    }
    UnregisterEspEvent();
    err = RegisterEspEvent(0);
    if (err == 0) {
        WifiLock();
        while (1) {
            if (esp_wifi_start() != ESP_OK) {
                err = ERROR_ESP_WIFI_START;
                break;
            }
            info->staStatus = WIFI_ACTIVE;
            break;
        }
        WifiUnlock();
    }
    if (err) {
        LOGE("err=%d", err);
        return ERROR_WIFI_IFACE_INVALID;
    }
    return WIFI_SUCCESS;
}

WifiErrorCode DisableWifi(void)
{
    DevWifiInfo_t *info = &DevWifiInfo;
    if (info->staStatus == WIFI_NOT_ACTIVE) {
        return ERROR_WIFI_NOT_STARTED;
    }
    WifiLock();
    info->staStatus = WIFI_NOT_ACTIVE;
    esp_wifi_disconnect();
    UnregisterEspEvent();
    esp_wifi_stop();
    WifiUnlock();
    return WIFI_SUCCESS;
}

WifiErrorCode Scan(void)
{
    wifi_scan_config_t config = {0};
    DevWifiInfo_t *info = &DevWifiInfo;
    WifiDeviceConfig *pconfig;
    if (info->staStatus != WIFI_ACTIVE) {
        return ERROR_WIFI_NOT_STARTED;
    }
    SendOnWifiScanStateChanged(info, WIFI_STATE_NOT_AVAILABLE, 0);

    esp_wifi_scan_stop();
    if (esp_wifi_scan_start(NULL, false) != ESP_OK) {
        return ERROR_WIFI_UNKNOWN;
    }
    info->scan_ok = SCANING;
    return WIFI_SUCCESS;
}

WifiErrorCode AddDeviceConfig(const WifiDeviceConfig *config, int *result)
{
    DevWifiInfo_t *info = &DevWifiInfo;
    if (!config)
        return ERROR_WIFI_INVALID_ARGS;
    WifiLock();
    for (unsigned i = 0; i < WIFI_MAX_CONFIG_SIZE; i++) {
        if (info->config[i].netId != i) {
            MEMCPY_S(&info->config[i], sizeof(WifiDeviceConfig), config, sizeof(WifiDeviceConfig));
            info->config[i].netId = i;
            WifiUnlock();
            if (result) {
                *result = i;
            }
            return WIFI_SUCCESS;
        }
    }
    WifiUnlock();
    if (result) {
        *result = 0;
    }
    return ERROR_WIFI_BUSY;
}

WifiErrorCode GetDeviceConfigs(WifiDeviceConfig *result, unsigned int *size)
{
    unsigned retIndex = 0, maxIndex;
    DevWifiInfo_t *info = &DevWifiInfo;
    if ((!result) || (!size))
        return ERROR_WIFI_INVALID_ARGS;
    if (!*size)
        return ERROR_WIFI_INVALID_ARGS;
    maxIndex = *size;

    WifiLock();
    for (unsigned i = 0; i < WIFI_MAX_CONFIG_SIZE; i++) {
        if (info->config[i].netId != i) {
            continue;
        }
        MEMCPY_S(&result[retIndex++], sizeof(WifiDeviceConfig), &info->config[i], sizeof(WifiDeviceConfig));
        if (maxIndex < retIndex) {
            break;
        }
    }
    WifiUnlock();

    if (retIndex == 0) {
        return ERROR_WIFI_NOT_AVAILABLE;
    }
    *size = retIndex;
    return WIFI_SUCCESS;
}

WifiErrorCode RemoveDevice(int networkId)
{
    DevWifiInfo_t *info = &DevWifiInfo;
    if ((networkId >= WIFI_MAX_CONFIG_SIZE) || (networkId < 0)) {
        return ERROR_WIFI_INVALID_ARGS;
    }

    WifiLock();
    MEMCPY_S(&info->config[networkId], sizeof(WifiDeviceConfig), 0, sizeof(WifiDeviceConfig));
    info->config[networkId].netId = WIFI_CONFIG_INVALID;
    WifiUnlock();
    return WIFI_SUCCESS;
}

WifiErrorCode GetDeviceMacAddress(unsigned char *result)
{
    if (!result) {
        return ERROR_WIFI_INVALID_ARGS;
    }

    if (esp_wifi_get_mac(WIFI_IF_STA, (uint8_t *)result) != ESP_OK) {
        return ERROR_WIFI_UNKNOWN;
    }

    return WIFI_SUCCESS;
}
static WifiSecurityType ESPSecToHoSec(wifi_auth_mode_t mode)
{
    switch (mode) {
        case WIFI_AUTH_OPEN:
            return WIFI_SEC_TYPE_OPEN;
        case WIFI_AUTH_WEP:
            return WIFI_SEC_TYPE_WEP;
        case WIFI_AUTH_WPA_PSK:
        case WIFI_AUTH_WPA2_PSK:
        case WIFI_AUTH_WPA_WPA2_PSK:
            return WIFI_SEC_TYPE_PSK;
        case WIFI_AUTH_WPA3_PSK:
        case WIFI_AUTH_WPA2_WPA3_PSK:
            return WIFI_SEC_TYPE_SAE;
        default:
            return WIFI_SEC_TYPE_INVALID;
    }
}

static wifi_auth_mode_t HoSecToESPSec(WifiSecurityType type)
{
    switch (type) {
        case WIFI_SEC_TYPE_OPEN:
            return WIFI_SEC_TYPE_OPEN;
        case WIFI_SEC_TYPE_WEP:
            return WIFI_SEC_TYPE_WEP;
        case WIFI_SEC_TYPE_PSK:
            return WIFI_AUTH_WPA_PSK;
        case WIFI_SEC_TYPE_SAE:
            return WIFI_AUTH_WPA3_PSK;
        default:
            return WIFI_AUTH_MAX;
    }
}

static int GetScanInfoListNext(WifiScanInfo *result, uint16_t ap_count, unsigned int *size)
{
    DevWifiInfo_t *info = &DevWifiInfo;
    wifi_ap_record_t *ap_info;
    unsigned int maxi = *size;
    info->scan_ok = 0x51;
    esp_wifi_scan_get_ap_num(&ap_count);
    if (maxi > ap_count) {
        *size = maxi = ap_count;
    }
    if (maxi == 0) {
        return WIFI_SUCCESS;
    }
    ap_info = (wifi_ap_record_t *)LOS_MemAlloc(OS_SYS_MEM_ADDR, sizeof(wifi_ap_record_t) * maxi);
    if (!ap_info) {
        return ERROR_WIFI_UNKNOWN;
    }
    for (int i = 0; i < maxi; ++i) {
        MEMCPY_S(result[i].ssid, sizeof(result[i].ssid), ap_info[i].ssid, sizeof(ap_info[i].ssid));
        MEMCPY_S(result[i].bssid, sizeof(result[i].bssid), ap_info[i].bssid, sizeof(ap_info[i].bssid));
        result[i].securityType = ESPSecToHoSec(ap_info[i].authmode);
        result[i].rssi = ap_info[i].rssi;
        result[i].band = 0;
        result[i].frequency = ChannelToFrequency(ap_info[i].primary);
    }
    LOS_MemFree(OS_SYS_MEM_ADDR, ap_info);
    return WIFI_SUCCESS;
}

WifiErrorCode GetScanInfoList(WifiScanInfo *result, unsigned int *size)
{
    DevWifiInfo_t *info = &DevWifiInfo;
    uint16_t ap_count = 0;
    unsigned int i, maxi;
    if ((!result) || (!size) || (*size == 0)) {
        return ERROR_WIFI_INVALID_ARGS;
    }
    if (info->staStatus != WIFI_ACTIVE) {
        return ERROR_WIFI_NOT_STARTED;
    }
    if (info->scan_ok == SCANING) {
        maxi = MAX_INDEX;
        for (i = 0; (i < maxi) && (info->scan_ok == SCANING); ++i) {
            LOS_Msleep(DELAY_50_TICK);
        }
        if (i >= maxi) {
            return ERROR_WIFI_BUSY;
        }
    }
    if (info->scan_ok != 1) {
        wifi_ap_record_t t_info[1];
        ap_count = 1;
        esp_wifi_scan_get_ap_records(&ap_count, t_info);
        *size = 0;
        return WIFI_SUCCESS;
    }
    return GetScanInfoListNext(result, ap_count, size);
}

WifiErrorCode ConnectTo(int networkId)
{
    WifiDeviceConfig *pconfig;
    DevWifiInfo_t *info = &DevWifiInfo;
    if ((networkId >= WIFI_MAX_CONFIG_SIZE) || (networkId < 0)) {
        return ERROR_WIFI_INVALID_ARGS;
    }
    if (info->staStatus != WIFI_ACTIVE)
        return ERROR_WIFI_NOT_AVAILABLE;

    WifiLock();
    pconfig = &info->config[networkId];
    if (pconfig->netId != networkId) {
        WifiUnlock();
        return ERROR_WIFI_NOT_AVAILABLE;
    }

    info->ip_ok = 0;
    WifiUnlock();

    wifi_config_t assocReq = {0};
    assocReq.sta.threshold.authmode = HoSecToESPSec(pconfig->securityType);
    MEMCPY_S(assocReq.sta.ssid, sizeof(assocReq.sta.ssid), pconfig->ssid, sizeof(pconfig->ssid));
    MEMCPY_S(assocReq.sta.password, sizeof(assocReq.sta.password),
            pconfig->preSharedKey, sizeof(pconfig->preSharedKey));
    MEMCPY_S(assocReq.sta.bssid, sizeof(assocReq.sta.bssid), pconfig->bssid, sizeof(pconfig->bssid));
    assocReq.sta.channel = FrequencyToChannel(pconfig->freq);
    assocReq.sta.pmf_cfg.capable = true;
    assocReq.sta.pmf_cfg.required = false;
    esp_wifi_set_config(WIFI_IF_STA, &assocReq);
    if (esp_wifi_connect() != ESP_OK) {
        return ERROR_WIFI_UNKNOWN;
    }
    for (int i = 0; i < DELAY_LOOP_TIMES; ++i) {
        if (info->ip_ok == 1) {
            break;
        }
        LOS_Msleep(DELAY_50_TICK);
    }
    return WIFI_SUCCESS;
}

WifiErrorCode Disconnect(void)
{
    if (esp_wifi_disconnect() != ESP_OK) {
        return ERROR_WIFI_UNKNOWN;
    }
    return WIFI_SUCCESS;
}

WifiErrorCode GetLinkedInfo(WifiLinkedInfo *result)
{
    DevWifiInfo_t *info = &DevWifiInfo;
    esp_err_t err;
    if (!result) {
        return ERROR_WIFI_INVALID_ARGS;
    }
    MEMCPY_S(result, sizeof(WifiLinkedInfo), 0, sizeof(WifiLinkedInfo));
    if (info->staStatus != WIFI_ACTIVE)
        return ERROR_WIFI_NOT_STARTED;

    wifi_ap_record_t ap_info = {0};
    if (esp_wifi_sta_get_ap_info(&ap_info) != ESP_OK) {
        result->connState = WIFI_DISCONNECTED;
        return ERROR_WIFI_UNKNOWN;
    }

    MEMCPY_S(result->ssid, sizeof(result->ssid), ap_info.ssid, sizeof(ap_info.ssid));
    MEMCPY_S(result->bssid, sizeof(result->bssid), ap_info.bssid, sizeof(ap_info.bssid));
    result->connState = WIFI_CONNECTED;
    result->rssi = ap_info.rssi;

    esp_netif_ip_info_t ip_info;
    err = esp_netif_get_ip_info(info->netif, &ip_info);
    if (err != ESP_OK) {
        LOGE("esp_netif_get_ip_info.err = %d", err);
        return ERROR_WIFI_UNKNOWN;
    }

    result->ipAddress = ip_info.ip.addr;
    return WIFI_SUCCESS;
}

int IsHotspotActive(void)
{
    return ((DevWifiInfo.staStatus == WIFI_ACTIVE) ? WIFI_HOTSPOT_ACTIVE : WIFI_HOTSPOT_NOT_ACTIVE);
}

WifiErrorCode DisableHotspot(void)
{
    return DisableWifi();
}

WifiErrorCode GetStationList(StationInfo *result, unsigned int *size)
{
    if ((!result) || (!size))
        return ERROR_WIFI_INVALID_ARGS;
    if (!*size)
        return ERROR_WIFI_INVALID_ARGS;

    wifi_sta_list_t wifi_sta_list = {0};
    unsigned int staNum = 0;
    esp_err_t ret = esp_wifi_ap_get_sta_list(&wifi_sta_list);
    if (ret != ESP_OK) {
        LOGE("esp_wifi_ap_get_sta_list.err=0x%X", ret);
        return ERROR_WIFI_NOT_AVAILABLE;
    }
    staNum = wifi_sta_list.num;
    if (*size < staNum) {
        staNum = *size;
    }
    MEMCPY_S(result, (sizeof(result[0]) * staNum), 0, sizeof(result[0]) * staNum);
    for (unsigned int i = 0; i < staNum; i++) {
        MEMCPY_S(result[i].macAddress, sizeof(result[i].macAddress),
                 wifi_sta_list.sta[i].mac, sizeof(wifi_sta_list.sta[i].mac));
    }
    return WIFI_SUCCESS;
}

WifiErrorCode SetBand(int band)
{
    DevWifiInfo_t *info = &DevWifiInfo;
    if (band != HOTSPOT_BAND_TYPE_2G) {
        return ERROR_WIFI_NOT_SUPPORTED;
    }
    info->hotConfig[0].band = band;
    return WIFI_SUCCESS;
}

WifiErrorCode GetBand(int *result)
{
    DevWifiInfo_t *info = &DevWifiInfo;
    if (result == NULL) {
        return ERROR_WIFI_INVALID_ARGS;
    }
    if (info->hotConfig[0].band == 0) {
        return ERROR_WIFI_NOT_AVAILABLE;
    }
    *result = (int)info->hotConfig[0].band;
    return WIFI_SUCCESS;
}

WifiErrorCode SetHotspotConfig(const HotspotConfig *config)
{
    DevWifiInfo_t *info = &DevWifiInfo;
    if (!config) {
        return ERROR_WIFI_INVALID_ARGS;
    }
    WifiLock();
    MEMCPY_S(&info->hotConfig[0], sizeof(HotspotConfig), config, sizeof(HotspotConfig));
    WifiUnlock();
    return WIFI_SUCCESS;
}

WifiErrorCode GetHotspotConfig(HotspotConfig *result)
{
    DevWifiInfo_t *info = &DevWifiInfo;
    if (result == NULL) {
        return ERROR_WIFI_INVALID_ARGS;
    }
    WifiLock();
    MEMCPY_S(result, sizeof(HotspotConfig), &info->hotConfig[0], sizeof(HotspotConfig));
    WifiUnlock();
    return WIFI_SUCCESS;
}

int GetSignalLevel(int rssi, int band)
{
    if (band == HOTSPOT_BAND_TYPE_2G) {
        if (rssi >= RSSI_LEVEL_4_2_G) {
            return RSSI_LEVEL_4;
        }
        if (rssi >= RSSI_LEVEL_3_2_G) {
            return RSSI_LEVEL_3;
        }
        if (rssi >= RSSI_LEVEL_2_2_G) {
            return RSSI_LEVEL_2;
        }
        if (rssi >= RSSI_LEVEL_1_2_G) {
            return RSSI_LEVEL_1;
        }
    }

    if (band == HOTSPOT_BAND_TYPE_5G) {
        if (rssi >= RSSI_LEVEL_4_5_G) {
            return RSSI_LEVEL_4;
        }
        if (rssi >= RSSI_LEVEL_3_5_G) {
            return RSSI_LEVEL_3;
        }
        if (rssi >= RSSI_LEVEL_2_5_G) {
            return RSSI_LEVEL_2;
        }
        if (rssi >= RSSI_LEVEL_1_5_G) {
            return RSSI_LEVEL_1;
        }
    }
    return ERROR_WIFI_INVALID_ARGS;
}

WifiErrorCode EnableHotspot(void)
{
    int err;
    DevWifiInfo_t *info = &DevWifiInfo;
    if (info->apStatus == WIFI_ACTIVE) {
        return ERROR_WIFI_BUSY;
    }

    UnregisterEspEvent();
    err = RegisterEspEvent(1);
    if (err) {
        LOGE("esp_wifi_start.err=0x%X", err);
        return ERROR_WIFI_INVALID_ARGS;
    }

    WifiLock();
    wifi_config_t wifi_config = {0};
    HotspotConfig *hotConfig = &info->hotConfig[0];
    wifi_config.ap.channel = HOTSPOT_DEFAULT_CHANNEL;
    if (hotConfig->channelNum) {
        wifi_config.ap.channel = hotConfig->channelNum;
    }
    wifi_config.ap.authmode = HoSecToESPSec(hotConfig->securityType);
    wifi_config.ap.max_connection = ESP_EXAMPLE_MAX_STA_CONN;
    MEMCPY_S(wifi_config.ap.ssid, sizeof(wifi_config.ap.ssid), hotConfig->ssid, sizeof(hotConfig->ssid));
    MEMCPY_S(wifi_config.ap.password, sizeof(wifi_config.ap.password),
            hotConfig->preSharedKey, sizeof(hotConfig->preSharedKey));
    wifi_config.ap.ssid_len = (uint8_t)strlen((const char *)wifi_config.ap.ssid);
    if ((uint8_t)strlen((const char *)wifi_config.ap.password) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    info->ip_ok = 0;

    esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    esp_err_t ret = esp_wifi_start();
    if (ret != ESP_OK) {
        LOGE("esp_wifi_start.err=0x%X", ret);
        WifiUnlock();
        return ERROR_WIFI_BUSY;
    }

    info->staStatus = WIFI_ACTIVE;
    WifiUnlock();
    LOS_Msleep(DELAY_10_TICK);
    for (int i = 0; i < DELAY_LOOP_TIMES; ++i) {
        if (info->ip_ok == 1) {
            break;
        }
        LOS_Msleep(DELAY_30_TICK);
    }
    return WIFI_SUCCESS;
}

WifiErrorCode AdvanceScan(WifiScanParams *params)
{
    DevWifiInfo_t *info = &DevWifiInfo;
    if (params == NULL) {
        return ERROR_WIFI_UNKNOWN;
    }

    if (info->staStatus != WIFI_ACTIVE) {
        return ERROR_WIFI_NOT_STARTED;
    }
    SendOnWifiScanStateChanged(info, WIFI_STATE_NOT_AVAILABLE, 0);

    wifi_scan_config_t config = {0};
    if (params->scanType == WIFI_FREQ_SCAN) {
        if (params->freqs <= 0) {
            return ERROR_WIFI_UNKNOWN;
        }
        config.channel = FrequencyToChannel(params->freqs);
    } else if (params->scanType == WIFI_SSID_SCAN) {
        if (params->ssidLen == 0) {
            return ERROR_WIFI_UNKNOWN;
        }
        config.ssid = (uint8_t *)params->ssid;
    } else if (params->scanType == WIFI_BSSID_SCAN) {
        if (!memcmp(NullBssid, params->bssid, WIFI_MAC_LEN)) {
            return ERROR_WIFI_UNKNOWN;
        }
        config.bssid = (uint8_t *)params->bssid;
    }
    config.show_hidden = false;
    config.scan_type = WIFI_SCAN_TYPE_ACTIVE;
    config.scan_time.active.min = 0;
    config.scan_time.active.max = 0;
    config.scan_time.passive = 0;
    esp_wifi_scan_stop();
    if (esp_wifi_scan_start(&config, false) != ESP_OK) {
        return ERROR_WIFI_BUSY;
    }
    info->scan_ok = SCANING;
    return WIFI_SUCCESS;
}

int GetHotspotChannel(void)
{
    DevWifiInfo_t *info = &DevWifiInfo;
    if (info->apStatus != WIFI_ACTIVE) {
        return ERROR_WIFI_NOT_STARTED;
    }
    wifi_config_t wifi_cfg = {0};
    esp_err_t ret = esp_wifi_get_config(WIFI_IF_AP, &wifi_cfg);
    if (ret != ESP_OK) {
        return ERROR_WIFI_UNKNOWN;
    }
    return wifi_cfg.ap.channel;
}

WifiErrorCode GetHotspotInterfaceName(char *result, int size)
{
    DevWifiInfo_t *info = &DevWifiInfo;
    if (info->apStatus != WIFI_ACTIVE) {
        return ERROR_WIFI_NOT_STARTED;
    }
    MEMCPY_S(result, size, "esp", EPS_STR_LEN);
    return WIFI_SUCCESS;
}
