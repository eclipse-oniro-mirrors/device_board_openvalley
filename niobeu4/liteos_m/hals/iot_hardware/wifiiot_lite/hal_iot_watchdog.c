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

#include "los_task.h"

#include "esp_task_wdt.h"

#define TWDT_TIMEOUT_DEFAULT 3

typedef enum {
    WATCHDOG_DISABLED = 0,
    WATCHDOG_ENABLED = 1
} watchdog_status_e;

#ifdef CONFIG_ESP_TASK_WDT
static watchdog_status_e g_watchdogStatus = WATCHDOG_ENABLED;
#else
static watchdog_status_e g_watchdogStatus = WATCHDOG_DISABLED;
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void IoTWatchDogEnable(void)
{
    if (g_watchdogStatus == WATCHDOG_ENABLED) {
        return;
    }

    esp_err_t ret = ESP_OK;
#ifdef CONFIG_ESP_TASK_WDT_PANIC
    ret = esp_task_wdt_init(CONFIG_ESP_TASK_WDT_TIMEOUT_S, true);
#elif CONFIG_ESP_TASK_WDT
    ret = esp_task_wdt_init(CONFIG_ESP_TASK_WDT_TIMEOUT_S, false);
#endif
    if (ret != ESP_OK) {
        return;
    }

    if (g_idleTaskID) {
        ret = esp_task_wdt_add(g_idleTaskID);
        if (ret == ESP_OK) {
            g_watchdogStatus = WATCHDOG_ENABLED;
        } else {
            esp_task_wdt_deinit();
        }
    }
}

void IoTWatchDogKick(void)
{
    if (g_watchdogStatus == WATCHDOG_DISABLED) {
        return;
    }

    esp_task_wdt_reset();
}

void IoTWatchDogDisable(void)
{
    if (g_watchdogStatus == WATCHDOG_DISABLED) {
        return;
    }

    esp_err_t ret = esp_task_wdt_delete(g_idleTaskID);
    if (ret != ESP_OK) {
        return;
    }

    ret = esp_task_wdt_deinit();
    if (ret == ESP_OK) {
        g_watchdogStatus = WATCHDOG_DISABLED;
    }
}
