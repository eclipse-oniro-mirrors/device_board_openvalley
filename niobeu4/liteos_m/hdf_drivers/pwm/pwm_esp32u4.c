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
#include <stdio.h>
#include "hcs_macro.h"
#include "hdf_config_macro.h"
#include "hdf_device_desc.h"
#include "pwm_if.h"
#include "pwm_core.h"
#include "hdf_log.h"
#include "driver/mcpwm.h"
#include "driver/ledc.h"

#define PERIOD_NUM 1000000000
#define PERIOD2FREQUENCY(period) (PERIOD_NUM / (period))
#define MCPWM_GENERATOR(cmpr) ((cmpr) % 2)
#define NUM_8192 8192
#define GET_ESP_DUTY_CYCLE_PERCENT(duty, period) (int)(NUM_8192 * (1.0 * (duty) / (period)))

#define UNIT_MAX_OUTPUT 6 // 每个PWM单元最大输出的个数
#define CONFIG_2_PWM_NUM(unit, signal_port) (((unit)*UNIT_MAX_OUTPUT) + (signal_port))
#define PWM_NUM_2_CONFIG(pwm_num, unit, signal_port) \
    do {                                             \
        (unit) = (pwm_num) / UNIT_MAX_OUTPUT;        \
        (signal_port) = (pwm_num) % UNIT_MAX_OUTPUT; \
    } while (0)

typedef struct _PwmDeviceConfig {
    int channel;
    int pwm_timer;
    int freq_hz;
    int gpio_pin;
    ledc_timer_config_t ledc_timer_config;
    ledc_channel_config_t ledc_channel_config;
} PwmDeviceConfig;

static int32_t PwmDevSetConfig(struct PwmDev *pwm, struct PwmConfig *config);
static int32_t PwmDevOpen(struct PwmDev *pwm);
static int32_t PwmDevClose(struct PwmDev *pwm);

struct PwmMethod g_pwmmethod = {
    .setConfig = PwmDevSetConfig,
    .open = PwmDevOpen,
    .close = PwmDevClose,
};

#define PLATFORM_CONFIG HCS_NODE(HCS_ROOT, platform)
#define PLATFORM_PWM_CONFIG HCS_NODE(HCS_NODE(HCS_ROOT, platform), pwm_config)

#define PWM_FIND_CONFIG(node, name, resource)                                               \
    do {                                                                                    \
        if (strcmp(HCS_PROP(node, match_attr), name) == 0) {                                \
            (resource)->channel = GetChannelFromStr(HCS_PROP(node, channel));               \
            (resource)->pwm_timer = GetTimerFromStr(HCS_PROP(node, timer));                 \
            (resource)->freq_hz = HCS_PROP(node, freq_hz);                                  \
            (resource)->gpio_pin = GetPinIndexFromStr(HCS_PROP(node, gpio_pin));            \
            (resource)->ledc_timer_config.duty_resolution = LEDC_TIMER_13_BIT;              \
            (resource)->ledc_timer_config.clk_cfg = LEDC_AUTO_CLK;                          \
            (resource)->ledc_timer_config.timer_num = (resource)->pwm_timer;                \
            (resource)->ledc_timer_config.speed_mode = LEDC_LOW_SPEED_MODE;                 \
            (resource)->ledc_channel_config.channel = (resource)->channel;                  \
            (resource)->ledc_channel_config.gpio_num = (resource)->gpio_pin;                \
            (resource)->ledc_channel_config.speed_mode = LEDC_LOW_SPEED_MODE;               \
            (resource)->ledc_channel_config.hpoint = 0;                                     \
            (resource)->ledc_channel_config.timer_sel = (resource)->pwm_timer;              \
            HDF_LOGD("<---------- %s ------------>", name);                                 \
            HDF_LOGD("channel = %s [%d]", HCS_PROP(node, channel), (resource)->channel);    \
            HDF_LOGD("pwm_timer = %s [%d]", HCS_PROP(node, timer), (resource)->pwm_timer);  \
            HDF_LOGD("freq_hz = %dHz", (resource)->freq_hz);                                \
            HDF_LOGD("gpio_pin = %s [%d]", HCS_PROP(node, gpio_pin), (resource)->gpio_pin); \
            result = HDF_SUCCESS;                                                           \
        }                                                                                   \
    } while (0)

#define NUM_10 10

static int GetPinIndexFromStr(const char *str)
{
    if (str == NULL || strlen(str) < 10) {
        HDF_LOGE("pin index config is illegal!");
        return GPIO_NUM_NC;
    }

    char *ptr_nun = &str[9];
    int level = strlen(ptr_nun); // 获取下标位数
    int pin_index = 0;           // GPIO号
    for (int i = 0; i < level; i++) {
        if (i != 0) {
            pin_index = NUM_10 * pin_index;
        }
        int num = ptr_nun[i] - '0';
        if (num >= NUM_10) { // 字符非法
            HDF_LOGE("pin index config is illegal!");
            return GPIO_NUM_NC;
        }
        pin_index += num;
    }
    return pin_index;
}

