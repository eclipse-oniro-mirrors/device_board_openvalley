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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "hal_hota_board.h"
#include "hota_partition.h"

#define NOT_SUPPORT 4096
#define PATH_SEPARATE_LEN 2

typedef struct {
    uint8_t index;
    uint8_t init;
    UpdateMetaData metaData;
    ComponentTableInfo componentTableInfo;
    uint8_t pubKey[16];
} UpdateInfo_t;

static UpdateInfo_t UpdateInfo = {0};
/**
 * @brief OTA module initialization.
 *
 * @return OHOS_SUCCESS: Success,
 *         Others: Failure.
 */
int HotaHalInit(void)
{
    if (UpdateInfo.init != 0) {
        return OHOS_FAILURE;
    }
    UpdateInfo.init = 1;
    memset_s(&UpdateInfo, sizeof(UpdateInfo), 0, sizeof(UpdateInfo));
    UpdateInfo.componentTableInfo.componentName = "OpenValley";
    UpdateInfo.componentTableInfo.imgPath = "/";
    return OHOS_SUCCESS;
}

/**
 * @brief Release OTA module resource.
 *
 * @return OHOS_SUCCESS: Success,
 *         Others: Failure.
 */
int HotaHalDeInit(void)
{
    if (UpdateInfo.init == 0) {
        return OHOS_FAILURE;
    }
    UpdateInfo.init = 0;
    return OHOS_SUCCESS;
}

/**
 * @brief Release OTA module resource.
 *
 * @return OHOS_SUCCESS: Success,
 *         Others: Failure.
 */
int HotaHalGetUpdateIndex(unsigned int *index)
{
    if (index == NULL) {
        return OHOS_FAILURE;
    }
    *index = UpdateInfo.index;
    return OHOS_SUCCESS;
}

/**
 * @brief Write image to partition.
 *
 * @param partition [in] scan result, result array size must larger than WIFI_SCAN_AP_LIMIT.
 * @param buffer    [in] image buffer.
 * @param offset    [in] The buffer offset of file.
 * @param bufLen    [in] The Length of buffer.
 *
 * @return OHOS_SUCCESS: Success,
 *         Others: Failure.
 */
int HotaHalWrite(int partition, unsigned char *buffer, unsigned int offset, unsigned int bufLen)
{
    if ((buffer == NULL) || (offset == 0) || (bufLen == 0)) {
        return OHOS_FAILURE;
    }
    return OHOS_SUCCESS;
}

/**
 * @brief read image of partition.
 *
 * @param partition [in]  scan result, result array size must larger than WIFI_SCAN_AP_LIMIT.
 * @param offset    [in]  The buffer offset of file.
 * @param bufLen    [in]  The Length of buffer.
 * @param buffer    [out] image buffer.
 *
 * @return OHOS_SUCCESS: Success,
 *         Others: Failure.
 */
int HotaHalRead(int partition, unsigned int offset, unsigned int bufLen, unsigned char *buffer)
{
    if ((buffer == NULL) || (offset == 0) || (bufLen == 0)) {
        return OHOS_FAILURE;
    }
    return OHOS_SUCCESS;
}

/**
 * @brief Write Boot Settings in order to notify device upgrade success or enter Recovery Part.
 *
 * @return OHOS_SUCCESS: Success,
 *         Others: Failure.
 */
int HotaHalSetBootSettings(void)
{
    UpdateInfo.index ^= 1;
    return OHOS_SUCCESS;
}

/**
 * @brief Restart after upgrade finish or go bootloader to upgrade.
 *
 * @return OHOS_SUCCESS: Success,
 *         Others: Failure.
 */
int HotaHalRestart(void)
{
    void panic_restart(void);
    panic_restart();
    return OHOS_SUCCESS;
}

/**
 * @brief Get partition info.
 *
 * You need to call this funtion in Init function, you need partition info when upgrade. \n
 *
 * @return Returns <b>0</b> if the operation is successful; returns <b>-1</b> otherwise.
 *
 * @since 1.0
 * @version 1.0
 */
