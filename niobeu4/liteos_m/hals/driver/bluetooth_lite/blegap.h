/*
 * Copyright (c) 2022 Hunan OpenValley Digital Industry Development Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef __BLEGAPKITS_H__
#define __BLEGAPKITS_H__
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"
#include "esp_log.h"
#include "ohos_bt_def.h"

#define GattInterfaceType esp_gatt_if_t
#define BleGapParam esp_ble_gap_cb_param_t
#define BleGattcParam esp_ble_gattc_cb_param_t
#define BleScanParams esp_ble_scan_params_t
#define DataLength 4
#define BT_DEBUG printf

#define OHOS_GATT_PREP_WRITE_CANCEL 0x00         //准备写标志，表示取消准备写
#define OHOS_GATT_PREP_WRITE_EXEC   0x01         //准备写标志，表示正在写
#define OHOS_GATT_UUID_CHAR_CLIENT_CONFIG 0x2902 //客户端特性配置
#define OHOS_GATT_UUID_CHAR_SRVR_CONFIG 0x2903   //服务端特性配置
#define OHOS_GATT_IF_NONE    0xff                //此事件不对应于任何应用程序
#define GattInterfaceType esp_gatt_if_t          //Gatt接口类型，不同的应用程序在Gatt客户端使用不同的gatt_if
#define BtUuids esp_bt_uuid_t                    //uuid
#define BleConnUpdateParams esp_ble_conn_update_params_t //更新连接参数
#define BT_LOGE(format, ...) printf("[%s] " format "\r\n", __FUNCTION__, ##__VA_ARGS__)  //bt log

/**@{
 * 数据标志位定义，用于广播数据标志
 */
#define OHOS_BLE_ADV_FLAG_LIMIT_DISC         (0x01 << 0)
#define OHOS_BLE_ADV_FLAG_GEN_DISC           (0x01 << 1)
#define OHOS_BLE_ADV_FLAG_BREDR_NOT_SPT      (0x01 << 2)
#define OHOS_BLE_ADV_FLAG_DMT_CONTROLLER_SPT (0x01 << 3)
#define OHOS_BLE_ADV_FLAG_DMT_HOST_SPT       (0x01 << 4)
#define OHOS_BLE_ADV_FLAG_NON_LIMIT_DISC     (0x00 )

/* 特征属性的定义 */
#define    OHOS_GATT_CHAR_PROP_BIT_BROADCAST    (1 << 0)       /* 0x01 */    /* relate to BTA_GATT_CHAR_PROP_BIT_BROADCAST in bta/bta_gatt_api.h */
#define    OHOS_GATT_CHAR_PROP_BIT_READ         (1 << 1)       /* 0x02 */    /* relate to BTA_GATT_CHAR_PROP_BIT_READ in bta/bta_gatt_api.h */
#define    OHOS_GATT_CHAR_PROP_BIT_WRITE_NR     (1 << 2)       /* 0x04 */    /* relate to BTA_GATT_CHAR_PROP_BIT_WRITE_NR in bta/bta_gatt_api.h */
#define    OHOS_GATT_CHAR_PROP_BIT_WRITE        (1 << 3)       /* 0x08 */    /* relate to BTA_GATT_CHAR_PROP_BIT_WRITE in bta/bta_gatt_api.h */
#define    OHOS_GATT_CHAR_PROP_BIT_NOTIFY       (1 << 4)       /* 0x10 */    /* relate to BTA_GATT_CHAR_PROP_BIT_NOTIFY in bta/bta_gatt_api.h */
#define    OHOS_GATT_CHAR_PROP_BIT_INDICATE     (1 << 5)       /* 0x20 */    /* relate to BTA_GATT_CHAR_PROP_BIT_INDICATE in bta/bta_gatt_api.h */
#define    OHOS_GATT_CHAR_PROP_BIT_AUTH         (1 << 6)       /* 0x40 */    /* relate to BTA_GATT_CHAR_PROP_BIT_AUTH in bta/bta_gatt_api.h */
#define    OHOS_GATT_CHAR_PROP_BIT_EXT_PROP     (1 << 7)       /* 0x80 */    /* relate to BTA_GATT_CHAR_PROP_BIT_EXT_PROP in bta/bta_gatt_api.h */
typedef uint8_t BdAddrs[OHOS_BD_ADDR_LEN];
//蓝牙返回状态码
typedef enum {
    BT_ERROR = -1,
    BT_SUCCESS = 0, 
    BT_USERAUTHID_ERROR = 1,
    BT_DEVICETYPE_ERROR = 2,
    BT_PARAMINPUT_ERROR = 3,
} BtError;

