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

#include "bluetooth_service.h"
#include "ohos_run.h"
#include "blegap.h"
#define GATTS_TAG "BLUETOOTH_SERVICE"

/**
 * @brief Registers GATT server callbacks.
 *
 * @param func Indicates the pointer to the callbacks to register, as enumerated in {@link BtGattServerCallbacks}.
 * @return Returns {@link OHOS_BT_STATUS_SUCCESS} if the callbacks are registered;
 * returns an error code defined in {@link BtStatus} otherwise.
 * @since 6
 */
BtError BleGattsRegisterCallbacks(BtGattServerCallbacks func)
{
    esp_err_t ret;
    ret = esp_ble_gatts_register_callback(func.gattsCallback);
    if (ret) {
        ESP_LOGE(GATTS_TAG, "gatts register error, error code = %x", ret);
        return;
    }
    ret = esp_ble_gap_register_callback(func.gapCallback);
    if (ret) {
        ESP_LOGE(GATTS_TAG, "gap register error, error code = %x", ret);
        return;
    }
    ret = esp_ble_gatts_app_register(func.profileAppId);
    if (ret) {
        ESP_LOGE(GATTS_TAG, "gatts app register error, error code = %x", ret);
        return;
    }
    return ret;
}

/**
 * @brief Starts a service.
 *
 * @param serverId Indicates the ID of the GATT server.
 * @param srvcHandle Indicates the handle ID of the service.
 * @return Returns {@link OHOS_BT_STATUS_SUCCESS} if the service is started;
 * returns an error code defined in {@link BtStatus} otherwise.
 * @since 6
 */
BtError BleGattsStartService(int serverId, int srvcHandle)
{
    return esp_ble_gatts_start_service(srvcHandle);
}

/**
 * @brief Stops a service.
 *
 * @param serverId Indicates the ID of the GATT server.
 * @param srvcHandle Indicates the handle ID of the service.
 * @return Returns {@link OHOS_BT_STATUS_SUCCESS} if the service is stopped;
 * returns an error code defined in {@link BtStatus} otherwise.
 * @since 6
 */
BtError BleGattsStopService(int serverId, int srvcHandle)
{
    return esp_ble_gatts_stop_service(srvcHandle);
}

/**
 * @brief Sends an indication or notification to the client.
 *
 * The <b>confirm</b> field in <b>param</b> determines whether to send an indication or a notification.
 *
 * @param serverId Indicates the ID of the GATT server.
 * @param param Indicates the pointer to the sending parameters. For details, see {@link GattsSendIndParam}.
 * @return Returns {@link OHOS_BT_STATUS_SUCCESS} if the indication or notification is sent;
 * returns an error code defined in {@link BtStatus} otherwise.
 * @since 6
 */
BtError BleGattsSendIndication(GattsSendParam *param)
{
    if (param == NULL && param->value == NULL) {
        return BT_PARAMINPUT_ERROR;
    }
    return esp_ble_gatts_send_indicate(param->gattsIf, param->connId, param->attrHandle, param->valueLen, param->value,
                                       param->needConfirm);
}

/**
 * @brief           This function is called to start advertising.
 *
 * @param      advParams: pointer to User defined advParams data structure.

 * @return
 *                  - ESP_OK : success
 *                  - other  : failed
 *
 */
BtError BleGattsStartAdvertising(BleAdvParams2 *advParams)
{
    if (advParams == NULL) {
        BT_DEBUG("BleGattsStartAdvertising param is NULL! \n");
        return BT_PARAMINPUT_ERROR;
    }
    return esp_ble_gap_start_advertising(advParams);
}

/**
 * @brief           This function is called to send a response to a request.
 *
 * @param[in]       gattsIf: GATT server access interface
 * @param[in]       connId - connection identifier.
 * @param[in]       transId - transfer id
 * @param[in]       status - response status
 * @param[in]       rsp - response data.
 *
 * @return
 *                  - ESP_OK : success
 *                  - other  : failed
 *
 */
BtError BleGattsSendResponse(GattInterfaceType gattsIf, uint16_t connId, uint32_t transId, GattStatus status,
                             BleGattRsp *rsp)
{
    return esp_ble_gatts_send_response(gattsIf, connId, transId, status, rsp);
}

/**
 * @brief           This function is called to override the BTA default ADV parameters.
 *
 * @param[in]       advData: Pointer to User defined ADV data structure. This
 *                  memory space can not be freed until callback of config_adv_data
 *                  is received.
 *
 * @return
 *                  - ESP_OK : success
 *                  - other  : failed
 *
 */
