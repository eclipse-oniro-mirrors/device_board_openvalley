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
#ifndef __BLUETOOTH_DEVICE_H__
#define __BLUETOOTH_DEVICE_H__
#include "securec.h"
#include "blegap.h"
#include "esp_gattc_api.h"
#define GATTC_TAG "BLUETOOTH_DEMO"

#define BleGapParam esp_ble_gap_cb_param_t
#define BleGattcParam esp_ble_gattc_cb_param_t
#define BleScanParams esp_ble_scan_params_t
#define BleGattcCharElem esp_gattc_char_elem_t
#define BleGattcDescrElem esp_gattc_descr_elem_t
#define BtLogBufferHex      ESP_LOG_BUFFER_HEX
#define BtLogBufferChar esp_log_buffer_char
#define ScanTime 30

typedef enum {
    OHOS_GATTC_REG_EVT                 = 0,
    OHOS_GATTC_UNREG_EVT               = 1,
    OHOS_GATTC_OPEN_EVT                = 2,
    OHOS_GATTC_READ_CHAR_EVT           = 3,
    OHOS_GATTC_WRITE_CHAR_EVT          = 4,
    OHOS_GATTC_CLOSE_EVT               = 5,
    OHOS_GATTC_SEARCH_CMPL_EVT         = 6,
    OHOS_GATTC_SEARCH_RES_EVT          = 7,
    OHOS_GATTC_READ_DESCR_EVT          = 8,
    OHOS_GATTC_WRITE_DESCR_EVT         = 9,
    OHOS_GATTC_NOTIFY_EVT              = 10,
    OHOS_GATTC_PREP_WRITE_EVT          = 11,
    OHOS_GATTC_EXEC_EVT                = 12,
    OHOS_GATTC_ACL_EVT                 = 13,
    OHOS_GATTC_CANCEL_OPEN_EVT         = 14,
    OHOS_GATTC_SRVC_CHG_EVT            = 15,
    OHOS_GATTC_ENC_CMPL_CB_EVT         = 17,
    OHOS_GATTC_CFG_MTU_EVT             = 18,
    OHOS_GATTC_ADV_DATA_EVT            = 19,
    OHOS_GATTC_MULT_ADV_ENB_EVT        = 20,
    OHOS_GATTC_MULT_ADV_UPD_EVT        = 21,
    OHOS_GATTC_MULT_ADV_DATA_EVT       = 22,
    OHOS_GATTC_MULT_ADV_DIS_EVT        = 23,
    OHOS_GATTC_CONGEST_EVT             = 24,
    OHOS_GATTC_BTH_SCAN_ENB_EVT        = 25,
    OHOS_GATTC_BTH_SCAN_CFG_EVT        = 26,
    OHOS_GATTC_BTH_SCAN_RD_EVT         = 27,
    OHOS_GATTC_BTH_SCAN_THR_EVT        = 28,
    OHOS_GATTC_BTH_SCAN_PARAM_EVT      = 29,
    OHOS_GATTC_BTH_SCAN_DIS_EVT        = 30,
    OHOS_GATTC_SCAN_FLT_CFG_EVT        = 31,
    OHOS_GATTC_SCAN_FLT_PARAM_EVT      = 32,
    OHOS_GATTC_SCAN_FLT_STATUS_EVT     = 33,
    OHOS_GATTC_ADV_VSC_EVT             = 34,
    OHOS_GATTC_REG_FOR_NOTIFY_EVT      = 38,
    OHOS_GATTC_UNREG_FOR_NOTIFY_EVT    = 39,
    OHOS_GATTC_CONNECT_EVT             = 40,
    OHOS_GATTC_DISCONNECT_EVT          = 41,
    OHOS_GATTC_READ_MULTIPLE_EVT       = 42,
    OHOS_GATTC_QUEUE_FULL_EVT          = 43,
    OHOS_GATTC_SET_ASSOC_EVT           = 44,
    OHOS_GATTC_GET_ADDR_LIST_EVT       = 45,
    OHOS_GATTC_DIS_SRVC_CMPL_EVT       = 46,
} GattcBleCallbackEvent;