const ComponentTableInfo *HotaHalGetPartitionInfo();

/**
 * @brief Get public key.
 *
 * You need to call this funtion when verfiy sign data \n
 *
 * @param length Indicates  pubkey len.
 *
 * @return Returns <b>0</b> if the operation is successful; public key.
 *
 * @since 1.0
 * @version 1.0
 */
unsigned char *HotaHalGetPubKey(unsigned int *length)
{
    if (length == NULL) {
        return NULL;
    }
    *length = sizeof(UpdateInfo.pubKey);
    return UpdateInfo.pubKey;
}

/**
 * @brief get update ability.
 *
 * You need to call this function when update process init. \n
 *
 * @return Returns update abilty.
 *
 * @since 1.0
 * @version 1.0
 */
int HotaHalGetUpdateAbility(void)
{
    return NOT_SUPPORT;
}

/**
 * @brief get ota package update path.
 *
 * You need to call this function before update process. \n
 *
 * @param path Indicates where ota package you place.
 * @param len Indicates  path len.
 *
 * @return Returns <b>0</b> if the operation is successful; returns <b>-1</b> otherwise.
 *
 * @since 1.0
 * @version 1.0
 */
int HotaHalGetOtaPkgPath(char *path, int len)
{
    int ret;
    if ((path == NULL) || (len == 0)) {
        return OHOS_FAILURE;
    }
    ret = memcpy_s(path, len, "/", PATH_SEPARATE_LEN);
    if (ret != 0) {
        return OHOS_FAILURE;
    }
    return OHOS_SUCCESS;
}

/**
 * @brief get update metadata.
 *
 * You need to call this function when update process .\n
 *
 * @return Returns OtaStatus if the operation is successful; returns <b>-1</b> otherwise.
 *
 * @since 1.0
 * @version 1.0
 */
int HotaHalGetMetaData(UpdateMetaData *metaData)
{
    int ret;
    if (metaData == NULL) {
        return OHOS_FAILURE;
    }
    ret = memcpy_s(metaData, sizeof(UpdateMetaData), &UpdateInfo.metaData, sizeof(UpdateMetaData));
    if (ret != 0) {
        return OHOS_FAILURE;
    }
    return OHOS_SUCCESS;
}

/**
 * @brief set update metadata.
 *
 * You need to call this function when update process.\n
 *
 * @return Returns <b>0</b> if the operation is successful; returns <b>-1</b> otherwise.
 *
 * @since 1.0
 * @version 1.0
 */
int HotaHalSetMetaData(UpdateMetaData *metaData)
{
    int ret;
    if (metaData == NULL) {
        return OHOS_FAILURE;
    }
    ret = memcpy_s(&UpdateInfo.metaData, sizeof(UpdateMetaData), metaData, sizeof(UpdateMetaData));
    if (ret != 0) {
        return OHOS_FAILURE;
    }
    return OHOS_SUCCESS;
}

/**
 * @brief check whether pkgVersion is valid.
 *
 * You need to call this function before update process.\n
 *
 * @return Returns <b>1</b> if pkgVersion is valid compared to currentVersion; returns <b>0</b> otherwise.
 *
 * @since 1.0
 * @version 1.0
 */
int HotaHalCheckVersionValid(const char *currentVersion, const char *pkgVersion, unsigned int pkgVersionLength)
{
    if ((currentVersion == NULL) || (pkgVersion == NULL) || (pkgVersionLength == 0)) {
        return OHOS_FAILURE;
    }
    return 0;
}

/**
 * @brief Get partition info.
 *
 * You need to call this funtion in Init function, you need partition info when upgrade. \n
 *
 * @return Returns <b>0</b> if the operation is successful; returns <b>-1</b> otherwise.
 *
 * @since 1.0
 * @version 1.0
 */
const ComponentTableInfo *HotaHalGetPartitionInfo()
{
    return &UpdateInfo.componentTableInfo;
}