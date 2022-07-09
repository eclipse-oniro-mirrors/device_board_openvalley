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
#include "lowpower.h"

#include "esp_err.h"
#include "esp_sleep.h"

#define WAKE_TIME_DEFAULT 20

static uint32_t ESPErrToHoErr(esp_err_t ret)
{
    if (ret == ESP_OK) {
        return IOT_SUCCESS;
    } else {
        return IOT_FAILURE;
    }
}

unsigned int LpcInit(void)
{
    return IOT_SUCCESS;
}

unsigned int LpcSetType(LpcType type)
{
    esp_err_t ret = esp_sleep_enable_timer_wakeup(WAKE_TIME_DEFAULT * 1000000);
    if (ret != ESP_OK) {
        return IOT_FAILURE;
    }

    if (type == LIGHT_SLEEP) {
        ret = esp_light_sleep_start();
        return ESPErrToHoErr(ret);
    } else if (type == DEEP_SLEEP) {
        esp_deep_sleep_start();
    }

    return IOT_SUCCESS;
}
