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
#include "ohos_run.h"
#include "blegap.h"
#include "bluetooth_device.h"
#define NULL 0L

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

BtError DisableBle(void)
{
    if (esp_bluedroid_get_status() == ESP_BLUEDROID_STATUS_ENABLED) {
        esp_bluedroid_disable();
        esp_bt_controller_disable();
    }
    return BT_SUCCESS;
}

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

BtError GetLocalAddr(unsigned char *mac, unsigned int len)
{
    int ret;
    if ((mac == NULL) || (len <= 0)) {
        BT_DEBUG("GetLocalAddr param is NULL! \n");
        return BT_PARAMINPUT_ERROR;
    }
    ret = memcpy_s(mac, len, esp_bt_dev_get_address(), len);
    if (!ret) {
        printf("memcpy_s fail!!\n");
        return ret;
    }
    return BT_SUCCESS;
}

BtError BleStartScan(void)
{
    if (esp_bluedroid_get_status() != ESP_BLUEDROID_STATUS_ENABLED) {
        return ESP_ERR_INVALID_STATE;
    }
    return esp_ble_gap_start_scanning(ScanTime);
}

BtError BleStopScan(void)
{
    if (esp_bluedroid_get_status() != ESP_BLUEDROID_STATUS_ENABLED) {
        return ESP_ERR_INVALID_STATE;
    }
    return esp_ble_gap_stop_scanning();
}

BtError BleGattcConnect(int clientId, VOID *func,
                        const BdAddr *bdAddr, bool isAutoConnect,
                        BtTransportType transport)
{
    if (bdAddr == NULL) {
        BT_DEBUG("BleGattcConnect param is NULL! \n");
        return BT_PARAMINPUT_ERROR;
    }
    return esp_ble_gattc_open(clientId, bdAddr, transport, isAutoConnect);
}

BtError BleGattcDisconnect(int clientId, int conn_id)
{
    return esp_ble_gattc_close(clientId, conn_id);
}

BtError BleGapDisconnect(BdAddr remote_device)
{
    uint8_t BdAddrs[OHOS_BD_ADDR_LEN];
    int ret = 0;
    ret = memcpy_s(BdAddrs, sizeof(BdAddrs), remote_device.addr, sizeof(remote_device.addr));
    if (!ret) {
        printf("memcpy_s fail!!\n");
        return ret;
    }
    return esp_ble_gap_disconnect(BdAddrs);
}

uint8_t *BleResolveAdvData(uint8_t *adv_data, uint8_t type, uint8_t *length)
{
    if ((adv_data == NULL) || (length <= 0)) {
        BT_DEBUG("BleResolveAdvData param is NULL! \n");
        return BT_PARAMINPUT_ERROR;
    }
    return esp_ble_resolve_adv_data(adv_data, type, length);
}

BtError BleGattcConfigureMtuSize(int mtuSize)
{
    return esp_ble_gatt_set_local_mtu(mtuSize);
}

BtError BleGattcRegister(BtGattClientCallbacks func)
{
    esp_err_t ret;
    ret = esp_ble_gap_register_callback(func.gap_callback);
    if (ret) {
        ESP_LOGE(GATTC_TAG, "%s gap register failed, error code = %x\n", __func__, ret);
        return ret;
    }

    ret = esp_ble_gattc_register_callback(func.gattc_callback);
    if (ret) {
        ESP_LOGE(GATTC_TAG, "%s gattc register failed, error code = %x\n", __func__, ret);
        return ret;
    }

    ret = esp_ble_gattc_app_register(func.PROFILE_APP_ID);
    if (ret) {
        ESP_LOGE(GATTC_TAG, "%s gattc app register failed, error code = %x\n", __func__, ret);
    }
    return ret;
}

BtError BleGattcUnRegister(int clientId)
{
    return esp_ble_gattc_app_unregister(clientId);
}