typedef enum {
    OHOS_GATT_SERVICE_FROM_REMOTE_DEVICE         = 0,
    OHOS_GATT_SERVICE_FROM_NVS_FLASH             = 1,
    OHOS_GATT_SERVICE_FROM_UNKNOWN               = 2,
} GattServeiceSource;

typedef enum {
    OHOS_GATT_DB_PRIMARY_SERVICE,
    OHOS_GATT_DB_SECONDARY_SERVICE,
    OHOS_GATT_DB_CHARACTERISTIC,
    OHOS_GATT_DB_DESCRIPTOR,
    OHOS_GATT_DB_INCLUDED_SERVICE,
    OHOS_GATT_DB_ALL,
} GattDbAttrType;

typedef void (* GattcBleCallback)(GattcBleCallbackEvent event, GattInterfaceType gattc_if, BleGattcParam *param);

typedef struct {
    GapBleCallback gap_callback;
    GattcBleCallback gattc_callback;
    uint16_t PROFILE_APP_ID;
} BtGattClientCallbacks;

typedef enum {
    OHOS_GATT_AUTH_REQ_NONE                  = 0,
    OHOS_GATT_AUTH_REQ_NO_MITM               = 1,
    OHOS_GATT_AUTH_REQ_MITM                  = 2,
    OHOS_GATT_AUTH_REQ_SIGNED_NO_MITM        = 3,
    OHOS_GATT_AUTH_REQ_SIGNED_MITM           = 4,
} GattBleAuthReq;

typedef struct {
    GattInterfaceType gattc_if;
    uint16_t conn_id;
    uint16_t char_handle;
    BtUuids descr_uuid;
} GattcGetDescr;

typedef struct {
    GattInterfaceType gattc_if;
    uint16_t conn_id;
    uint16_t handle;
    uint16_t value_len;
    GattBleWriteType write_type;
} GattcWriteChar;

typedef struct {
    GattInterfaceType gattc_if;
    uint16_t conn_id;
    esp_gatt_db_attr_type_t type;
    uint16_t start_handle;
    uint16_t end_handle;
} GattcGetAttr;

typedef struct {
    GattInterfaceType gattc_if;
    uint16_t conn_id;
    uint16_t start_handle;
    uint16_t end_handle;
} GattcGetChar;

BtError BleGattcConnect(int clientId, VOID *func, const BdAddr *bdAddr, bool isAutoConnect,
                        BtTransportType transport);

BtError BleGattcDisconnect(int clientId, int conn_id);

BtError BleGattcRegister(BtGattClientCallbacks func);

BtError BleGattcUnRegister(int clientId);

BtError BleGattcConfigureMtuSize(int mtuSize);

BtError BleGattcSearchServices(int clientId, int conn_id, BtUuid *filter_uuid);

BtError BleGattcWriteCharacteristic(GattcWriteChar write_char, uint8_t *value,
                                    GattBleAuthReq auth_req);

BtError BleGattcSendMtuReq(GattInterfaceType gattc_if, uint16_t conn_id);

BtError BleGattcGetAttrCount(GattcGetAttr get_attr,
                             uint16_t char_handle,
                             uint16_t *count);

GattStatus BleGattcGetCharByUuid(GattcGetChar get_char, BtUuids char_uuid,
                                 BleGattcCharElem *result, uint16_t *count);

BtError BleGattcRegisterForNotify(GattInterfaceType gattc_if,
                                  BdAddr server_bda,
                                  uint16_t handle);

GattStatus BleGattcGetDescrByCharHandle(GattcGetDescr get_descr, BleGattcDescrElem *result,
                                        uint16_t *count);

BtError BleGattcWriteCharDescr(GattcWriteChar write_char,
                               uint8_t *value,
                               GattAttributePermission auth_req);
#endif