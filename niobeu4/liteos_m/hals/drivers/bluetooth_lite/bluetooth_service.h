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
#ifndef __BLUETOOTH_SERVICE_H__
#define __BLUETOOTH_SERVICE_H__
#include "securec.h"
#include "blegap.h"
#include "esp_gatts_api.h"

typedef esp_ble_gatts_cb_param_t BleGattsParam;

// Gap回调参数联合体
typedef esp_ble_gap_cb_param_t BleGapParam;
// Gatt服务端回调参数联合体
typedef esp_ble_gatts_cb_param_t BleGattsCbParam;
// Ble扫描参数
typedef esp_ble_scan_params_t BleScanParams;
// 设置属性值类型
typedef esp_attr_value_t BleAttrValue;
// 广播数据结构体
typedef esp_ble_adv_data_t BleAdvData;
// 广播参数
typedef esp_ble_adv_params_t BleAdvParams2;
// GATT远程读请求响应类型
typedef esp_gatt_rsp_t BleGattRsp;
// Gatt服务ID
typedef esp_gatt_srvc_id_t GattSrvcId;
// 属性自动响应标志
typedef esp_attr_control_t BleAttrControl;
// 蓝牙版本号
#define BT_VERSION 1

/// GATT服务回调函数事件
typedef enum {
    OHOS_GATTS_REG_EVT = 0,             /*!< When register application id, the event comes */
    OHOS_GATTS_READ_EVT = 1,            /*!< When gatt client request read operation, the event comes */
    OHOS_GATTS_WRITE_EVT = 2,           /*!< When gatt client request write operation, the event comes */
    OHOS_GATTS_EXEC_WRITE_EVT = 3,      /*!< When gatt client request execute write, the event comes */
    OHOS_GATTS_MTU_EVT = 4,             /*!< When set mtu complete, the event comes */
    OHOS_GATTS_CONF_EVT = 5,            /*!< When receive confirm, the event comes */
    OHOS_GATTS_UNREG_EVT = 6,           /*!< When unregister application id, the event comes */
    OHOS_GATTS_CREATE_EVT = 7,          /*!< When create service complete, the event comes */
    OHOS_GATTS_ADD_INCL_SRVC_EVT = 8,   /*!< When add included service complete, the event comes */
    OHOS_GATTS_ADD_CHAR_EVT = 9,        /*!< When add characteristic complete, the event comes */
    OHOS_GATTS_ADD_CHAR_DESCR_EVT = 10, /*!< When add descriptor complete, the event comes */
    OHOS_GATTS_DELETE_EVT = 11,         /*!< When delete service complete, the event comes */
    OHOS_GATTS_START_EVT = 12,          /*!< When start service complete, the event comes */
    OHOS_GATTS_STOP_EVT = 13,           /*!< When stop service complete, the event comes */
    OHOS_GATTS_CONNECT_EVT = 14,        /*!< When gatt client connect, the event comes */
    OHOS_GATTS_DISCONNECT_EVT = 15,     /*!< When gatt client disconnect, the event comes */
    OHOS_GATTS_OPEN_EVT = 16,           /*!< When connect to peer, the event comes */
    OHOS_GATTS_CANCEL_OPEN_EVT = 17,    /*!< When disconnect from peer, the event comes */
    OHOS_GATTS_CLOSE_EVT = 18,          /*!< When gatt server close, the event comes */
    OHOS_GATTS_LISTEN_EVT = 19,         /*!< When gatt listen to be connected the event comes */
    OHOS_GATTS_CONGEST_EVT = 20,        /*!< When congest happen, the event comes */
    /* following is extra event */
    OHOS_GATTS_RESPONSE_EVT = 21,            /*!< When gatt send response complete, the event comes */
    OHOS_GATTS_CREAT_ATTR_TAB_EVT = 22,      /*!< When gatt create table complete, the event comes */
    OHOS_GATTS_SET_ATTR_VAL_EVT = 23,        /*!< When gatt set attr value complete, the event comes */
    OHOS_GATTS_SEND_SERVICE_CHANGE_EVT = 24, /*!< When gatt send service change indication complete, the event comes */
} GattsBleCallbackEvent;

/**
 * @brief GATT服务端回调函数类型
 * @param event : 事件类型
 * @param gattc_if : GATT客户端访问接口，不同的gattc_if对应不同的profile
 * @param param : 指向回调参数指针，当前是联合类型
 */
typedef void (*GattsBleCallback)(GattsBleCallbackEvent event, GattInterfaceType gattsIf, BleGattsParam *param);

typedef struct {
    GapBleCallback gapCallback;     // gap回调函数
    GattsBleCallback gattsCallback; // gatts回调函数
    uint16_t profileAppId; // 应用程序标识(UUID)，用于不同的应用程序，应用程序回调函数使用
} BtGattServerCallbacks;