BtError BleGattcSearchServices(int clientId, int conn_id, BtUuid *filter_uuid)
{
    if (filter_uuid == NULL) {
        BT_DEBUG("BleResolveAdvData param is NULL! \n");
        return BT_PARAMINPUT_ERROR;
    }
    esp_bt_uuid_t remote_filter_service_uuid = {
        .len = filter_uuid->uuidLen,
        .uuid = {.uuid16 = (uint16_t)filter_uuid->uuid, },
    };
    esp_ble_gattc_search_service(clientId, conn_id, &remote_filter_service_uuid);
}

BtError BleGattcWriteCharacteristic(GattcWriteChar write_char, uint8_t *value,
                                    GattBleAuthReq auth_req)
{
    if ((value == NULL) || (write_char.value_len <= 0)) {
        BT_DEBUG("BleGattcWriteCharacteristic param is NULL! \n");
        return BT_PARAMINPUT_ERROR;
    }
    return esp_ble_gattc_write_char(write_char.gattc_if, write_char.conn_id,
                                    write_char.handle, write_char.value_len,
                                    value, write_char.write_type, auth_req);
}

BtError BleGatSetScanParams(BleScanParams *scan_params)
{
    if (scan_params == NULL) {
        BT_DEBUG("BleGatSetScanParams param is NULL! \n");
        return BT_PARAMINPUT_ERROR;
    }
    return esp_ble_gap_set_scan_params(scan_params);
}

BtError BleGattcSendMtuReq(GattInterfaceType gattc_if, uint16_t conn_id)
{
    return esp_ble_gattc_send_mtu_req(gattc_if, conn_id);
}

BtError BleGattcGetAttrCount(GattcGetAttr get_attr,
                             uint16_t char_handle,
                             uint16_t *count)
{
    if (count == NULL) {
        BT_DEBUG("BleGattcGetAttrCount param is NULL! \n");
        return BT_PARAMINPUT_ERROR;
    }
    return esp_ble_gattc_get_attr_count(get_attr.gattc_if, get_attr.conn_id,
                                        get_attr.type, get_attr.start_handle,
                                        get_attr.end_handle, char_handle, count);
}

GattStatus BleGattcGetCharByUuid(GattcGetChar get_char, BtUuids char_uuid,
                                 BleGattcCharElem *result, uint16_t *count)
{
    if ((result == NULL) || (count == NULL)) {
        BT_DEBUG("BleGattcGetCharByUuid param is NULL! \n");
        return BT_PARAMINPUT_ERROR;
    }
    return esp_ble_gattc_get_char_by_uuid(get_char.gattc_if, get_char.conn_id,
                                          get_char.start_handle, get_char.end_handle,
                                          char_uuid, result, count);
}

BtError BleGattcRegisterForNotify(GattInterfaceType gattc_if,
                                  BdAddr server_bda,
                                  uint16_t handle)
{
    uint8_t BdAddrs[OHOS_BD_ADDR_LEN];
    int ret = 0;
    ret = memcpy_s(BdAddrs, sizeof(BdAddrs), server_bda.addr, sizeof(server_bda.addr));
    if (!ret) {
        printf("memcpy_s fail!!\n");
        return ret;
    }
    return esp_ble_gattc_register_for_notify(gattc_if, BdAddrs, handle);
}

GattStatus BleGattcGetDescrByCharHandle(GattcGetDescr get_descr, BleGattcDescrElem *result,
                                        uint16_t *count)
{
    if ((result == NULL) || (count == NULL)) {
        BT_DEBUG("BleGattcGetCharByUuid param is NULL! \n");
        return BT_PARAMINPUT_ERROR;
    }
    return esp_ble_gattc_get_descr_by_char_handle(get_descr.gattc_if, get_descr.conn_id,
                                                  get_descr.char_handle, get_descr.descr_uuid,
                                                  result, count);
}

BtError BleGattcWriteCharDescr(GattcWriteChar write_char,
                               uint8_t *value,
                               GattAttributePermission auth_req)
{
    if (value == NULL) {
        BT_DEBUG("BleGattcWriteCharDescr param is NULL! \n");
        return BT_PARAMINPUT_ERROR;
    }
    return esp_ble_gattc_write_char_descr(write_char.gattc_if, write_char.conn_id,
                                          write_char.handle, write_char.value_len,
                                          value, write_char.write_type, auth_req);
}
