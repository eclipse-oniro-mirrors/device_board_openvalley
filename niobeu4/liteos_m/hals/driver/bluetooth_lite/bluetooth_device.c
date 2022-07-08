/*
 * Copyright (c) 2022 OpenValley Digital Co., Ltd.
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
#include "ohos_run.h"
#include "blegap.h"
#include "bluetooth_device.h"
#define NULL 0L

/**
 * @brief Enable ble.
 *
 * @return Returns <b>true</b> if the operation is accepted;
 *         returns <b>false</b> if the operation is rejected.
 */
BtError EnableBle(void)
{
    esp_err_t ret; 
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(GATTC_TAG, "%s initialize controller failed: %s\n", __func__, esp_err_to_name(ret));
        return ret;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(GATTC_TAG, "%s enable controller failed: %s\n", __func__, esp_err_to_name(ret));
        return ret;
    }

    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(GATTC_TAG, "%s init bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
        return ret;
    }

    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(GATTC_TAG, "%s enable bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
        return ret;
    }
    return ret;
}

/**
 * @brief Disable ble.
 *
 * @return Returns <b>true</b> if the operation is accepted;
 *         returns <b>false</b> if the operation is rejected.
 */
BtError DisableBle(void)
{
    if(esp_bluedroid_get_status() == ESP_BLUEDROID_STATUS_ENABLED) {
        esp_bluedroid_disable();
        esp_bt_controller_disable();
    }
    return BT_SUCCESS;
}

/**
 * @brief Set local device name.
 * 
 * @param localName Device name.
 * @param length localName length, The maximum length of the name is {@link OHOS_BD_NAME_LEN}.
 * @return Returns <b>true</b> if the operation is successful;
 *         returns <b>false</b> if the operation fails.
 */
BtError SetLocalName(unsigned char *localName, unsigned char length)
{
    if (esp_bluedroid_get_status() != ESP_BLUEDROID_STATUS_ENABLED) {
        printf("bluedroid is not enable! \n");
        return ESP_ERR_INVALID_STATE;
    }
    if ((localName == NULL) || (length <= 0)) {
        BT_DEBUG("SetLocalName param is NULL! \n");
        return BT_PARAMINPUT_ERROR;
    }
    return esp_ble_gap_set_device_name(localName);
}

/**
 * @brief Get local host bluetooth address.
 * 
 * @return Returns <b>true</b> if the operation is accepted;
 *         returns <b>false</b> if the operation is rejected.
 */
BtError GetLocalAddr(unsigned char *mac, unsigned int len)
{
    if ((mac == NULL) || (len <= 0)) {
        BT_DEBUG("GetLocalAddr param is NULL! \n");
        return BT_PARAMINPUT_ERROR;
    }
    memcpy(mac, esp_bt_dev_get_address(), len);
    return BT_SUCCESS;
}

/**
 * @brief Starts a scan.
 *
 * @return Returns {@link OHOS_BT_STATUS_SUCCESS} if the scan is started;
 * returns an error code defined in {@link BtStatus} otherwise.
 * @since 6
 */
BtError BleStartScan(void)
{
    if (esp_bluedroid_get_status() != ESP_BLUEDROID_STATUS_ENABLED) {
        return ESP_ERR_INVALID_STATE;
    }
    return esp_ble_gap_start_scanning(30);
}

/**
 * @brief Stops a scan.
 *
 * @return Returns {@link OHOS_BT_STATUS_SUCCESS} if the scan is stopped;
 * returns an error code defined in {@link BtStatus} otherwise.
 * @since 6
 */
BtError BleStopScan(void)
{
    if (esp_bluedroid_get_status() != ESP_BLUEDROID_STATUS_ENABLED) {
        return ESP_ERR_INVALID_STATE;
    }
    return esp_ble_gap_stop_scanning();
}

/**
 * @brief Create a Gatt connection to a remote device.
 *
 * @param clientId Indicates the ID of the GATT client.
 * @param bdAddr Indicates the remote device's address.
 * @param isAutoConnect Indicates whether it is a direct connection(false) or a background connection(true).
 * @param transport Indicates the transport of Gatt client {@link BtTransportType}.
 * @return Returns the operation result status {@link BtStatus}.
 */
