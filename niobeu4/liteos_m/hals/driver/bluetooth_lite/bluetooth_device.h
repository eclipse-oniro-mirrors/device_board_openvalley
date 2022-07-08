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
#ifndef __BLEMASTERKITS_H__
#define __BLEMASTERKITS_H__
#include "blegap.h"
#include "esp_gattc_api.h"
#define GATTC_TAG "BLUETOOTH_DEMO"

#define BleGapParam esp_ble_gap_cb_param_t       //Gap回调参数联合体
#define BleGattcParam esp_ble_gattc_cb_param_t   //Gatt客户端回调参数联合体
#define BleScanParams esp_ble_scan_params_t      //Ble扫描参数
#define BleGattcCharElem esp_gattc_char_elem_t   //Gatt描述符元素元素
#define BleGattcDescrElem esp_gattc_descr_elem_t //描述符元素
#define BtLogBufferHex      ESP_LOG_BUFFER_HEX   //打印HEX
#define BtLogBufferChar esp_log_buffer_char      //打印char
#define BT_VERSION 1                             //蓝牙版本号
#define DataLength 4                             //userAuthID数据长度

/// GATT客户端回调函数事件
typedef enum {
    OHOS_GATTC_REG_EVT                 = 0,        /*!< When GATT client is registered, the event comes */
    OHOS_GATTC_UNREG_EVT               = 1,        /*!< When GATT client is unregistered, the event comes */
    OHOS_GATTC_OPEN_EVT                = 2,        /*!< When GATT virtual connection is set up, the event comes */
    OHOS_GATTC_READ_CHAR_EVT           = 3,        /*!< When GATT characteristic is read, the event comes */
    OHOS_GATTC_WRITE_CHAR_EVT          = 4,        /*!< When GATT characteristic write operation completes, the event comes */
    OHOS_GATTC_CLOSE_EVT               = 5,        /*!< When GATT virtual connection is closed, the event comes */
    OHOS_GATTC_SEARCH_CMPL_EVT         = 6,        /*!< When GATT service discovery is completed, the event comes */
    OHOS_GATTC_SEARCH_RES_EVT          = 7,        /*!< When GATT service discovery result is got, the event comes */
    OHOS_GATTC_READ_DESCR_EVT          = 8,        /*!< When GATT characteristic descriptor read completes, the event comes */
    OHOS_GATTC_WRITE_DESCR_EVT         = 9,        /*!< When GATT characteristic descriptor write completes, the event comes */
    OHOS_GATTC_NOTIFY_EVT              = 10,       /*!< When GATT notification or indication arrives, the event comes */
    OHOS_GATTC_PREP_WRITE_EVT          = 11,       /*!< When GATT prepare-write operation completes, the event comes */
    OHOS_GATTC_EXEC_EVT                = 12,       /*!< When write execution completes, the event comes */
    OHOS_GATTC_ACL_EVT                 = 13,       /*!< When ACL connection is up, the event comes */
    OHOS_GATTC_CANCEL_OPEN_EVT         = 14,       /*!< When GATT client ongoing connection is cancelled, the event comes */
    OHOS_GATTC_SRVC_CHG_EVT            = 15,       /*!< When "service changed" occurs, the event comes */
    OHOS_GATTC_ENC_CMPL_CB_EVT         = 17,       /*!< When encryption procedure completes, the event comes */
    OHOS_GATTC_CFG_MTU_EVT             = 18,       /*!< When configuration of MTU completes, the event comes */
    OHOS_GATTC_ADV_DATA_EVT            = 19,       /*!< When advertising of data, the event comes */
    OHOS_GATTC_MULT_ADV_ENB_EVT        = 20,       /*!< When multi-advertising is enabled, the event comes */
    OHOS_GATTC_MULT_ADV_UPD_EVT        = 21,       /*!< When multi-advertising parameters are updated, the event comes */
    OHOS_GATTC_MULT_ADV_DATA_EVT       = 22,       /*!< When multi-advertising data arrives, the event comes */
    OHOS_GATTC_MULT_ADV_DIS_EVT        = 23,       /*!< When multi-advertising is disabled, the event comes */
    OHOS_GATTC_CONGEST_EVT             = 24,       /*!< When GATT connection congestion comes, the event comes */
    OHOS_GATTC_BTH_SCAN_ENB_EVT        = 25,       /*!< When batch scan is enabled, the event comes */
    OHOS_GATTC_BTH_SCAN_CFG_EVT        = 26,       /*!< When batch scan storage is configured, the event comes */
    OHOS_GATTC_BTH_SCAN_RD_EVT         = 27,       /*!< When Batch scan read event is reported, the event comes */
    OHOS_GATTC_BTH_SCAN_THR_EVT        = 28,       /*!< When Batch scan threshold is set, the event comes */
    OHOS_GATTC_BTH_SCAN_PARAM_EVT      = 29,       /*!< When Batch scan parameters are set, the event comes */
    OHOS_GATTC_BTH_SCAN_DIS_EVT        = 30,       /*!< When Batch scan is disabled, the event comes */
    OHOS_GATTC_SCAN_FLT_CFG_EVT        = 31,       /*!< When Scan filter configuration completes, the event comes */
    OHOS_GATTC_SCAN_FLT_PARAM_EVT      = 32,       /*!< When Scan filter parameters are set, the event comes */
    OHOS_GATTC_SCAN_FLT_STATUS_EVT     = 33,       /*!< When Scan filter status is reported, the event comes */
    OHOS_GATTC_ADV_VSC_EVT             = 34,       /*!< When advertising vendor spec content event is reported, the event comes */
    OHOS_GATTC_REG_FOR_NOTIFY_EVT      = 38,       /*!< When register for notification of a service completes, the event comes */
    OHOS_GATTC_UNREG_FOR_NOTIFY_EVT    = 39,       /*!< When unregister for notification of a service completes, the event comes */
    OHOS_GATTC_CONNECT_EVT             = 40,       /*!< When the ble physical connection is set up, the event comes */
    OHOS_GATTC_DISCONNECT_EVT          = 41,       /*!< When the ble physical connection disconnected, the event comes */
    OHOS_GATTC_READ_MULTIPLE_EVT       = 42,       /*!< When the ble characteristic or descriptor multiple complete, the event comes */
    OHOS_GATTC_QUEUE_FULL_EVT          = 43,       /*!< When the gattc command queue full, the event comes */
    OHOS_GATTC_SET_ASSOC_EVT           = 44,       /*!< When the ble gattc set the associated address complete, the event comes */
    OHOS_GATTC_GET_ADDR_LIST_EVT       = 45,       /*!< When the ble get gattc address list in cache finish, the event comes */
    OHOS_GATTC_DIS_SRVC_CMPL_EVT       = 46,       /*!< When the ble discover service complete, the event comes */
} GattcBleCallbackEvent;