//蓝牙传输通道枚举
typedef enum {
    OHOS_BT_TRANSPORT_TYPE_AUTO = 0x0,
    OHOS_BT_TRANSPORT_TYPE_LE,
    OHOS_BT_TRANSPORT_TYPE_CLASSIC,
} BtTransportType;

/// GAP BLE回调事件
typedef enum {
    OHOS_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT        = 0,       /*!< When advertising data set complete, the event comes */
    OHOS_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT,             /*!< When scan response data set complete, the event comes */
    OHOS_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT,                /*!< When scan parameters set complete, the event comes */
    OHOS_GAP_BLE_SCAN_RESULT_EVT,                            /*!< When one scan result ready, the event comes each time */
    OHOS_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT,              /*!< When raw advertising data set complete, the event comes */
    OHOS_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT,         /*!< When raw advertising data set complete, the event comes */
    OHOS_GAP_BLE_ADV_START_COMPLETE_EVT,                     /*!< When start advertising complete, the event comes */
    OHOS_GAP_BLE_SCAN_START_COMPLETE_EVT,                    /*!< When start scan complete, the event comes */
    OHOS_GAP_BLE_AUTH_CMPL_EVT = 8,                          /* Authentication complete indication. */
    OHOS_GAP_BLE_KEY_EVT,                                    /* BLE  key event for peer device keys */
    OHOS_GAP_BLE_SEC_REQ_EVT,                                /* BLE  security request */
    OHOS_GAP_BLE_PASSKEY_NOTIF_EVT,                          /* passkey notification event */
    OHOS_GAP_BLE_PASSKEY_REQ_EVT,                            /* passkey request event */
    OHOS_GAP_BLE_OOB_REQ_EVT,                                /* OOB request event */
    OHOS_GAP_BLE_LOCAL_IR_EVT,                               /* BLE local IR event */
    OHOS_GAP_BLE_LOCAL_ER_EVT,                               /* BLE local ER event */
    OHOS_GAP_BLE_NC_REQ_EVT,                                 /* Numeric Comparison request event */
    OHOS_GAP_BLE_ADV_STOP_COMPLETE_EVT,                      /*!< When stop adv complete, the event comes */
    OHOS_GAP_BLE_SCAN_STOP_COMPLETE_EVT,                     /*!< When stop scan complete, the event comes */
    OHOS_GAP_BLE_SET_STATIC_RAND_ADDR_EVT = 19,              /*!< When set the static rand address complete, the event comes */
    OHOS_GAP_BLE_UPDATE_CONN_PARAMS_EVT,                     /*!< When update connection parameters complete, the event comes */
    OHOS_GAP_BLE_SET_PKT_LENGTH_COMPLETE_EVT,                /*!< When set pkt length complete, the event comes */
    OHOS_GAP_BLE_SET_LOCAL_PRIVACY_COMPLETE_EVT,             /*!< When  Enable/disable privacy on the local device complete, the event comes */
    OHOS_GAP_BLE_REMOVE_BOND_DEV_COMPLETE_EVT,               /*!< When remove the bond device complete, the event comes */
    OHOS_GAP_BLE_CLEAR_BOND_DEV_COMPLETE_EVT,                /*!< When clear the bond device clear complete, the event comes */
    OHOS_GAP_BLE_GET_BOND_DEV_COMPLETE_EVT,                  /*!< When get the bond device list complete, the event comes */
    OHOS_GAP_BLE_READ_RSSI_COMPLETE_EVT,                     /*!< When read the rssi complete, the event comes */
    OHOS_GAP_BLE_UPDATE_WHITELIST_COMPLETE_EVT,              /*!< When add or remove whitelist complete, the event comes */
    OHOS_GAP_BLE_UPDATE_DUPLICATE_EXCEPTIONAL_LIST_COMPLETE_EVT,  /*!< When update duplicate exceptional list complete, the event comes */
    OHOS_GAP_BLE_SET_CHANNELS_EVT = 29,                           /*!< When setting BLE channels complete, the event comes */
    OHOS_GAP_BLE_EVT_MAX,
} GapBleCallbackEvent;