BtError BleGattcConnect(int clientId, void *func, const BdAddr *bdAddr, bool isAutoConnect, BtTransportType transport)
{
    if (bdAddr == NULL) {
        BT_DEBUG("BleGattcConnect param is NULL! \n");
        return BT_PARAMINPUT_ERROR;
    }
    return esp_ble_gattc_open(clientId, bdAddr, transport, isAutoConnect);//根据mac地址连接蓝牙
}

/**
 * @brief Disconnect a Gatt connection with a remote device.
 *
 * @param clientId Indicates the ID of the GATT client.
 * @Returns the operation result status {@link BtStatus}.
 */
BtError BleGattcDisconnect(int clientId, int conn_id)
{
    return esp_ble_gattc_close(clientId, conn_id);
}

/**
* @brief           This function is to disconnect the physical connection of the peer device
*                  gattc may have multiple virtual GATT server connections when multiple app_id registered.
*                  esp_ble_gattc_close (esp_gatt_if_t gattc_if, uint16_t conn_id) only close one virtual GATT server connection.
*                  if there exist other virtual GATT server connections, it does not disconnect the physical connection.
*                  esp_ble_gap_disconnect(esp_bd_addr_t remote_device) disconnect the physical connection directly.
*
*
*
* @param[in]       remote_device : BD address of the peer device
*
* @return            - ESP_OK : success
*                    - other  : failed
*
*/
BtError BleGapDisconnect(BdAddrs remote_device)
{
    return esp_ble_gap_disconnect(remote_device);
}

/**
 * @brief          This function is called to get ADV data for a specific type.
 *
 * @param[in]       adv_data - pointer of ADV data which to be resolved
 * @param[in]       type   - finding ADV data type
 * @param[out]      length - return the length of ADV data not including type
 *
 * @return          pointer of ADV data
 *
 */
uint8_t *BleResolveAdvData(uint8_t *adv_data, uint8_t type, uint8_t *length)
{
    if ((adv_data == NULL) || (length <= 0)) {
        BT_DEBUG("BleResolveAdvData param is NULL! \n");
        return BT_PARAMINPUT_ERROR;
    }
    return esp_ble_resolve_adv_data(adv_data, type, length);
}

/**
 * @brief Configure the ATT MTU size.
 *
 * @param clientId Indicates the ID of the GATT client.
 * @param mtuSize The size of MTU.
 * @return Returns the operation result status {@link BtStatus}.
 */
BtError BleGattcConfigureMtuSize(int mtuSize)
{
    return esp_ble_gatt_set_local_mtu(mtuSize);
}

BtError BleGattcRegister(BtGattClientCallbacks func)
{
    esp_err_t ret;
    //register the  callback function to the gap module
    ret = esp_ble_gap_register_callback(func.gap_callback);
    if (ret){
        ESP_LOGE(GATTC_TAG, "%s gap register failed, error code = %x\n", __func__, ret);
        return ret;
    }

    //register the callback function to the gattc module
    ret = esp_ble_gattc_register_callback(func.gattc_callback);
    if(ret){
        ESP_LOGE(GATTC_TAG, "%s gattc register failed, error code = %x\n", __func__, ret);
        return ret;
    }

    ret = esp_ble_gattc_app_register(func.PROFILE_APP_ID);
    if (ret){
        ESP_LOGE(GATTC_TAG, "%s gattc app register failed, error code = %x\n", __func__, ret);
    }
    return ret;
}

/**
 * @brief Unregisters a GATT client with a specified ID.
 *
 * @param clientId Indicates the ID of the GATT client.
 * @return Returns the operation result status {@link BtStatus}.
 */
BtError BleGattcUnRegister(int clientId)
{
    return esp_ble_gattc_app_unregister(clientId);
}

/**
 * @brief Request a GATT service discovery on a remote device.
 *
 * @param clientId Indicates the ID of the GATT client.
 * @return Returns the operation result status {@link BtStatus}.
 */
BtError BleGattcSearchServices(int clientId, int conn_id, BtUuid *filter_uuid)
{
    if (filter_uuid == NULL) {
        BT_DEBUG("BleResolveAdvData param is NULL! \n");
        return BT_PARAMINPUT_ERROR;
    }
    esp_bt_uuid_t remote_filter_service_uuid = {
        .len = filter_uuid->uuidLen,
        .uuid = {.uuid16 = (uint16_t)filter_uuid->uuid,},
    };
    esp_ble_gattc_search_service(clientId, conn_id, &remote_filter_service_uuid);
}

