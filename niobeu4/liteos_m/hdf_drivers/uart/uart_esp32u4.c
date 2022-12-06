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

#include <stdlib.h>
#include <string.h>
#include "esp_err.h"
#include "uart.h"
#include "gpio_types.h"
#include "uart_types.h"
#include "uart_if.h"
#include "uart_core.h"
#include "osal_sem.h"
#include "hcs_macro.h"
#include "hdf_config_macro.h"
#include "hdf_log.h"

#ifdef GPIO_NUM_MAX
#undef GPIO_NUM_MAX
#endif

#define HDF_UART_TMO 1000
#define HDF_LOG_TAG uartDev
#define GPIO_MAX_LENGTH 32
#define UART_FIFO_MAX_BUFFER 2048
#define UART_DMA_RING_BUFFER_SIZE 256 // mast be 2^n
#define MAX_DEV_NAME_SIZE 32

#define UART_PIN_NUNS 4 // 串口引脚配置个数
#define MAX_UART_NUMS 3 // 最大串口个数
#define UART_RX_BUFF_SIZE 512
#define UART_TX_BUFF_SIZE 512
#define OH_ESP_DATABITS_EX(data_bits) (3 - (data_bits))
#define OH2ESP_STOPBITS(oh_stop_bits) (1 + (oh_stop_bits))
#define ESP2OH_STOPBITS(esp_stop_bits) ((esp_stop_bits)-1)

#define PLATFORM_CONFIG HCS_NODE(HCS_ROOT, platform)
#define PLATFORM_UART_CONFIG HCS_NODE(HCS_NODE(HCS_ROOT, platform), uart_config)
#define HDF_GPIO_FIND_SOURCE(node, uartHost, device)                                                 \
    do {                                                                                             \
        if (strcmp(HCS_PROP(node, match_attr), (device)->deviceMatchAttr) == 0) {                    \
            int cur_uart_gpio_pin[] = HCS_ARRAYS(HCS_PROP(node, uart_gpio_pin));                     \
            struct UartAttrSourceStr cur_uart_attr = HCS_ARRAYS(HCS_PROP(node, uart_attr));          \
            UartAttr *uart_config = (UartAttr *)OsalMemAlloc(sizeof(UartAttr));                      \
            if (uart_config == NULL) {                                                               \
                HDF_LOGE("%s: OsalMemCalloc uart_config error", __func__);                           \
                return HDF_ERR_MALLOC_FAIL;                                                          \
            }                                                                                        \
            uart_config->uart_port = HCS_PROP(node, uart_port);                                      \
            uart_config->baudrate = HCS_PROP(node, baudrate);                                        \
            uart_config->uart_pin[0] = cur_uart_gpio_pin[0];                                         \
            uart_config->uart_pin[1] = cur_uart_gpio_pin[1];                                         \
            uart_config->uart_pin[2] = cur_uart_gpio_pin[2];                                         \
            uart_config->uart_pin[3] = cur_uart_gpio_pin[3];                                         \
            uart_config->data_bits = GetWordLengthFromStr(cur_uart_attr.data_bits);                  \
            uart_config->parity = GetParityFromStr(cur_uart_attr.parity);                            \
            uart_config->stop_bits = GetStopBitsFromStr(cur_uart_attr.stop_bits);                    \
            uart_config->hw_flowcontrol = GetHwFlowControlFromStr(cur_uart_attr.hw_flowcontrol);     \
            HDF_LOGE("----- UART%d Config -----", uart_config->uart_port);                           \
            HDF_LOGE("baudrate = %d", uart_config->baudrate);                                        \
            HDF_LOGE("Uart_Pin = [%d,%d,%d,%d]", uart_config->uart_pin[0], uart_config->uart_pin[1], \
                     uart_config->uart_pin[2], uart_config->uart_pin[3]);                            \
            HDF_LOGE("data_bits = %s[%d]", cur_uart_attr.data_bits, uart_config->data_bits);         \
            HDF_LOGE("parity = %s[%d]", cur_uart_attr.parity, uart_config->parity);                  \
            HDF_LOGE("stop_bits = %s[%d]", cur_uart_attr.stop_bits, uart_config->stop_bits);         \
            HDF_LOGE("hw_flowcontrol = %s[%d]", cur_uart_attr.hw_flowcontrol,                        \
                     uart_config->hw_flowcontrol);                                                   \
            uart_config->block_time = 0;                                                             \
            (uartHost)->priv = uart_config;                                                          \
            (uartHost)->num = uart_config->uart_port;                                                \
        }                                                                                            \
    } while (0)