BtError BleGapConfigAdvData(BleAdvData *advData)
{
    if (advData == NULL) {
        BT_DEBUG("BleGapConfigAdvData param is NULL! \n");
        return BT_PARAMINPUT_ERROR;
    }
    return esp_ble_gap_config_adv_data(advData);
}

/**
 * @brief           Create a service. When service creation is done, a callback
 *                  event ESP_GATTS_CREATE_EVT is called to report status
 *                  and service ID to the profile. The service ID obtained in
 *                  the callback function needs to be used when adding included
 *                  service and characteristics/descriptors into the service.
 *
 * @param[in]       gattsIf: GATT server access interface
 * @param[in]       serviceId: service ID.
 * @param[in]       numHandle: number of handle requested for this service.
 *
 * @return
 *                  - ESP_OK : success
 *                  - other  : failed
 *
 */
BtError BleGattsCreateService(GattInterfaceType gattsIf, GattSrvcId *serviceId, uint16_t numHandle)
{
    if (serviceId == NULL) {
        BT_DEBUG("BleGattsCreateService param is NULL! \n");
        return BT_PARAMINPUT_ERROR;
    }
    return esp_ble_gatts_create_service(gattsIf, serviceId, numHandle);
}

/**
 * @brief       Retrieve attribute value
 *
 * @param[in]   attrHandle: Attribute handle.
 * @param[out]  length: pointer to the attribute value length
 * @param[out]  value:  Pointer to attribute value payload, the value cannot be modified by user
 *
 * @return
 *                  - ESP_GATT_OK : success
 *                  - other  : failed
 *
 */
BtError BleGattsGetAttrValue(uint16_t attrHandle, uint16_t *length, const uint8_t **value)
{
    if ((length == NULL) || (value == NULL)) {
        BT_DEBUG("BleGattsGetAttrValue param is NULL! \n");
        return BT_PARAMINPUT_ERROR;
    }
    return esp_ble_gatts_get_attr_value(attrHandle, length, value);
}

/**
 * @brief           This function is called to add characteristic descriptor. When
 *                  it's done, a callback event ESP_GATTS_ADD_DESCR_EVT is called
 *                  to report the status and an ID number for this descriptor.
 *
 * @param[in]       serviceHandle: service handle to which this characteristic descriptor is to
 *                              be added.
 * @param[in]       perm: descriptor access permission.
 * @param[in]       descrUuid: descriptor UUID.
 * @param[in]       charDescrVal  : Characteristic descriptor value
 * @param[in]       control : attribute response control byte
 * @return
 *                  - ESP_OK : success
 *                  - other  : failed
 *
 */
BtError BleGattsAddCharDescr(uint16_t serviceHandle, BtUuid *descrUuid, uint16_t perm, BleAttrValue *charDescrVal,
                             BleAttrControl *control)
{
    if (descrUuid == NULL) {
        BT_DEBUG("BleGattsAddCharDescr param is NULL! \n");
        return BT_PARAMINPUT_ERROR;
    }
    return esp_ble_gatts_add_char_descr(serviceHandle, descrUuid, perm, charDescrVal, control);
}

/**
 * @brief           This function is called to add a characteristic into a service.
 *
 * @param[in]       serviceHandle: service handle to which this included service is to
 *                  be added.
 * @param[in]       character : Characteristic.
 *
 * @return
 *                  - ESP_OK : success
 *                  - other  : failed
 *
 */
BtError BleGattsAddChar(uint16_t serviceHandle, GattsChar *character)
{
    if ((charUuid == NULL) || (character == NULL) || (character->charVal == NULL)) {
        BT_DEBUG("BleGattsAddChar param is NULL! \n");
        return BT_PARAMINPUT_ERROR;
    }
    return esp_ble_gatts_add_char(serviceHandle, character->charUuid, character->perm, character->property,
                                  character->charVal, character->control);
}

/**
 * @brief           Update connection parameters, can only be used when connection is up.
 *
 * @param[in]       params   -  connection update parameters
 *
 * @return
 *                  - ESP_OK : success
 *                  - other  : failed
 *
 */
BtError BleGapUpdateConnParams(BleConnUpdateParams *params)
{
    if (params == NULL) {
        BT_DEBUG("BleGapUpdateConnParams param is NULL! \n");
        return BT_PARAMINPUT_ERROR;
    }
    return esp_ble_gap_update_conn_params(params);
}