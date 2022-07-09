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

#include <securec.h>

#include "iot_errno.h"
#include "iot_pwm.h"

#include "driver/ledc.h"
#include "esp_err.h"

#define CLK_40M (40000000)
#define DUTY_MIN (0)
#define DUTY_MAX (100)
#define DUTY_RES_MIN (128)
#define LEDC_OUTPUT_IO (5)
#define LEDC_TIMER LEDC_TIMER_0

#define NUM2 2
#define DUTY_RESOLUTION_MAX (NUM2 << ((LEDC_TIMER_BIT_MAX) - 1))
#if SOC_LEDC_SUPPORT_HS_MODE
#define LEDC_MODE LEDC_HIGH_SPEED_MODE
#else
#define LEDC_MODE LEDC_LOW_SPEED_MODE
#endif

typedef enum {
    PWM_UNINIT = 0,
    PWM_INIT = 1
} pwm_status_e;

typedef struct {
    pwm_status_e pwm_state;
    ledc_channel_config_t pwm_attr;
} pwm_driver_data_t;

static pwm_driver_data_t g_pwm[LEDC_CHANNEL_MAX] = {0};

static unsigned int PwmDutyCalc(uint8_t TimerBit, uint32_t duty)
{
    return ((((NUM2 >> TimerBit) - 1) * duty) / DUTY_MAX);
}

static void InitPwm(ledc_channel_t num, ledc_channel_config_t *pwm_conf)
{
    assert(num < LEDC_CHANNEL_MAX);
    assert(pwm_conf != NULL);
    pwm_conf->speed_mode = LEDC_MODE;
    pwm_conf->channel = num;
    pwm_conf->timer_sel = LEDC_TIMER;
    pwm_conf->intr_type = LEDC_INTR_DISABLE;
    pwm_conf->gpio_num = LEDC_OUTPUT_IO;
    pwm_conf->duty = 0;
    pwm_conf->hpoint = 0;
}

static uint32_t ESPErrToHoErr(esp_err_t ret)
{
    if (ret == ESP_OK) {
        return IOT_SUCCESS;
    } else {
        return IOT_FAILURE;
    }
}

unsigned int IoTPwmInit(unsigned int port)
{
    if (port >= LEDC_CHANNEL_MAX) {
        return IOT_FAILURE;
    }

    pwm_driver_data_t *pwm = &g_pwm[port];
    if (pwm->pwm_state == PWM_INIT) {
        return IOT_FAILURE;
    }

    pwm->pwm_state = PWM_INIT;
    InitPwm((ledc_channel_t)port, &(pwm->pwm_attr));
    return IOT_SUCCESS;
}

unsigned int IoTPwmDeinit(unsigned int port)
{
    if (port >= LEDC_CHANNEL_MAX) {
        return IOT_FAILURE;
    }

    pwm_driver_data_t *pwm = &g_pwm[port];
    if (pwm->pwm_state == PWM_UNINIT) {
        return IOT_FAILURE;
    }

    memset_s(pwm, sizeof(pwm_driver_data_t), 0, sizeof(pwm_driver_data_t));
    return IOT_SUCCESS;
}

unsigned int IoTPwmStart(unsigned int port, unsigned short duty, unsigned int freq)
{
    if (port >= LEDC_CHANNEL_MAX) {
        return IOT_FAILURE;
    }

    pwm_driver_data_t *pwm = &g_pwm[port];
    if (pwm->pwm_state == PWM_UNINIT) {
        return IOT_FAILURE;
    }

    if ((freq == 0) || (duty >= DUTY_MAX) || (duty == DUTY_MIN)) {
        return IOT_FAILURE;
    }

    uint32_t DutyResolution = CLK_40M / freq;
    if (DutyResolution < DUTY_RES_MIN || DutyResolution > DUTY_RESOLUTION_MAX) {
        return IOT_FAILURE;
    }

    uint8_t TimerBit = 0;
    while (DutyResolution) {
        DutyResolution = DutyResolution >> 1;
        TimerBit++;
    }

    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_MODE,
        .timer_num = LEDC_TIMER,
        .duty_resolution = (TimerBit - 1),
        .freq_hz = freq,
        .clk_cfg = LEDC_AUTO_CLK};

    esp_err_t ret = ledc_timer_config(&ledc_timer);
    if (ret != ESP_OK) {
        return IOT_FAILURE;
    }

    ret = ledc_channel_config(&(pwm->pwm_attr));
    if (ret != ESP_OK) {
        return IOT_FAILURE;
    }

    uint32_t PwmDuty = PwmDutyCalc(TimerBit, duty);
    ret = ledc_set_duty(LEDC_MODE, port, PwmDuty);
    if (ret != ESP_OK) {
        return IOT_FAILURE;
    }

    ret = ledc_update_duty(LEDC_MODE, port);
    return ESPErrToHoErr(ret);
}

unsigned int IoTPwmStop(unsigned int port)
{
    if (port >= LEDC_CHANNEL_MAX) {
        return IOT_FAILURE;
    }

    pwm_driver_data_t *pwm = &g_pwm[port];
    if (pwm->pwm_state == PWM_UNINIT) {
        return IOT_FAILURE;
    }

    esp_err_t ret = ledc_stop(LEDC_MODE, port, 0);
    return ESPErrToHoErr(ret);
}
