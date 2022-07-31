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
#ifndef __BLEGAP_H__
#define __BLEGAP_H__
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
#define BT_DEBUG printf

#define OHOS_GATT_PREP_WRITE_CANCEL 0x00
#define OHOS_GATT_PREP_WRITE_EXEC   0x01
#define OHOS_GATT_UUID_CHAR_CLIENT_CONFIG 0x2902
#define OHOS_GATT_UUID_CHAR_SRVR_CONFIG 0x2903
#define OHOS_GATT_IF_NONE    0xff
#define GattInterfaceType esp_gatt_if_t
#define BtUuids esp_bt_uuid_t
#define BleConnUpdateParams esp_ble_conn_update_params_t
#define BT_LOGE(format, ...) printf("[%s] " format "\r\n", __FUNCTION__, ##__VA_ARGS__)

#define OHOS_BLE_ADV_FLAG_LIMIT_DISC (0x01 << 0)
#define OHOS_BLE_ADV_FLAG_GEN_DISC (0x01 << 1)
#define OHOS_BLE_ADV_FLAG_BREDR_NOT_SPT (0x01 << 2)
#define OHOS_BLE_ADV_FLAG_DMT_CONTROLLER_SPT (0x01 << 3)
#define OHOS_BLE_ADV_FLAG_DMT_HOST_SPT (0x01 << 4)
#define OHOS_BLE_ADV_FLAG_NON_LIMIT_DISC (0x00)

/* 特征属性的定义 */
#define    OHOS_GATT_CHAR_PROP_BIT_BROADCAST    (1 << 0)
#define    OHOS_GATT_CHAR_PROP_BIT_READ   (1 << 1)
#define    OHOS_GATT_CHAR_PROP_BIT_WRITE_NR     (1 << 2)
#define    OHOS_GATT_CHAR_PROP_BIT_WRITE  (1 << 3)
#define    OHOS_GATT_CHAR_PROP_BIT_NOTIFY (1 << 4)
#define    OHOS_GATT_CHAR_PROP_BIT_INDICATE     (1 << 5)
#define    OHOS_GATT_CHAR_PROP_BIT_AUTH   (1 << 6)
#define    OHOS_GATT_CHAR_PROP_BIT_EXT_PROP     (1 << 7)

typedef enum {
    BT_ERROR = -1,
    BT_SUCCESS = 0,
    BT_USERAUTHID_ERROR = 1,
    BT_DEVICETYPE_ERROR = 2,
    BT_PARAMINPUT_ERROR = 3,
} BtError;

typedef enum {
    OHOS_BT_TRANSPORT_TYPE_AUTO = 0x0,
    OHOS_BT_TRANSPORT_TYPE_LE,
    OHOS_BT_TRANSPORT_TYPE_CLASSIC,
} BtTransportType;

typedef enum {
    OHOS_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT = 0,
    OHOS_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT,
    OHOS_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT,
    OHOS_GAP_BLE_SCAN_RESULT_EVT,
    OHOS_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT,
    OHOS_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT,
    OHOS_GAP_BLE_ADV_START_COMPLETE_EVT,
    OHOS_GAP_BLE_SCAN_START_COMPLETE_EVT,
    OHOS_GAP_BLE_AUTH_CMPL_EVT = 8,
    OHOS_GAP_BLE_KEY_EVT,
    OHOS_GAP_BLE_SEC_REQ_EVT,
    OHOS_GAP_BLE_PASSKEY_NOTIF_EVT,
    OHOS_GAP_BLE_PASSKEY_REQ_EVT,
    OHOS_GAP_BLE_OOB_REQ_EVT,
    OHOS_GAP_BLE_LOCAL_IR_EVT,
    OHOS_GAP_BLE_LOCAL_ER_EVT,
    OHOS_GAP_BLE_NC_REQ_EVT,
    OHOS_GAP_BLE_ADV_STOP_COMPLETE_EVT,
    OHOS_GAP_BLE_SCAN_STOP_COMPLETE_EVT,
    OHOS_GAP_BLE_SET_STATIC_RAND_ADDR_EVT = 19,
    OHOS_GAP_BLE_UPDATE_CONN_PARAMS_EVT,
    OHOS_GAP_BLE_SET_PKT_LENGTH_COMPLETE_EVT,
    OHOS_GAP_BLE_SET_LOCAL_PRIVACY_COMPLETE_EVT,
    OHOS_GAP_BLE_REMOVE_BOND_DEV_COMPLETE_EVT,
    OHOS_GAP_BLE_CLEAR_BOND_DEV_COMPLETE_EVT,
    OHOS_GAP_BLE_GET_BOND_DEV_COMPLETE_EVT,
    OHOS_GAP_BLE_READ_RSSI_COMPLETE_EVT,
    OHOS_GAP_BLE_UPDATE_WHITELIST_COMPLETE_EVT,
    OHOS_GAP_BLE_UPDATE_DUPLICATE_EXCEPTIONAL_LIST_COMPLETE_EVT,
    OHOS_GAP_BLE_SET_CHANNELS_EVT = 29,
    OHOS_GAP_BLE_EVT_MAX,
} GapBleCallbackEvent;