static int GetTimerFromStr(const char *str)
{
    if (!strcmp(str, "LEDC_TIMER_0")) {
        return LEDC_TIMER_0;
    } else if (!strcmp(str, "LEDC_TIMER_1")) {
        return LEDC_TIMER_1;
    } else if (!strcmp(str, "LEDC_TIMER_2")) {
        return LEDC_TIMER_2;
    } else if (!strcmp(str, "LEDC_TIMER_3")) {
        return LEDC_TIMER_3;
    }
    HDF_LOGE("pwm_timer is illegal! must be LEDC_TIMER_0-LEDC_TIMER_03.");
    return LEDC_TIMER_MAX;
}

static int GetChannelFromStr(const char *str)
{
    if (!strcmp(str, "LEDC_CHANNEL_0")) {
        return LEDC_CHANNEL_0;
    } else if (!strcmp(str, "LEDC_CHANNEL_1")) {
        return LEDC_CHANNEL_1;
    } else if (!strcmp(str, "LEDC_CHANNEL_2")) {
        return LEDC_CHANNEL_2;
    } else if (!strcmp(str, "LEDC_CHANNEL_3")) {
        return LEDC_CHANNEL_3;
    } else if (!strcmp(str, "LEDC_CHANNEL_4")) {
        return LEDC_CHANNEL_4;
    } else if (!strcmp(str, "LEDC_CHANNEL_5")) {
        return LEDC_CHANNEL_5;
    } else if (!strcmp(str, "LEDC_CHANNEL_6")) {
        return LEDC_CHANNEL_6;
    } else if (!strcmp(str, "LEDC_CHANNEL_7")) {
        return LEDC_CHANNEL_7;
    }
    HDF_LOGE("channel is illegal! must be LEDC_CHANNEL 0-7.");
    return LEDC_CHANNEL_MAX;
}

