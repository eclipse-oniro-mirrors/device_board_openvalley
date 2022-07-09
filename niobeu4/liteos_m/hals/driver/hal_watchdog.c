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

#define RWDT_PROTECT_KEY 0x50d83aa1
#define RWDT_PROTECT 0x3ff480a4
#define RWDT_CFG0 0x3ff4808c
#define RWDT_CFG0_VALUE ((1 << 10) | (7 << 11) | (3 << 28) | (1 << 31))
#define RWDT_CFG1 0x3ff48090
#define RWDT_FEED 0x3ff480a0
#define RWDT_TICKS_TYPE 0x3ff48070
#define RWDT_WOG_VALUE (1 << 31)
#define RWDT_IS_RUN_BIT (1 << 31)
#define OFFSET_30   30
#define BIT_3   3
#define SLOW_CK_TICKS (150 * 1000)
#define XTAL_32K_TICKS (32768)
#define CK8M_D256_OUT_CK_TICKS (8 * 1000 * 1000 / 256)

static uint32_t GetRwdtTicksType(void)
{
    volatile uint32_t *ptr = RWDT_TICKS_TYPE;
    return (((*ptr) >> OFFSET_30) & BIT_3);
}

static void RWDT_WOG(void)
{
    *(volatile uint32_t *)RWDT_FEED = RWDT_WOG_VALUE;
}

static uint32_t RWDT_IS_RUN(void)
{
    return (*(volatile uint32_t *)RWDT_CFG0) & RWDT_IS_RUN_BIT;
}

void WdtEnable(void)
{
    uint32_t ticks;
    if (RWDT_IS_RUN()) {
        return;
    }

    switch (GetRwdtTicksType()) {
        case 1:
            ticks = XTAL_32K_TICKS;
            break; // XTAL_32K
        case 2:
            ticks = CK8M_D256_OUT_CK_TICKS;
            break; // CK8M_D256_OUT
        default:
            ticks = SLOW_CK_TICKS;
            break; // SLOW_CK
    }

    *(volatile uint32_t *)RWDT_PROTECT = RWDT_PROTECT_KEY;
    RWDT_WOG();
    *(volatile uint32_t *)RWDT_CFG0 = RWDT_CFG0_VALUE;
    *(volatile uint32_t *)RWDT_CFG1 = 5 * ticks; // 最大超时时间 4G / (150K 或 32768 或 8M/256)
    *(volatile uint32_t *)RWDT_PROTECT = 0;
}

void WdtDisable(void)
{
    if (!RWDT_IS_RUN()) {
        return;
    }
    *(volatile uint32_t *)RWDT_PROTECT = RWDT_PROTECT_KEY;
    RWDT_WOG();
    *(volatile uint32_t *)RWDT_CFG0 = 0;
    *(volatile uint32_t *)RWDT_PROTECT = 0;
}

void WdtFeed(void)
{
    *(volatile uint32_t *)RWDT_PROTECT = RWDT_PROTECT_KEY;
    RWDT_WOG();
    *(volatile uint32_t *)RWDT_PROTECT = 0;
}

void IoTWatchDogEnable(void)
{
    WdtEnable();
}

void IoTWatchDogKick(void)
{
    WdtFeed();
}

void IoTWatchDogDisable(void)
{
    WdtDisable();
}
