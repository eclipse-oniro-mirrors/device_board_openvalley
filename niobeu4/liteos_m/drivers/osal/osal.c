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

#include <stdlib.h>
#include <string.h>
#include "esp_intr_alloc.h"
#include "device_resource_if.h"
#include "hdf_device_desc.h"
#include "hdf_io_service_if.h"
#include "osal.h"

#define LOG_TAG "esp_osal"
#define MAX_IRQ_ID  71
static intr_handle_data_t *OaslIntrHandle[MAX_IRQ_ID] = {NULL};
int32_t OsalRegisterIrq(uint32_t irqId, uint32_t config, OsalIRQHandle handle, const char *name, void *dev)
{
    // xt_set_interrupt_handler
    int ret;
    if (irqId >= MAX_IRQ_ID)
        return HDF_FAILURE;
    if (OaslIntrHandle[irqId])
        OsalUnregisterIrq(irqId, dev);
    ret = esp_intr_alloc(irqId, config, (intr_handler_t)handle, dev, &OaslIntrHandle[irqId]);
    return (ret == ESP_OK) ? HDF_SUCCESS : HDF_FAILURE;
}

int32_t OsalUnregisterIrq(uint32_t irqId, void *dev)
{
    intr_handle_data_t *handle;
    if (irqId > MAX_IRQ_ID)
        return HDF_FAILURE;
    handle = OaslIntrHandle[irqId];
    OaslIntrHandle[irqId] = NULL;
    if (!handle)
        return HDF_FAILURE;
    return (ESP_OK == esp_intr_free(handle)) ? HDF_SUCCESS : HDF_FAILURE;
}

int32_t OsalEnableIrq(uint32_t irqId)
{
    if (irqId > MAX_IRQ_ID)
        return HDF_FAILURE;
    if (!OaslIntrHandle[irqId])
        return HDF_FAILURE;
    return (ESP_OK == esp_intr_enable(OaslIntrHandle[irqId])) ? HDF_SUCCESS : HDF_FAILURE;
}

int32_t OsalDisableIrq(uint32_t irqId)
{
    if (irqId > MAX_IRQ_ID)
        return HDF_FAILURE;
    if (!OaslIntrHandle[irqId])
        return HDF_FAILURE;
    return (ESP_OK == esp_intr_disable(OaslIntrHandle[irqId])) ? HDF_SUCCESS : HDF_FAILURE;
}