// OHOS_GAP_BLE_SCAN_RESULT_EVT 子事件
typedef enum {
    OHOS_GAP_SEARCH_INQ_RES_EVT             = 0,      /*!< Inquiry result for a peer device. */
    OHOS_GAP_SEARCH_INQ_CMPL_EVT            = 1,      /*!< Inquiry complete. */
    OHOS_GAP_SEARCH_DISC_RES_EVT            = 2,      /*!< Discovery result for a peer device. */
    OHOS_GAP_SEARCH_DISC_BLE_RES_EVT        = 3,      /*!< Discovery result for BLE GATT based service on a peer device. */
    OHOS_GAP_SEARCH_DISC_CMPL_EVT           = 4,      /*!< Discovery complete. */
    OHOS_GAP_SEARCH_DI_DISC_CMPL_EVT        = 5,      /*!< Discovery complete. */
    OHOS_GAP_SEARCH_SEARCH_CANCEL_CMPL_EVT  = 6,      /*!< Search cancelled */
    OHOS_GAP_SEARCH_INQ_DISCARD_NUM_EVT     = 7,      /*!< The number of pkt discarded by flow control */
} GapSearchEvent;

/// The type of advertising data(not adv_type)
typedef enum {
    OHOS_BLE_AD_TYPE_FLAG                     = 0x01,    /* relate to BTM_BLE_AD_TYPE_FLAG in stack/btm_ble_api.h */
    OHOS_BLE_AD_TYPE_16SRV_PART               = 0x02,    /* relate to BTM_BLE_AD_TYPE_16SRV_PART in stack/btm_ble_api.h */
    OHOS_BLE_AD_TYPE_16SRV_CMPL               = 0x03,    /* relate to BTM_BLE_AD_TYPE_16SRV_CMPL in stack/btm_ble_api.h */
    OHOS_BLE_AD_TYPE_32SRV_PART               = 0x04,    /* relate to BTM_BLE_AD_TYPE_32SRV_PART in stack/btm_ble_api.h */
    OHOS_BLE_AD_TYPE_32SRV_CMPL               = 0x05,    /* relate to BTM_BLE_AD_TYPE_32SRV_CMPL in stack/btm_ble_api.h */
    OHOS_BLE_AD_TYPE_128SRV_PART              = 0x06,    /* relate to BTM_BLE_AD_TYPE_128SRV_PART in stack/btm_ble_api.h */
    OHOS_BLE_AD_TYPE_128SRV_CMPL              = 0x07,    /* relate to BTM_BLE_AD_TYPE_128SRV_CMPL in stack/btm_ble_api.h */
    OHOS_BLE_AD_TYPE_NAME_SHORT               = 0x08,    /* relate to BTM_BLE_AD_TYPE_NAME_SHORT in stack/btm_ble_api.h */
    OHOS_BLE_AD_TYPE_NAME_CMPL                = 0x09,    /* relate to BTM_BLE_AD_TYPE_NAME_CMPL in stack/btm_ble_api.h */
    OHOS_BLE_AD_TYPE_TX_PWR                   = 0x0A,    /* relate to BTM_BLE_AD_TYPE_TX_PWR in stack/btm_ble_api.h */
    OHOS_BLE_AD_TYPE_DEV_CLASS                = 0x0D,    /* relate to BTM_BLE_AD_TYPE_DEV_CLASS in stack/btm_ble_api.h */
    OHOS_BLE_AD_TYPE_SM_TK                    = 0x10,    /* relate to BTM_BLE_AD_TYPE_SM_TK in stack/btm_ble_api.h */
    OHOS_BLE_AD_TYPE_SM_OOB_FLAG              = 0x11,    /* relate to BTM_BLE_AD_TYPE_SM_OOB_FLAG in stack/btm_ble_api.h */
    OHOS_BLE_AD_TYPE_INT_RANGE                = 0x12,    /* relate to BTM_BLE_AD_TYPE_INT_RANGE in stack/btm_ble_api.h */
    OHOS_BLE_AD_TYPE_SOL_SRV_UUID             = 0x14,    /* relate to BTM_BLE_AD_TYPE_SOL_SRV_UUID in stack/btm_ble_api.h */
    OHOS_BLE_AD_TYPE_128SOL_SRV_UUID          = 0x15,    /* relate to BTM_BLE_AD_TYPE_128SOL_SRV_UUID in stack/btm_ble_api.h */
    OHOS_BLE_AD_TYPE_SERVICE_DATA             = 0x16,    /* relate to BTM_BLE_AD_TYPE_SERVICE_DATA in stack/btm_ble_api.h */
    OHOS_BLE_AD_TYPE_PUBLIC_TARGET            = 0x17,    /* relate to BTM_BLE_AD_TYPE_PUBLIC_TARGET in stack/btm_ble_api.h */
    OHOS_BLE_AD_TYPE_RANDOM_TARGET            = 0x18,    /* relate to BTM_BLE_AD_TYPE_RANDOM_TARGET in stack/btm_ble_api.h */
    OHOS_BLE_AD_TYPE_APPEARANCE               = 0x19,    /* relate to BTM_BLE_AD_TYPE_APPEARANCE in stack/btm_ble_api.h */
    OHOS_BLE_AD_TYPE_ADV_INT                  = 0x1A,    /* relate to BTM_BLE_AD_TYPE_ADV_INT in stack/btm_ble_api.h */
    OHOS_BLE_AD_TYPE_LE_DEV_ADDR              = 0x1b,    /* relate to BTM_BLE_AD_TYPE_LE_DEV_ADDR in stack/btm_ble_api.h */
    OHOS_BLE_AD_TYPE_LE_ROLE                  = 0x1c,    /* relate to BTM_BLE_AD_TYPE_LE_ROLE in stack/btm_ble_api.h */
    OHOS_BLE_AD_TYPE_SPAIR_C256               = 0x1d,    /* relate to BTM_BLE_AD_TYPE_SPAIR_C256 in stack/btm_ble_api.h */
    OHOS_BLE_AD_TYPE_SPAIR_R256               = 0x1e,    /* relate to BTM_BLE_AD_TYPE_SPAIR_R256 in stack/btm_ble_api.h */
    OHOS_BLE_AD_TYPE_32SOL_SRV_UUID           = 0x1f,    /* relate to BTM_BLE_AD_TYPE_32SOL_SRV_UUID in stack/btm_ble_api.h */
    OHOS_BLE_AD_TYPE_32SERVICE_DATA           = 0x20,    /* relate to BTM_BLE_AD_TYPE_32SERVICE_DATA in stack/btm_ble_api.h */
    OHOS_BLE_AD_TYPE_128SERVICE_DATA          = 0x21,    /* relate to BTM_BLE_AD_TYPE_128SERVICE_DATA in stack/btm_ble_api.h */
    OHOS_BLE_AD_TYPE_LE_SECURE_CONFIRM        = 0x22,    /* relate to BTM_BLE_AD_TYPE_LE_SECURE_CONFIRM in stack/btm_ble_api.h */
    OHOS_BLE_AD_TYPE_LE_SECURE_RANDOM         = 0x23,    /* relate to BTM_BLE_AD_TYPE_LE_SECURE_RANDOM in stack/btm_ble_api.h */
    OHOS_BLE_AD_TYPE_URI                      = 0x24,    /* relate to BTM_BLE_AD_TYPE_URI in stack/btm_ble_api.h */
    OHOS_BLE_AD_TYPE_INDOOR_POSITION          = 0x25,    /* relate to BTM_BLE_AD_TYPE_INDOOR_POSITION in stack/btm_ble_api.h */
    OHOS_BLE_AD_TYPE_TRANS_DISC_DATA          = 0x26,    /* relate to BTM_BLE_AD_TYPE_TRANS_DISC_DATA in stack/btm_ble_api.h */
    OHOS_BLE_AD_TYPE_LE_SUPPORT_FEATURE       = 0x27,    /* relate to BTM_BLE_AD_TYPE_LE_SUPPORT_FEATURE in stack/btm_ble_api.h */
    OHOS_BLE_AD_TYPE_CHAN_MAP_UPDATE          = 0x28,    /* relate to BTM_BLE_AD_TYPE_CHAN_MAP_UPDATE in stack/btm_ble_api.h */
    OHOS_BLE_AD_MANUFACTURER_SPECIFIC_TYPE    = 0xFF,    /* relate to BTM_BLE_AD_MANUFACTURER_SPECIFIC_TYPE in stack/btm_ble_api.h */
} BleAdvDataType;