static int32_t UartDriverBind(struct HdfDeviceObject *device);
static int32_t UartDriverInit(struct HdfDeviceObject *device);
static void UartDriverRelease(struct HdfDeviceObject *device);

struct HdfDriverEntry g_UartDriverEntry = {
    .moduleVersion = 1,
    .moduleName = "ESP32U4_HDF_UART",
    .Bind = UartDriverBind,
    .Init = UartDriverInit,
    .Release = UartDriverRelease,
};

HDF_INIT(g_UartDriverEntry);

static int32_t UartHostDevInit(struct UartHost *host);
static int32_t UartHostDevDeinit(struct UartHost *host);
static int32_t UartHostDevWrite(struct UartHost *host, uint8_t *data, uint32_t size);
static int32_t UartHostDevSetBaud(struct UartHost *host, uint32_t baudRate);
static int32_t UartHostDevGetBaud(struct UartHost *host, uint32_t *baudRate);
static int32_t UartHostDevRead(struct UartHost *host, uint8_t *data, uint32_t size);
static int32_t UartHostDevSetAttribute(struct UartHost *host, struct UartAttribute *attribute);
static int32_t UartHostDevGetAttribute(struct UartHost *host, struct UartAttribute *attribute);
static int32_t UartHostDevSetTransMode(struct UartHost *host, enum UartTransMode mode);

struct UartHostMethod g_uartHostMethod = {
    .Init = UartHostDevInit,
    .Deinit = UartHostDevDeinit,
    .Read = UartHostDevRead,
    .Write = UartHostDevWrite,
    .SetBaud = UartHostDevSetBaud,
    .GetBaud = UartHostDevGetBaud,
    .SetAttribute = UartHostDevSetAttribute,
    .GetAttribute = UartHostDevGetAttribute,
    .SetTransMode = UartHostDevSetTransMode,
};

struct UartAttrSourceStr {
    char *data_bits;
    char *parity;
    char *stop_bits;
    char *hw_flowcontrol;
};

typedef struct _UartAttr {
    int uart_port;
    int baudrate;
    int uart_pin[UART_PIN_NUNS];
    int data_bits;
    int parity;
    int stop_bits;
    int hw_flowcontrol;
    unsigned int block_time;
} UartAttr;

static int GetWordLengthFromStr(const char *str)
{
    if (!strcmp(str, "UART_DATA_5_BITS")) {
        return UART_DATA_5_BITS;
    } else if (!strcmp(str, "UART_DATA_6_BITS")) {
        return UART_DATA_6_BITS;
    } else if (!strcmp(str, "UART_DATA_7_BITS")) {
        return UART_DATA_7_BITS;
    }
    return UART_DATA_8_BITS;
}

static int GetParityFromStr(const char *str)
{
    if (!strcmp(str, "UART_PARITY_EVEN")) {
        return UART_PARITY_EVEN;
    } else if (!strcmp(str, "UART_PARITY_ODD")) {
        return UART_PARITY_ODD;
    }
    return UART_PARITY_DISABLE;
}

static int GetStopBitsFromStr(const char *str)
{
    if (!strcmp(str, "UART_STOP_BITS_1_5")) {
        return UART_STOP_BITS_1_5;
    } else if (!strcmp(str, "UART_STOP_BITS_2")) {
        return UART_STOP_BITS_2;
    }
    return UART_STOP_BITS_1;
}

static int GetHwFlowControlFromStr(const char *str)
{
    if (!strcmp(str, "UART_HW_FLOWCTRL_RTS")) {
        return UART_HW_FLOWCTRL_RTS;
    } else if (!strcmp(str, "UART_HW_FLOWCTRL_CTS")) {
        return UART_HW_FLOWCTRL_CTS;
    } else if (!strcmp(str, "UART_HW_FLOWCTRL_CTS_RTS")) {
        return UART_HW_FLOWCTRL_CTS_RTS;
    }
    return UART_HW_FLOWCTRL_DISABLE;
}