typedef enum {
    OHOS_GATT_SERVICE_FROM_REMOTE_DEVICE         = 0,                                       /* relate to BTA_GATTC_SERVICE_INFO_FROM_REMOTE_DEVICE in bta_gattc_int.h */
    OHOS_GATT_SERVICE_FROM_NVS_FLASH             = 1,                                       /* relate to BTA_GATTC_SERVICE_INFO_FROM_NVS_FLASH in bta_gattc_int.h */
    OHOS_GATT_SERVICE_FROM_UNKNOWN               = 2,                                       /* relate to BTA_GATTC_SERVICE_INFO_FROM_UNKNOWN in bta_gattc_int.h */
} GattServeiceSource;

/**
  * @brief Gatt属性元素的类型
  */
typedef enum {
    OHOS_GATT_DB_PRIMARY_SERVICE,                            /*!< Gattc primary service attribute type in the cache */
    OHOS_GATT_DB_SECONDARY_SERVICE,                          /*!< Gattc secondary service attribute type in the cache */
    OHOS_GATT_DB_CHARACTERISTIC,                             /*!< Gattc characteristic attribute type in the cache */
    OHOS_GATT_DB_DESCRIPTOR,                                 /*!< Gattc characteristic descriptor attribute type in the cache */
    OHOS_GATT_DB_INCLUDED_SERVICE,                           /*!< Gattc include service attribute type in the cache */
    OHOS_GATT_DB_ALL,                                        /*!< Gattc all the attribute (primary service & secondary service & include service & char & descriptor) type in the cache */
} GattDbAttrType;    

/**
 * @brief GATT客户端回调函数类型
 * @param event : 事件类型
 * @param gattc_if : GATT客户端访问接口，不同的gattc_if对应不同的profile
 * @param param : 指向回调参数指针，当前是联合类型
 */