/**
  * @brief Gatt写类型
  */
typedef enum {
    OHOS_GATT_WRITE_TYPE_NO_RSP  =   1,                      /*!< Gatt write attribute need no response */
    OHOS_GATT_WRITE_TYPE_RSP,                                /*!< Gatt write attribute need remote response */
} GattBleWriteType;

/**
 * @brief GAP回调函数类型
 * @param event : 事件类型
 * @param param : 指向回调参数的指针，当前为联合类型
 */
typedef void (* GapBleCallback)(GapBleCallbackEvent event, BleGapParam *param);

/**
 * @brief 启动Ble函数
 * @return BtResult 成功返回 BT_SUCCESS
 */
BtError EnableBle(void);

/**
 * @brief 关闭ble函数
 * 
 * @return BtResult 成功返回 BT_SUCCESS
 */
BtError DisableBle(void);

/**
 * @brief 设置本机蓝牙名称函数
 * 
 * @param localName 设备名称
 * @param length 名称长度
 * @return BtResult 成功返回 BT_SUCCESS
 */
BtError SetLocalName(unsigned char *localName, unsigned char length);

/**
 * @brief 获取本机蓝牙地址
 * 
 * @param mac 蓝牙mac地址指针
 * @param length 蓝牙mac地址长度
 * @return BtResult 成功返回 BT_SUCCESS
 */
