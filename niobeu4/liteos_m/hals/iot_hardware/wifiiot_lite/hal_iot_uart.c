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
#include "iot_uart.h"

#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_err.h"

#ifndef portMAX_DELAY
#define portMAX_DELAY (TickType_t)0xffffffffUL
#endif

#define TXD_PIN (GPIO_NUM_4)
#define RXD_PIN (GPIO_NUM_5)

#define UART_ATTR_DEFAULT                                                \
    {                                                                    \
        115200, UART_DATA_8_BITS, UART_PARITY_DISABLE, UART_STOP_BITS_1, \
            UART_HW_FLOWCTRL_DISABLE, UART_SCLK_APB                      \
    }

typedef enum {
    ESP_UART_BLOCK_STATE_NONE_BLOCK = 1,
    ESP_UART_BLOCK_STATE_BLOCK,
} uart_block_state_e;

typedef enum {
    UART_STATE_NOT_OPENED = 0,
    UART_STATE_USEABLE
} uart_status_e;

typedef struct {
    uart_port_t num;
    uart_status_e uart_state;
    uart_block_state_e tx_block_state;
    uart_block_state_e rx_block_state;
    uart_config_t attr;
    uint8_t pad;
} uart_driver_data_t;

static uart_driver_data_t g_uart_0 = {
    .num = UART_NUM_0,
    .uart_state = UART_STATE_NOT_OPENED,
    .tx_block_state = ESP_UART_BLOCK_STATE_NONE_BLOCK,
    .rx_block_state = ESP_UART_BLOCK_STATE_NONE_BLOCK,
    .attr = UART_ATTR_DEFAULT,
    .pad = 0,
};

static uart_driver_data_t g_uart_1 = {
    .num = UART_NUM_1,
    .uart_state = UART_STATE_NOT_OPENED,
    .tx_block_state = ESP_UART_BLOCK_STATE_NONE_BLOCK,
    .rx_block_state = ESP_UART_BLOCK_STATE_NONE_BLOCK,
    .attr = UART_ATTR_DEFAULT,
    .pad = 0,
};

static uart_driver_data_t g_uart_2 = {
    .num = UART_NUM_2,
    .uart_state = UART_STATE_NOT_OPENED,
    .tx_block_state = ESP_UART_BLOCK_STATE_NONE_BLOCK,
    .rx_block_state = ESP_UART_BLOCK_STATE_NONE_BLOCK,
    .attr = UART_ATTR_DEFAULT,
    .pad = 0,
};

static const int RX_BUF_SIZE = 1024;
static uart_driver_data_t *g_uart[UART_NUM_MAX] = {&g_uart_0, &g_uart_1, &g_uart_2};

static uart_word_length_t HoDataBitsToESPDataBits(IotUartIdxDataBit DataBits)
{
    switch (DataBits) {
        case IOT_UART_DATA_BIT_5:
            return UART_DATA_5_BITS;
        case IOT_UART_DATA_BIT_6:
            return UART_DATA_6_BITS;
        case IOT_UART_DATA_BIT_7:
            return UART_DATA_7_BITS;
        case IOT_UART_DATA_BIT_8:
            return UART_DATA_8_BITS;
        default:
            return UART_DATA_BITS_MAX;
    }
}

static uart_parity_t HoParityToESParity(IotUartParity Parity)
{
    switch (Parity) {
        case IOT_UART_PARITY_NONE:
            return UART_PARITY_DISABLE;
        case IOT_UART_PARITY_ODD:
            return UART_PARITY_ODD;
        case IOT_UART_PARITY_EVEN:
            return UART_PARITY_EVEN;
        default:
            assert(0);
    }

    return UART_PARITY_DISABLE;
}

static uart_stop_bits_t HoStopBitsToESPStopBits(IotUartStopBit StopBits)
{
    switch (StopBits) {
        case IOT_UART_STOP_BIT_1:
            return UART_STOP_BITS_1;
        case IOT_UART_STOP_BIT_2:
            return UART_STOP_BITS_2;
        default:
            return UART_STOP_BITS_MAX;
    }
}

static uart_hw_flowcontrol_t HoflowCtrlToESPflowCtrl(IotFlowCtrl flowCtrl)
{
    switch (flowCtrl) {
        case IOT_FLOW_CTRL_NONE:
            return UART_HW_FLOWCTRL_DISABLE;
        case IOT_FLOW_CTRL_RTS_CTS:
            return UART_HW_FLOWCTRL_CTS_RTS;
        case IOT_FLOW_CTRL_RTS_ONLY:
            return UART_HW_FLOWCTRL_RTS;
        case IOT_FLOW_CTRL_CTS_ONLY:
            return UART_HW_FLOWCTRL_CTS;
        default:
            return UART_HW_FLOWCTRL_MAX;
    }
}