typedef struct {
    GattInterfaceType gattsIf;  // GATT服务器访问接口
    uint16_t connId;            // 连接id
    uint16_t attrHandle;        // 属性句柄
    uint16_t valueLen;          // 值的长度
    uint8_t *value;             // 值
    bool needConfirm;           // 是否需要确认
} GattsSendParam;

typedef struct {
    BtUuid *charUuid;           // Characteristic UUID.
    uint16_t perm;              // Characteristic value declaration attribute permission.
    uint8_t property;           // Characteristic Properties
    BleAttrValue *charVal;      // Characteristic value
    BleAttrControl *control;    // attribute response control byte
} GattsChar;

/**
 * @brief 发现请求远程设备上的GATT服务
 *
 * @param clientId 客户端ID
 * @param connId  连接ID
 * @param filterUuid  过滤服务UUID *
 * @return BtResult 成功返回 BT_SUCCESS
 */
BtError BleGattcSearchServices(int clientId, int connId, BtUuid *filterUuid);

/**
 * @brief 注册Gatt服务端回调
 *
 * @param func BtGattServerCallbacks结构体
 * @return BtResult 成功返回 BT_SUCCESS
 */
BtError BleGattsRegisterCallbacks(BtGattServerCallbacks func);

/**
 * @brief 启动Gatt服务
 *
 * @param serverId Gatt服务id
 * @param srvcHandl Gatt服务句柄
 * @return BtResult 成功返回 BT_SUCCESS
 * @since 6
 */
BtError BleGattsStartService(int serverId, int srvcHandle);

/**
 * @brief 停止Gatt服务
 *
 * @param serverId Gatt服务id
 * @param srvcHandl Gatt服务句柄
 * @return BtResult 成功返回 BT_SUCCESS
 */
BtError BleGattsStopService(int serverId, int srvcHandle);

/**
 * @brief 发送指示或通知GATT客户端
 *
 * @param param: GATT服务器发送数据参数
 * @return BtResult 成功返回 BT_SUCCESS
 */
BtError BleGattsSendIndication(GattsSendParam* param);

/**
 * @brief 发送指示或通知GATT客户端
 *
 * @param advParams: 广播参数
 * @return BtResult 成功返回 BT_SUCCESS
 */
BtError BleGattsStartAdvertising(BleAdvParams2 *advParams);

/**
 * @brief 向请求发送响应函数.
 *
 * @param gattsIf: GATT server access interface
 * @param connId - connection identifier.
 * @param transId - transfer id
 * @param status - response status
 * @param rsp - response data.
 * @return BtResult 成功返回 BT_SUCCESS
 *
 */
BtError BleGattsSendResponse(GattInterfaceType gattsIf, uint16_t connId, uint32_t transId, GattStatus status,
                             BleGattRsp *rsp);
/**
 * @brief 覆盖BTA默认的ADV参数
 *
 * @param[in]       advData: Pointer to User defined ADV data structure.
 * @return BtResult 成功返回 BT_SUCCESS
 *
 */
BtError BleGapConfigAdvData(BleAdvData *advData);

/**
 * @brief 创建Gatt服务
 *
 * @param gattsIf: GATT server access interface
 * @param serviceId: service ID.
 * @param numHandle: number of handle requested for this service.
 *
 * @return BtResult 成功返回 BT_SUCCESS
 *
 */
BtError BleGattsCreateService(GattInterfaceType gattsIf, GattSrvcId *serviceId, uint16_t numHandle);

/**
 * @brief 获取属性值
 *
 * @param attrHandle: Attribute handle.
 * @param length: pointer to the attribute value length
 * @param value:  Pointer to attribute value payload, the value cannot be modified by user
 * @return BtResult 成功返回 BT_SUCCESS
 *
 */
BtError BleGattsGetAttrValue(uint16_t attrHandle, uint16_t *length, const uint8_t **value);

/**
 * @brief 添加特征描述符
 *
 * @param serviceHandle: service handle to which this characteristic descriptor is to
 *                              be added.
 * @param perm: descriptor access permission.
 * @param descrUuid: descriptor UUID.
 * @param charDescrVal  : Characteristic descriptor value
 * @param control : attribute response control byte
 * @return BtResult 成功返回 BT_SUCCESS
 *
 */
BtError BleGattsAddCharDescr(uint16_t serviceHandle, BtUuid *descrUuid, uint16_t perm, BleAttrValue *charDescrVal,
                             BleAttrControl *control);

/**
 * @brief 将特征添加到服务中
 *
 * @param serviceHandle: service handle to which this included service is to
 *                  be added.
 * @param character: 特征值参数
 * @return BtResult 成功返回 BT_SUCCESS
 *
 */
BtError BleGattsAddChar(uint16_t serviceHandle, GattsChar* character);
#endif