/**
 * @brief Write characteristic value to the remote device.
 *
 * @param clientId Indicates the ID of the GATT client.
 * @param characteristic The specified characteristic {@link BtGattCharacteristic} to be read.
 * @param writeType Indicates the characteristic write type.
 * @param value The value to be write.
 * @param len The length of the value.
 * @return Returns the operation result status {@link BtStatus}.
 */
BtError BleGattcWriteCharacteristic(GattInterfaceType gattc_if,
                                   uint16_t conn_id, uint16_t handle,
                                   uint16_t value_len,
                                   uint8_t *value,
                                   GattBleWriteType write_type,
                                   GattBleAuthReq auth_req)
{
    if ((value == NULL) || (value_len <= 0)) {
        BT_DEBUG("BleGattcWriteCharacteristic param is NULL! \n");
        return BT_PARAMINPUT_ERROR;
    }
    return esp_ble_gattc_write_char(gattc_if, conn_id, handle, value_len, value, write_type, auth_req);
}

/**
 * @brief           This function is called to set scan parameters
 *
 * @param[in]       scan_params: Pointer to User defined scan_params data structure. This
 *                  memory space can not be freed until callback of set_scan_params
 *
 * @return
 *                  - ESP_OK : success
 *                  - other  : failed
 *
 */
BtError BleGatSetScanParams(BleScanParams *scan_params)
{
    if (scan_params == NULL) {
        BT_DEBUG("BleGatSetScanParams param is NULL! \n");
        return BT_PARAMINPUT_ERROR;
    }
    return esp_ble_gap_set_scan_params(scan_params);
}

/**
 * @brief           Configure the MTU size in the GATT channel. This can be done
 *                  only once per connection. Before using, use esp_ble_gatt_set_local_mtu()
 *                  to configure the local MTU size.
 *
 *
 * @param[in]       gattc_if: Gatt client access interface.
 * @param[in]       conn_id: connection ID.
 *
 * @return
 *                  - ESP_OK: success
 *                  - other: failed
 *
 */
BtError BleGattcSendMtuReq(GattInterfaceType gattc_if, uint16_t conn_id)
{
    return esp_ble_gattc_send_mtu_req(gattc_if, conn_id);
}

/**
 * @brief           Find the attribute count with the given service or characteristic in the gattc cache
 *
 * @param[in]       gattc_if: Gatt client access interface.
 * @param[in]       conn_id: connection ID which identify the server.
 * @param[in]       type: the attribute type.
 * @param[in]       start_handle: the attribute start handle, if the type is ESP_GATT_DB_DESCRIPTOR, this parameter should be ignore
 * @param[in]       end_handle: the attribute end handle, if the type is ESP_GATT_DB_DESCRIPTOR, this parameter should be ignore
 * @param[in]       char_handle: the characteristic handle, this parameter valid when the type is ESP_GATT_DB_DESCRIPTOR. If the type
 *                               isn't ESP_GATT_DB_DESCRIPTOR, this parameter should be ignore.
 * @param[out]      count: output the number of attribute has been found in the gattc cache with the given attribute type.
 *
 * @return
 *                  - ESP_OK: success
 *                  - other: failed
 *
 */
BtError BleGattcGetAttrCount(GattInterfaceType gattc_if,
                                               uint16_t conn_id,
                                               esp_gatt_db_attr_type_t type,
                                               uint16_t start_handle,
                                               uint16_t end_handle,
                                               uint16_t char_handle,
                                               uint16_t *count)
{
    if (count == NULL) {
        BT_DEBUG("BleGattcGetAttrCount param is NULL! \n");
        return BT_PARAMINPUT_ERROR;
    }
    return esp_ble_gattc_get_attr_count(gattc_if, conn_id, type, start_handle, end_handle, char_handle, count);
}