unsigned int IoTUartInit(unsigned int id, const IotUartAttribute *param)
{
    if (id > UART_NUM_MAX || param == NULL) {
        return IOT_FAILURE;
    }

    uart_driver_data_t *uart = g_uart[id];
    if (uart->uart_state == UART_STATE_USEABLE) {
        return IOT_FAILURE;
    }

    uart->attr.baud_rate = param->baudRate;
    uart->attr.data_bits = HoDataBitsToESPDataBits(param->dataBits);
    assert(uart->attr.data_bits != UART_DATA_BITS_MAX);
    uart->attr.parity = HoParityToESParity(param->parity);
    uart->attr.stop_bits = HoStopBitsToESPStopBits(param->stopBits);
    assert(uart->attr.stop_bits != UART_STOP_BITS_MAX);
    uart->pad = param->pad;

    if (IOT_UART_BLOCK_STATE_NONE_BLOCK == param->rxBlock) {
        uart->rx_block_state = ESP_UART_BLOCK_STATE_NONE_BLOCK;
    } else {
        uart->rx_block_state = ESP_UART_BLOCK_STATE_BLOCK;
    }

    if (IOT_UART_BLOCK_STATE_NONE_BLOCK == param->txBlock) {
        uart->tx_block_state = ESP_UART_BLOCK_STATE_NONE_BLOCK;
    } else {
        uart->tx_block_state = ESP_UART_BLOCK_STATE_BLOCK;
    }

    int ret = uart_driver_install(uart->num, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    ret += uart_param_config(uart->num, &(uart->attr));
    ret += uart_set_pin(uart->num, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (ret != ESP_OK) {
        return IOT_FAILURE;
    }

    uart->uart_state = UART_STATE_USEABLE;
    return IOT_SUCCESS;
}

int IoTUartRead(unsigned int id, unsigned char *data, unsigned int dataLen)
{
    if (id > UART_NUM_MAX) {
        return IOT_FAILURE;
    }

    uart_driver_data_t *uart = g_uart[id];
    if (data == NULL || dataLen == 0) {
        return IOT_FAILURE;
    }

    if (uart->uart_state == UART_STATE_NOT_OPENED) {
        return IOT_FAILURE;
    }

    uint8_t *rd_data = data;
    int data_received = 0;
    int data_remaining = dataLen;
    while (data_remaining) {
        if (uart->rx_block_state == ESP_UART_BLOCK_STATE_BLOCK) {
            data_received += uart_read_bytes(uart->num, (void *)(&rd_data[data_received]),
                                             data_remaining, (TickType_t)portMAX_DELAY);
        } else {
            data_received += uart_read_bytes(uart->num, (void *)(&rd_data[data_received]), data_remaining, 0);
        }

        if (data_received < 0) {
            return IOT_FAILURE;
        }

        data_remaining -= data_received;
    }

    return IOT_SUCCESS;
}

int IoTUartWrite(unsigned int id, const unsigned char *data, unsigned int dataLen)
{
    if (id > UART_NUM_MAX) {
        return IOT_FAILURE;
    }

    uart_driver_data_t *uart = g_uart[id];
    if (uart->uart_state == UART_STATE_NOT_OPENED) {
        return IOT_FAILURE;
    }

    int txBytes = uart_write_bytes(uart->num, (const char *)data, dataLen);
    if (uart->tx_block_state == ESP_UART_BLOCK_STATE_BLOCK) {
        int ret = uart_wait_tx_done(uart->num, (TickType_t)portMAX_DELAY);
        if (txBytes != dataLen || ret != ESP_OK) {
            return IOT_FAILURE;
        }
    }

    return IOT_SUCCESS;
}

unsigned int IoTUartDeinit(unsigned int id)
{
    if (id > UART_NUM_MAX) {
        return IOT_FAILURE;
    }

    uart_driver_data_t *uart = g_uart[id];
    if (uart->uart_state == UART_STATE_NOT_OPENED) {
        return IOT_FAILURE;
    }

    return uart_driver_delete(uart->num);
}

unsigned int IoTUartSetFlowCtrl(unsigned int id, IotFlowCtrl flowCtrl)
{
    if (id > UART_NUM_MAX) {
        return IOT_FAILURE;
    }

    uart_driver_data_t *uart = g_uart[id];
    uart->attr.flow_ctrl = HoflowCtrlToESPflowCtrl(flowCtrl);
    assert(uart->attr.flow_ctrl != UART_HW_FLOWCTRL_MAX);
    return uart_param_config(uart->num, &(uart->attr));
}
