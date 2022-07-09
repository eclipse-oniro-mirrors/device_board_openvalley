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

#include "iot_errno.h"
#include "iot_flash.h"

#include "esp_err.h"
#include "esp_spi_flash.h"

#define FLASH_WAIT_FOREVER 0xFFFFFFFF
#define FLASH_SECTOR_SIZE (4096)

static uint32_t ESPErrToHoErr(esp_err_t ret)
{
    if (ret == ESP_OK) {
        return IOT_SUCCESS;
    } else {
        return IOT_FAILURE;
    }
}

unsigned int IoTFlashRead(unsigned int flashOffset, unsigned int size, unsigned char *ramData)
{
    if (size == 0 || ramData == NULL) {
        return IOT_FAILURE;
    }

    esp_err_t ret = spi_flash_read(flashOffset, ramData, size);
    return ESPErrToHoErr(ret);
}

unsigned int IoTFlashWrite(unsigned int flashOffset, unsigned int size,
                           const unsigned char *ramData, unsigned char doErase)
{
    if (size == 0 || ramData == NULL) {
        return IOT_FAILURE;
    }

    esp_err_t ret = ESP_OK;
    if (doErase == true) {
        uint32_t startEraseAddr = (flashOffset / FLASH_SECTOR_SIZE) * FLASH_SECTOR_SIZE;
        uint32_t endEraseAddr = ((flashOffset + size) / FLASH_SECTOR_SIZE + 1) * FLASH_SECTOR_SIZE - 1;
        uint32_t eraseSecNum = (endEraseAddr - startEraseAddr + 1) / FLASH_SECTOR_SIZE;
        uint32_t startBackupSize = flashOffset % FLASH_SECTOR_SIZE;
        uint32_t lastBackupSize = endEraseAddr - (flashOffset + size);

        uint8_t *backupBuffer = (uint8_t *)malloc(startBackupSize + lastBackupSize);
        if (backupBuffer == NULL) {
            return IOT_FAILURE;
        }

        ret = spi_flash_read(startEraseAddr, backupBuffer, startBackupSize);
        if (ret != ESP_OK) {
            free(backupBuffer);
            return IOT_FAILURE;
        }

        ret = spi_flash_read(flashOffset + size + 1, backupBuffer + startBackupSize, lastBackupSize);
        if (ret != ESP_OK) {
            free(backupBuffer);
            return IOT_FAILURE;
        }

        ret = spi_flash_erase_range(startEraseAddr, eraseSecNum * FLASH_SECTOR_SIZE);
        if (ret != ESP_OK) {
            free(backupBuffer);
            return IOT_FAILURE;
        }

        ret = spi_flash_write(startEraseAddr, backupBuffer, startBackupSize);
        if (ret != ESP_OK) {
            free(backupBuffer);
            return IOT_FAILURE;
        }

        ret = spi_flash_write(flashOffset + size + 1, backupBuffer + startBackupSize, lastBackupSize);
        if (ret != ESP_OK) {
            free(backupBuffer);
            return IOT_FAILURE;
        }

        free(backupBuffer);
    }

    ret = spi_flash_write(flashOffset, ramData, size);
    return ESPErrToHoErr(ret);
}

unsigned int IoTFlashErase(unsigned int flashOffset, unsigned int size)
{
    if (size == 0) {
        return IOT_FAILURE;
    }

    esp_err_t ret = spi_flash_erase_range(flashOffset, size);
    return ESPErrToHoErr(ret);
}

unsigned int IoTFlashInit(void)
{
    return IOT_SUCCESS;
}

unsigned int IoTFlashDeinit(void)
{
    return IOT_SUCCESS;
}