typedef enum {
    OHOS_GAP_SEARCH_INQ_RES_EVT = 0, /*!< Inquiry result for a peer device. */
    OHOS_GAP_SEARCH_INQ_CMPL_EVT = 1, /*!< Inquiry complete. */
    OHOS_GAP_SEARCH_DISC_RES_EVT = 2, /*!< Discovery result for a peer device. */
    OHOS_GAP_SEARCH_DISC_BLE_RES_EVT = 3, /*!< Discovery result for BL */
    OHOS_GAP_SEARCH_DISC_CMPL_EVT = 4, /*!< Discovery complete. */
    OHOS_GAP_SEARCH_DI_DISC_CMPL_EVT = 5, /*!< Discovery complete. */
    OHOS_GAP_SEARCH_SEARCH_CANCEL_CMPL_EVT = 6, /*!< Search cancelled */
    OHOS_GAP_SEARCH_INQ_DISCARD_NUM_EVT = 7, /*!< The number of pkt discarded by flow control */
} GapSearchEvent;

/// The type of advertising data(not adv_type)
typedef enum {
    OHOS_BLE_AD_TYPE_FLAG   = 0x01, /* relate to BTM_BLE_AD_TYPE_FLAG */
    OHOS_BLE_AD_TYPE_16SRV_PART   = 0x02, /* relate to BTM_BLE_AD_TYPE_16SRV_PAR */
    OHOS_BLE_AD_TYPE_16SRV_CMPL   = 0x03, /* relate to BTM_BLE_AD_TYPE_16SRV_CMPL */
    OHOS_BLE_AD_TYPE_32SRV_PART   = 0x04, /* relate to BTM_BLE_AD_TYPE_32SRV_PART */
    OHOS_BLE_AD_TYPE_32SRV_CMPL   = 0x05, /* relate to BTM_BLE_AD_TYPE_32SRV_CMPL */
    OHOS_BLE_AD_TYPE_128SRV_PART  = 0x06, /* relate to BTM_BLE_AD_TYPE_128SRV_PART */
    OHOS_BLE_AD_TYPE_128SRV_CMPL  = 0x07, /* relate to BTM_BLE_AD_TYPE_128SRV_CMPL */
    OHOS_BLE_AD_TYPE_NAME_SHORT   = 0x08, /* relate to BTM_BLE_AD_TYPE_NAME_SHORT */
    OHOS_BLE_AD_TYPE_NAME_CMPL    = 0x09, /* relate to BTM_BLE_AD_TYPE_NAME_CMPL */
    OHOS_BLE_AD_TYPE_TX_PWR = 0x0A, /* relate to BTM_BLE_AD_TYPE_TX_PWR */
    OHOS_BLE_AD_TYPE_DEV_CLASS    = 0x0D, /* relate to BTM_BLE_AD_TYPE_DEV_CLASS */
    OHOS_BLE_AD_TYPE_SM_TK  = 0x10, /* relate to BTM_BLE_AD_TYPE_SM_TK */
    OHOS_BLE_AD_TYPE_SM_OOB_FLAG  = 0x11, /* relate to BTM_BLE_AD_TYPE_SM_OOB_FLAG */
    OHOS_BLE_AD_TYPE_INT_RANGE    = 0x12, /* relate to BTM_BLE_AD_TYPE_INT_RANGE */
    OHOS_BLE_AD_TYPE_SOL_SRV_UUID = 0x14, /* relate to BTM_BLE_AD_TYPE_SOL_SRV_UUID */
    OHOS_BLE_AD_TYPE_128SOL_SRV_UUID    = 0x15, /* relate to BTM_BLE_AD_TYPE_128SOL_SRV_UUID */
    OHOS_BLE_AD_TYPE_SERVICE_DATA = 0x16, /* relate to BTM_BLE_AD_TYPE_SERVICE_DATA */
    OHOS_BLE_AD_TYPE_PUBLIC_TARGET = 0x17, /* relate to BTM_BLE_AD_TYPE_PUBLIC_TARGET */
    OHOS_BLE_AD_TYPE_RANDOM_TARGET = 0x18, /* relate to BTM_BLE_AD_TYPE_RANDOM_TARGET */
    OHOS_BLE_AD_TYPE_APPEARANCE   = 0x19, /* relate to BTM_BLE_AD_TYPE_APPEARANCE */
    OHOS_BLE_AD_TYPE_ADV_INT = 0x1A, /* relate to BTM_BLE_AD_TYPE_ADV_INT */
    OHOS_BLE_AD_TYPE_LE_DEV_ADDR  = 0x1b, /* relate to BTM_BLE_AD_TYPE_LE_DEV_ADDR */
    OHOS_BLE_AD_TYPE_LE_ROLE = 0x1c, /* relate to BTM_BLE_AD_TYPE_LE_ROLE */
    OHOS_BLE_AD_TYPE_SPAIR_C256   = 0x1d, /* relate to BTM_BLE_AD_TYPE_SPAIR_C256 */
    OHOS_BLE_AD_TYPE_SPAIR_R256   = 0x1e, /* relate to BTM_BLE_AD_TYPE_SPAIR_R256 */
    OHOS_BLE_AD_TYPE_32SOL_SRV_UUID     = 0x1f, /* relate to BTM_BLE_AD_TYPE_32SOL_SRV_UUID */
    OHOS_BLE_AD_TYPE_32SERVICE_DATA     = 0x20, /* relate to BTM_BLE_AD_TYPE_32SERVICE_DATA */
    OHOS_BLE_AD_TYPE_128SERVICE_DATA    = 0x21, /* relate to BTM_BLE_AD_TYPE_128SERVICE_DATA */
    OHOS_BLE_AD_TYPE_LE_SECURE_CONFIRM  = 0x22, /* relate to BTM_BLE_AD_TYPE_LE_SECURE_CONFIRM */
    OHOS_BLE_AD_TYPE_LE_SECURE_RANDOM   = 0x23, /* relate to BTM_BLE_AD_TYPE_LE_SECURE_RANDOM */
    OHOS_BLE_AD_TYPE_URI    = 0x24, /* relate to BTM_BLE_AD_TYPE_URI */
    OHOS_BLE_AD_TYPE_INDOOR_POSITION    = 0x25, /* relate to BTM_BLE_AD_TYPE_INDOOR_POSITION */
    OHOS_BLE_AD_TYPE_TRANS_DISC_DATA    = 0x26, /* relate to BTM_BLE_AD_TYPE_TRANS_DISC_DATA */
    OHOS_BLE_AD_TYPE_LE_SUPPORT_FEATURE = 0x27, /* relate to BTM_BLE_AD_TYPE_LE_SUPPORT_FEATURE */
    OHOS_BLE_AD_TYPE_CHAN_MAP_UPDATE    = 0x28, /* relate to BTM_BLE_AD_TYPE_CHAN_MAP_UPDATE */
    OHOS_BLE_AD_MANUFACTURER_SPECIFIC_TYPE = 0xFF, /* relate to BTM_BLE_AD_MANUFACTURER_SPECIFIC_TYPE */
} BleAdvDataType;

/**
  * @brief Gatt写类型
  */
typedef enum {
    OHOS_GATT_WRITE_TYPE_NO_RSP = 1, /*!< Gatt write attribute need no response */
    OHOS_GATT_WRITE_TYPE_RSP, /*!< Gatt write attribute need remote response */
} GattBleWriteType;

typedef void (* GapBleCallback)(GapBleCallbackEvent event, BleGapParam *param);

BtError EnableBle(void);

BtError DisableBle(void);

BtError SetLocalName(unsigned char *localName, unsigned char length);

BtError GetLocalAddr(unsigned char *mac, unsigned int len);

BtError BleStartScan(void);

BtError BleStopScan(void);

BtError BleGapUpdateConnParams(BleConnUpdateParams *params);

BtError BleGatSetScanParams(BleScanParams *scan_params);

BtError BleGapDisconnect(BdAddr remote_device);

uint8_t *BleResolveAdvData(uint8_t *adv_data, uint8_t type, uint8_t *length);
#endif