static uint32_t GetPwmDeviceResource(PwmDeviceConfig *device, const char *deviceMatchAttr)
{
    int32_t result = HDF_FAILURE;
    if (device == NULL || deviceMatchAttr == NULL) {
        HDF_LOGE("%s: device or deviceMatchAttr is NULL", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    HCS_FOREACH_CHILD_VARGS(PLATFORM_PWM_CONFIG, PWM_FIND_CONFIG, deviceMatchAttr, device);

    if (result != HDF_SUCCESS) {
        HDF_LOGE("resourceNode %s is NULL\r\n", deviceMatchAttr);
    }

    return result;
}

static int32_t AttachPwmDevice(struct PwmDev *host, struct HdfDeviceObject *device)
{
    int32_t ret;
    PwmDeviceConfig *pwmDevice = NULL;
    if (device == NULL || host == NULL) {
        HDF_LOGE("%s: param is NULL\r\n", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    pwmDevice = (PwmDeviceConfig *)OsalMemAlloc(sizeof(PwmDeviceConfig));
    if (pwmDevice == NULL) {
        HDF_LOGE("%s: OsalMemAlloc pwmDevice error\r\n", __func__);
        return HDF_ERR_MALLOC_FAIL;
    }
    ret = GetPwmDeviceResource(pwmDevice, device->deviceMatchAttr);
    if (ret != HDF_SUCCESS) {
        (void)OsalMemFree(pwmDevice);
        return HDF_FAILURE;
    }

    PwmSetPriv(host, pwmDevice);
    host->num = pwmDevice->channel;

    return HDF_SUCCESS;
}

static int32_t PwmDriverBind(struct HdfDeviceObject *device);
static int32_t PwmDriverInit(struct HdfDeviceObject *device);
static void PwmDriverRelease(struct HdfDeviceObject *device);

struct HdfDriverEntry g_pwmDriverEntry = {
    .moduleVersion = 1,
    .moduleName = "ESP32U4_HDF_PLATFORM_PWM",
    .Bind = PwmDriverBind,
    .Init = PwmDriverInit,
    .Release = PwmDriverRelease,
};
HDF_INIT(g_pwmDriverEntry);

static int32_t PwmDriverBind(struct HdfDeviceObject *device)
{
    struct PwmDev *devService = NULL;
    if (device == NULL) {
        HDF_LOGE("hdfDevice object is null!\r\n");
        return HDF_FAILURE;
    }

    devService = (struct PwmDev *)OsalMemCalloc(sizeof(struct PwmDev));
    if (devService == NULL) {
        HDF_LOGE("malloc pwmDev failed\n");
    }
    device->service = &devService->service;
    devService->device = device;

    return HDF_SUCCESS;
}

static int32_t PwmDriverInit(struct HdfDeviceObject *device)
{
    int32_t ret;
    struct PwmDev *host = NULL;

    if (device == NULL) {
        HDF_LOGE("%s: device is NULL\r\n", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }

    host = (struct PwmDev *)device->service;
    if (host == NULL) {
        HDF_LOGE("%s: host is NULL\r\n", __func__);
        return HDF_ERR_MALLOC_FAIL;
    }

    ret = AttachPwmDevice(host, device);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s:attach error\r\n", __func__);
        return HDF_DEV_ERR_ATTACHDEV_FAIL;
    }

    host->method = &g_pwmmethod;
    ret = PwmDeviceAdd(device, host);
    if (ret != HDF_SUCCESS) {
        PwmDeviceRemove(device, host);
        OsalMemFree(host->device);
        OsalMemFree(host);
        return HDF_DEV_ERR_NO_DEVICE;
    }

    return HDF_SUCCESS;
}

static void PwmDriverRelease(struct HdfDeviceObject *device)
{
    struct PwmDev *host = NULL;
    if (device == NULL || device->service == NULL) {
        HDF_LOGE("device is null\r\n");
        return;
    }
    host = (struct PwmDev *)device->service;
    if (host != NULL && host->device != NULL) {
        host->method = NULL;
        OsalMemFree(host->device);
        OsalMemFree(host);
        host->device = NULL;
        host = NULL;
    }
    device->service = NULL;
    host = NULL;
    return;
}

static int32_t PwmDevSetConfig(struct PwmDev *pwm, struct PwmConfig *config)
{
    int ret;
    if (pwm == NULL || config == NULL) {
        HDF_LOGE("%s: PwmDev* pwm or struct PwmConfig *config is NULL!", __FUNCTION__);
        return HDF_ERR_INVALID_PARAM;
    }
    PwmDeviceConfig *pwmDevice = (PwmDeviceConfig *)PwmGetPriv(pwm);
    if (pwmDevice == NULL) {
        HDF_LOGE("%s: PwmDeviceConfig* pwmDevice is NULL", __FUNCTION__);
        return HDF_ERR_INVALID_PARAM;
    }
    if (config->status == PWM_ENABLE_STATUS) {                                          // 使能PWM
        if (pwmDevice->ledc_timer_config.freq_hz != PERIOD2FREQUENCY(config->period)) { // 周期有频率变化
            pwmDevice->ledc_timer_config.freq_hz = PERIOD2FREQUENCY(config->period);
            ledc_timer_config(&pwmDevice->ledc_timer_config); // 重新配置周期
            printf("\r\n------->config freq_hz = %d [period = %d]\r\n",
                   pwmDevice->ledc_timer_config.freq_hz, config->period);
            printf("speed_mode = %d\r\n", pwmDevice->ledc_timer_config.speed_mode);
            printf("timer_num = %d\r\n", pwmDevice->ledc_timer_config.timer_num);
            printf("clk_cfg = %d\r\n", pwmDevice->ledc_timer_config.clk_cfg);
            printf("duty_resolution = %d\r\n", pwmDevice->ledc_timer_config.duty_resolution);
        }
        if (pwmDevice->ledc_channel_config.duty != GET_ESP_DUTY_CYCLE_PERCENT(config->duty, config->period)) {
            pwmDevice->ledc_channel_config.duty = GET_ESP_DUTY_CYCLE_PERCENT(config->duty, config->period);
            printf("\r\n------->config ledc_duty = %d [duty=%d <--> period=%d]\r\n",
                   pwmDevice->ledc_channel_config.duty, config->duty, config->period);
            printf("gpio_num = %d\r\n", pwmDevice->ledc_channel_config.gpio_num);
            printf("speed_mode = %d\r\n", pwmDevice->ledc_channel_config.speed_mode);
            printf("channel = %d\r\n", pwmDevice->ledc_channel_config.channel);
            printf("timer_sel = %d\r\n", pwmDevice->ledc_channel_config.timer_sel);
            printf("hpoint = %d\r\n", pwmDevice->ledc_channel_config.hpoint);
        }
    } else if (config->status == PWM_DISABLE_STATUS) { // 失能PWM
        ledc_stop(pwmDevice->ledc_timer_config.speed_mode, pwmDevice->channel, config->polarity);
        printf("------->pwm output stop, set level = %d\r\n", config->polarity);
    }
    return HDF_SUCCESS;
}

static int32_t PwmDevOpen(struct PwmDev *pwm)
{
    if (pwm == NULL) {
        HDF_LOGE("%s: PwmDev* pwm is NULL", __FUNCTION__);
        return HDF_ERR_INVALID_PARAM;
    }

    PwmDeviceConfig *pwmDevice = (PwmDeviceConfig *)PwmGetPriv(pwm);
    if (pwmDevice == NULL) {
        HDF_LOGE("%s: PwmDeviceConfig* pwmDevice is NULL", __FUNCTION__);
        return HDF_ERR_INVALID_PARAM;
    }
    return HDF_SUCCESS;
}

static int32_t PwmDevClose(struct PwmDev *pwm)
{
    HDF_LOGI("——> Enter %s", __FUNCTION__);
    return HDF_SUCCESS;
}