typedef void (* GattcBleCallback)(GattcBleCallbackEvent event, GattInterfaceType gattc_if, BleGattcParam *param);

typedef struct {
    GapBleCallback gap_callback; //gap回调函数
    GattcBleCallback gattc_callback; //gattc回调函数
    uint16_t PROFILE_APP_ID; //应用程序标识(UUID)，用于不同的应用程序，应用程序回调函数使用
} BtGattClientCallbacks;

/**
 * @brief Gatt认证请求类型
 */
typedef enum {
    OHOS_GATT_AUTH_REQ_NONE                  = 0,                                       /* relate to BTA_GATT_AUTH_REQ_NONE in bta/bta_gatt_api.h */
    OHOS_GATT_AUTH_REQ_NO_MITM               = 1,   /* unauthenticated encryption */    /* relate to BTA_GATT_AUTH_REQ_NO_MITM in bta/bta_gatt_api.h */
    OHOS_GATT_AUTH_REQ_MITM                  = 2,   /* authenticated encryption */      /* relate to BTA_GATT_AUTH_REQ_MITM in bta/bta_gatt_api.h */
    OHOS_GATT_AUTH_REQ_SIGNED_NO_MITM        = 3,                                       /* relate to BTA_GATT_AUTH_REQ_SIGNED_NO_MITM in bta/bta_gatt_api.h */
    OHOS_GATT_AUTH_REQ_SIGNED_MITM           = 4,                                       /* relate to BTA_GATT_AUTH_REQ_SIGNED_MITM in bta/bta_gatt_api.h */
} GattBleAuthReq;

/**
 * @brief 创建Gatt连接远端设备
 *
 * @param clientId GATT客户端ID
 * @param bdAddr 远端设备地址
 * @param isAutoConnect 是否直连
 * @param transport 蓝牙传输通道
 * @return BtResult 成功返回 BT_SUCCESS
 */
BtError BleGattcConnect(int clientId, void *func, const BdAddr *bdAddr, bool isAutoConnect, BtTransportType transport);

/**
 * @brief 断开与远端设备Gatt连接
 *
 * @param clientId GATT客户端ID
 * @return BtResult 成功返回 BT_SUCCESS
 */
BtError BleGattcDisconnect(int clientId, int conn_id);

/**
 * @brief 注册Gatt回调函数
 *
 * @param func BtGattClientCallbacks结构体
 * @return BtResult 成功返回 BT_SUCCESS
 */
BtError BleGattcRegister(BtGattClientCallbacks func);

/**
 * @brief 注销Gatt客户端
 *
 * @param clientId Gatt客户端ID
 * @return BtResult 成功返回 BT_SUCCESS
 */
BtError BleGattcUnRegister(int clientId);

/**
 * @brief 配置ATT MTU大小
 *
 * @param mtuSize MTU的大小值
 * @return BtResult 成功返回 BT_SUCCESS
 */
BtError BleGattcConfigureMtuSize(int mtuSize);

/**
 * @brief 发现请求远程设备上的GATT服务
 *
 * @param clientId 客户端ID
 * @param conn_id  连接ID
 * @param filter_uuid  过滤服务UUID * 
 * @return BtResult 成功返回 BT_SUCCESS
 */
BtError BleGattcSearchServices(int clientId, int conn_id, BtUuid *filter_uuid);

/**
 * @brief 写入特征值到远端设备函数
 *
 * @param gattc_if: Gatt客户端访问接口
 * @param conn_id : 连接ID.
 * @param handle : 写的特征句柄
 * @param value_len: 要写入的值的长度
 * @param value : 要写入的值
 * @param write_type : 属性写操作的类型
 * @param auth_req : 认证请求
 * @return BtResult 成功返回 BT_SUCCESS
 */
BtError BleGattcWriteCharacteristic(GattInterfaceType gattc_if,
                                   uint16_t conn_id, uint16_t handle,
                                   uint16_t value_len,
                                   uint8_t *value,
                                   GattBleWriteType write_type,
                                   GattBleAuthReq auth_req);

/**
 * @brief           Configure the MTU size in the GATT channel. This can be done
 *                  only once per connection. Before using, use esp_ble_gatt_set_local_mtu()
 *                  to configure the local MTU size.
 *
 *
 * @param gattc_if: Gatt client access interface.
 * @param conn_id: connection ID.
 *
 * @return BtResult 成功返回 BT_SUCCESS
 *
 */