BtError GetLocalAddr(unsigned char *mac, unsigned int len);

/**
 * @brief 启动蓝牙Ble扫描
 * 
 * @return BtResult 成功返回 BT_SUCCESS
 */
BtError BleStartScan(void);

/**
 * @brief 停止蓝牙扫描
 * 
 * @return BtResult 成功返回 BT_SUCCESS
 */
BtError BleStopScan(void);

/**
 * @brief 更新连接参数
 *
 * @param connection update parameters
 *
 * @return BtResult 成功返回 BT_SUCCESS
 */
BtError BleGapUpdateConnParams(BleConnUpdateParams *params);

/**
 * @brief 数据加密
 * 
 * @param data 数据包
 * @param dataLen 数据包长度
 * @param addr 蓝牙mac地址
 * @param addrLen 蓝牙mac地址长度
 * @return 数据长度
 */
unsigned char BtEncrypt(unsigned char data[], unsigned char dataLen, unsigned char addr[], unsigned char addrLen);

/**
 * @brief 数据解密
 * 
 * @param data 数据包
 * @param dataLen 数据包长度
 * @param addr 蓝牙mac地址
 * @param addrLen 蓝牙mac地址长度
 * @return 数据长度
 */
unsigned char BtDecrypt(unsigned char data[], unsigned char dataLen, unsigned char addr[], unsigned char addrLen);

/**
 * @brief 设置扫描参数函数
 *
 * @param scan_params: Pointer to User defined scan_params data structure. This
 *                  memory space can not be freed until callback of set_scan_params
 *
 * @return BtResult 成功返回 BT_SUCCESS
 *
 */
BtError BleGatSetScanParams(BleScanParams *scan_params);

/**
* @brief 断开对端设备的物理连接
*
* @param remote_device : BD address of the peer device
*
 * @return BtResult 成功返回 BT_SUCCESS
*/
BtError BleGapDisconnect(BdAddrs remote_device);

/**
 * @brief 获取特定类型的ADV数据
 *
 * @param adv_data - pointer of ADV data which to be resolved
 * @param type   - finding ADV data type
 * @param length - return the length of ADV data not including type
 *
 * @return BtResult 成功返回ADV data指针
 *          
 */
uint8_t *BleResolveAdvData(uint8_t *adv_data, uint8_t type, uint8_t *length);
#endif