/**
 * @brief           Find the characteristic with the given characteristic uuid in the gattc cache
 *                  Note: It just get characteristic from local cache, won't get from remote devices.
 *
 * @param[in]       gattc_if: Gatt client access interface.
 * @param[in]       conn_id: connection ID which identify the server.
 * @param[in]       start_handle: the attribute start handle
 * @param[in]       end_handle: the attribute end handle
 * @param[in]       char_uuid: the characteristic uuid
 * @param[out]      result: The pointer to the characteristic in the service.
 * @param[inout]   count: input the number of characteristic want to find,
 *                         it will output the number of characteristic has been found in the gattc cache with the given service.
 *
 * @return
 *                  - ESP_OK: success
 *                  - other: failed
 *
 */
GattStatus BleGattcGetCharByUuid(GattInterfaceType gattc_if,
                                                 uint16_t conn_id,
                                                 uint16_t start_handle,
                                                 uint16_t end_handle,
                                                 BtUuids char_uuid,
                                                 BleGattcCharElem *result,
                                                 uint16_t *count)
{
    if ((result == NULL) || (count == NULL)) {
        BT_DEBUG("BleGattcGetCharByUuid param is NULL! \n");
        return BT_PARAMINPUT_ERROR;
    }
    return esp_ble_gattc_get_char_by_uuid(gattc_if, conn_id, start_handle, end_handle, char_uuid, result, count);
}

/**
 * @brief           This function is called to register for notification of a service.
 *
 * @param[in]       gattc_if: Gatt client access interface.
 * @param[in]       server_bda : target GATT server.
 * @param[in]       handle : GATT characteristic handle.
 *
 * @return
 *                  - ESP_OK: registration succeeds
 *                  - other: failed
 *
 */
BtError BleGattcRegisterForNotify(GattInterfaceType gattc_if,
                                             BdAddrs server_bda,
                                             uint16_t handle)
{
    return esp_ble_gattc_register_for_notify(gattc_if, server_bda, handle);
}

/**
 * @brief           Find the descriptor with the given characteristic handle in the gattc cache
 *                  Note: It just get descriptor from local cache, won't get from remote devices.
 *
 * @param[in]       gattc_if: Gatt client access interface.
 * @param[in]       conn_id: connection ID which identify the server.
 * @param[in]       char_handle: the characteristic handle.
 * @param[in]       descr_uuid: the descriptor uuid.
 * @param[out]      result: The pointer to the descriptor in the given characteristic.
 * @param[inout]   count: input the number of descriptor want to find,
 *                         it will output the number of descriptor has been found in the gattc cache with the given characteristic.
 *
 * @return
 *                  - ESP_OK: success
 *                  - other: failed
 *
 */
GattStatus BleGattcGetDescrByCharHandle(GattInterfaceType gattc_if,
                                                         uint16_t conn_id,
                                                         uint16_t char_handle,
                                                         BtUuids descr_uuid,
                                                         BleGattcDescrElem *result,
                                                         uint16_t *count)
{
    if ((result == NULL) || (count == NULL)) {
        BT_DEBUG("BleGattcGetCharByUuid param is NULL! \n");
        return BT_PARAMINPUT_ERROR;
    }
    return esp_ble_gattc_get_descr_by_char_handle(gattc_if, conn_id, char_handle, descr_uuid, result, count);
}

/**
 * @brief           This function is called to write characteristic descriptor value.
 *
 * @param[in]       gattc_if: Gatt client access interface.
 * @param[in]       conn_id : connection ID
 * @param[in]       handle : descriptor hadle to write.
 * @param[in]       value_len: length of the value to be written.
 * @param[in]       value : the value to be written.
 * @param[in]       write_type : the type of attribute write operation.
 * @param[in]       auth_req : authentication request.
 *
 * @return
 *                  - ESP_OK: success
 *                  - other: failed
 *
 */
BtError BleGattcWriteCharDescr(GattInterfaceType gattc_if,
                                         uint16_t conn_id,
                                         uint16_t handle,
                                         uint16_t value_len,
                                         uint8_t *value,
                                         BtGattWriteType write_type,
                                         GattAttributePermission auth_req)
{
    if (value == NULL) {
        BT_DEBUG("BleGattcWriteCharDescr param is NULL! \n");
        return BT_PARAMINPUT_ERROR;
    }
    return esp_ble_gattc_write_char_descr(gattc_if, conn_id, handle, value_len, value, write_type, auth_req);
}