BtError BleGattcSendMtuReq(GattInterfaceType gattc_if, uint16_t conn_id);

/**
 * @brief 在gatc缓存中查找具有给定服务或特征的属性计数
 *
 * @param gattc_if: Gatt client access interface.
 * @param conn_id: connection ID which identify the server.
 * @param type: the attribute type.
 * @param start_handle: the attribute start handle, if the type is ESP_GATT_DB_DESCRIPTOR, this parameter should be ignore
 * @param end_handle: the attribute end handle, if the type is ESP_GATT_DB_DESCRIPTOR, this parameter should be ignore
 * @param char_handle: the characteristic handle, this parameter valid when the type is ESP_GATT_DB_DESCRIPTOR. If the type
 *                               isn't ESP_GATT_DB_DESCRIPTOR, this parameter should be ignore.
 * @param[out]      count: output the number of attribute has been found in the gattc cache with the given attribute type.
 *
 * @return BtResult 成功返回 BT_SUCCESS
 *
 */
BtError BleGattcGetAttrCount(GattInterfaceType gattc_if,
                                               uint16_t conn_id,
                                               esp_gatt_db_attr_type_t type,
                                               uint16_t start_handle,
                                               uint16_t end_handle,
                                               uint16_t char_handle,
                                               uint16_t *count);

/**
 * @brief 在gatTc缓存中找到具有给定uuid的特征
 *
 * @param gattc_if: Gatt client access interface.
 * @param conn_id: connection ID which identify the server.
 * @param start_handle: the attribute start handle
 * @param end_handle: the attribute end handle
 * @param char_uuid: the characteristic uuid
 * @param[out]      result: The pointer to the characteristic in the service.
 * @param[inout]   count: input the number of characteristic want to find,
 *                         it will output the number of characteristic has been found in the gattc cache with the given service.
 *
 * @return BtResult 返回 GattStatus
 *
 */
GattStatus BleGattcGetCharByUuid(GattInterfaceType gattc_if,
                                                 uint16_t conn_id,
                                                 uint16_t start_handle,
                                                 uint16_t end_handle,
                                                 BtUuids char_uuid,
                                                 BleGattcCharElem *result,
                                                 uint16_t *count);

/**
 * @brief 调用此函数来注册服务通知 
 *
 * @param gattc_if: Gatt client access interface.
 * @param server_bda : target GATT server.
 * @param handle : GATT characteristic handle.
 *
 * @return BtResult 成功返回 BT_SUCCESS
 *
 */
BtError BleGattcRegisterForNotify(GattInterfaceType gattc_if,
                                             BdAddrs server_bda,
                                             uint16_t handle);

/**
 * @brief 在gaTtc缓存中查找具有给定特征句柄的描述符
 *
 * @param gattc_if: Gatt client access interface.
 * @param conn_id: connection ID which identify the server.
 * @param char_handle: the characteristic handle.
 * @param descr_uuid: the descriptor uuid.
 * @param[out]      result: The pointer to the descriptor in the given characteristic.
 * @param[inout]   count: input the number of descriptor want to find,
 *                         it will output the number of descriptor has been found in the gattc cache with the given characteristic.
 * @return BtResult 成功返回 GattStatus
 *
 */
GattStatus BleGattcGetDescrByCharHandle(GattInterfaceType gattc_if,
                                                         uint16_t conn_id,
                                                         uint16_t char_handle,
                                                         BtUuids descr_uuid,
                                                         BleGattcDescrElem *result,
                                                         uint16_t *count);

/**
 * @brief 写入特征描述符值
 *
 * @param gattc_if: Gatt client access interface.
 * @param conn_id : connection ID
 * @param handle : descriptor hadle to write.
 * @param value_len: length of the value to be written.
 * @param value : the value to be written.
 * @param write_type : the type of attribute write operation.
 * @param auth_req : authentication request.
 * @return BtResult 成功返回 BT_SUCCESS
 *
 */
BtError BleGattcWriteCharDescr(GattInterfaceType gattc_if,
                                         uint16_t conn_id,
                                         uint16_t handle,
                                         uint16_t value_len,
                                         uint8_t *value,
                                         BtGattWriteType write_type,
                                         GattAttributePermission auth_req);
#endif