static int32_t AttachUartDevice(struct UartHost *uartHost, struct HdfDeviceObject *device)
{
    int32_t ret;
    if (device == NULL || uartHost == NULL) {
        HDF_LOGE("%s: uartHost or device is NULL", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    HCS_FOREACH_CHILD_VARGS(PLATFORM_UART_CONFIG, HDF_GPIO_FIND_SOURCE, uartHost, device);

    return HDF_SUCCESS;
}

static int32_t UartDriverBind(struct HdfDeviceObject *device)
{
    if (device == NULL) {
        HDF_LOGE("%s: invalid parameter", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    struct UartHost *devService = UartHostCreate(device);
    if (devService == NULL) {
        HDF_LOGE("%s: UartHostCreate fail!", __func__);
        return HDF_FAILURE;
    }
    HDF_LOGI("%s: UartHostCreate success!", __func__);
    return HDF_SUCCESS;
}

static void UartDriverRelease(struct HdfDeviceObject *device)
{
    struct UartHost *host = NULL;
    if (device == NULL) {
        HDF_LOGE("%s: device is NULL!", __func__);
        return;
    }

    host = UartHostFromDevice(device);
    if (host == NULL) {
        HDF_LOGE("%s: host is null", __func__);
        return;
    }

    if (host->priv != NULL) {
        OsalMemFree(host->priv);
        host->priv = NULL;
    }
}

static int32_t UartDriverInit(struct HdfDeviceObject *device)
{
    HDF_LOGI("Enter %s:", __func__);
    int32_t ret;
    struct UartHost *host = NULL;

    if (device == NULL) {
        HDF_LOGE("%s: device is NULL", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }

    host = UartHostFromDevice(device);
    if (host == NULL) {
        HDF_LOGE("%s: host is NULL", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }

    ret = AttachUartDevice(host, device);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: attach error", __func__);
        return HDF_FAILURE;
    }
    host->method = &g_uartHostMethod;
    return ret;
}

#define DEFAULT_RX_FLOW_CTRL_THRESH 122
#define INDEX_2 2
#define INDEX_3 3
/* UartHostMethod implementations */
static int32_t UartHostDevInit(struct UartHost *host)
{
    int ret;
    if (host == NULL) {
        HDF_LOGE("%s: invalid parameter", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    UartAttr *uart_config = (UartAttr *)host->priv;
    if (uart_config == NULL) {
        HDF_LOGE("%s: parse uart config fail", __func__);
        return HDF_FAILURE;
    }
    uart_config_t uartConfig = {0};
    uartConfig.baud_rate = uart_config->baudrate;
    uartConfig.data_bits = uart_config->data_bits;
    uartConfig.parity = uart_config->parity;
    uartConfig.stop_bits = uart_config->stop_bits;
    uartConfig.flow_ctrl = uart_config->hw_flowcontrol;
    if (0x01 & uartConfig.flow_ctrl) {
        uartConfig.rx_flow_ctrl_thresh = DEFAULT_RX_FLOW_CTRL_THRESH;
    }
    uart_param_config(uart_config->uart_port, &uartConfig);

    // 检查pin脚是否合法
    for (int i = 0; i < UART_PIN_NUNS; i++) {
        if (uart_config->uart_pin[i] >= GPIO_NUM_MAX || uart_config->uart_pin[i] < 0) {
            uart_config->uart_pin[i] = -1;
        }
    }

    ret = uart_set_pin(uart_config->uart_port, uart_config->uart_pin[0], uart_config->uart_pin[1],
                       uart_config->uart_pin[INDEX_2], uart_config->uart_pin[INDEX_3]);
    if (ret != 0) {
        HDF_LOGE("uart_set_pin failed ret=0x%X\n", ret);
        return HDF_FAILURE;
    }

    ret = uart_driver_install(uart_config->uart_port, UART_RX_BUFF_SIZE, UART_TX_BUFF_SIZE, 0, NULL, 0);
    if (ret != 0) {
        HDF_LOGE("uart_driver_install failed ret=0x%X\n", ret);
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static int32_t UartHostDevDeinit(struct UartHost *host)
{
    int ret = HDF_FAILURE;
    ret = uart_driver_delete(host->num);
    if (ret != ESP_OK) {
        HDF_LOGE("%s: uart%d_driver_delete failed\n", host->num, __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static int32_t UartHostDevWrite(struct UartHost *host, uint8_t *data, uint32_t size)
{
    int ret = HDF_FAILURE;
    ret = uart_write_bytes(host->num, (const uint8_t *)data, size);
    if (ret < 0) {
        HDF_LOGE("uart_write_bytes failed!");
        return HDF_FAILURE;
    }
    return ret;
}

static int32_t UartHostDevRead(struct UartHost *host, uint8_t *data, uint32_t size)
{
    int length = 0;
    int rcv_bytes = 0;
    if (host == NULL) {
        HDF_LOGE("UartHost is NULL!");
        return HDF_ERR_INVALID_PARAM;
    }

    UartAttr *uart_config = (UartAttr *)host->priv;
    if (uart_config == NULL) {
        HDF_LOGE("%s: parse uart config fail", __func__);
        return HDF_FAILURE;
    }
    int ret = uart_read_bytes(host->num, (uint8_t *)data, 1, uart_config->block_time);
    if (ret < 0) {
        HDF_LOGE("uart_read_bytes failed!");
        return HDF_FAILURE;
    }
    rcv_bytes += ret;
    uart_get_buffered_data_len(host->num, &length); // 获取buffer长度
    if (length > 0) {
        ret = uart_read_bytes(host->num, (uint8_t *)(&data[1]), size - 1, 0);
        if (ret < 0) {
            HDF_LOGE("uart_read_bytes failed!");
            return HDF_FAILURE;
        }
        rcv_bytes += ret;
    }
    return rcv_bytes;
}

static int32_t UartHostDevSetBaud(struct UartHost *host, uint32_t baudRate)
{
    if (host == NULL) {
        HDF_LOGE("UartHost is NULL!");
        return HDF_ERR_INVALID_PARAM;
    }

    UartAttr *uart_config = (UartAttr *)host->priv;
    if (uart_config == NULL) {
        HDF_LOGE("%s: parse uart config fail", __func__);
        return HDF_FAILURE;
    }
    uart_config->baudrate = baudRate;
    return HDF_SUCCESS;
}

static int32_t UartHostDevGetBaud(struct UartHost *host, uint32_t *baudRate)
{
    if (host == NULL) {
        HDF_LOGE("UartHost is NULL!");
        return HDF_ERR_INVALID_PARAM;
    }
    UartAttr *uart_config = (UartAttr *)host->priv;
    if (uart_config == NULL) {
        HDF_LOGE("%s: parse uart config fail", __func__);
        return HDF_FAILURE;
    }
    *baudRate = uart_config->baudrate;
    return HDF_SUCCESS;
}

static int32_t UartHostDevSetAttribute(struct UartHost *host, struct UartAttribute *attribute)
{
    if (host == NULL) {
        HDF_LOGE("UartHost is NULL!");
        return HDF_ERR_INVALID_PARAM;
    }
    UartAttr *uart_config = (UartAttr *)host->priv;
    if (uart_config == NULL) {
        HDF_LOGE("%s: parse uart config fail", __func__);
        return HDF_FAILURE;
    }

    uart_config->data_bits = OH_ESP_DATABITS_EX(attribute->dataBits);
    if (attribute->parity == UART_ATTR_PARITY_NONE) {
        uart_config->parity = UART_PARITY_DISABLE;
    } else if (attribute->parity == UART_ATTR_PARITY_EVEN) {
        uart_config->parity = UART_PARITY_EVEN;
    } else if (attribute->parity == UART_ATTR_PARITY_ODD) {
        uart_config->parity = UART_PARITY_ODD;
    } else {
        HDF_LOGE("%s: Not support set uart attribute->parity = %d", __func__, attribute->parity);
        return HDF_FAILURE;
    }
    uart_config->stop_bits = OH2ESP_STOPBITS(attribute->stopBits);
    return HDF_SUCCESS;
}

static int32_t UartHostDevGetAttribute(struct UartHost *host, struct UartAttribute *attribute)
{
    if (host == NULL) {
        HDF_LOGE("UartHost is NULL!");
        return HDF_ERR_INVALID_PARAM;
    }
    UartAttr *uart_config = (UartAttr *)host->priv;
    if (uart_config == NULL) {
        HDF_LOGE("%s: parse uart config fail", __func__);
        return HDF_FAILURE;
    }

    attribute->dataBits = OH_ESP_DATABITS_EX(uart_config->data_bits);

    if (uart_config->parity == UART_PARITY_EVEN) {
        attribute->parity = UART_ATTR_PARITY_EVEN;
    } else if (uart_config->parity == UART_PARITY_ODD) {
        attribute->parity = UART_ATTR_PARITY_ODD;
    } else {
        attribute->parity = UART_ATTR_PARITY_NONE;
    }
    attribute->stopBits = ESP2OH_STOPBITS(uart_config->stop_bits);
    return HDF_SUCCESS;
}

static int32_t UartHostDevSetTransMode(struct UartHost *host, enum UartTransMode mode)
{
    UartAttr *uart_config = (UartAttr *)host->priv;
    if (uart_config == NULL) {
        HDF_LOGE("%s: parse uart config fail", __func__);
        return HDF_FAILURE;
    }
    if (mode == UART_MODE_RD_BLOCK) {
        uart_config->block_time = 0xFFFFFFFF;
    } else if (mode == UART_MODE_RD_NONBLOCK) {
        uart_config->block_time = 0;
    } else {
        HDF_LOGE("%s: Not support mode = %d", __func__, mode);
    }
    return HDF_SUCCESS;
}