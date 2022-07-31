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
#include <stdarg.h>
#include <unistd.h>
#include "cmsis_os2.h"
#include "los_compiler.h"
#include "los_debug.h"
#include "los_task.h"
#include "los_sched.h"
#include "nvs.h"
#include "ohos_init.h"
#include "ohos_types.h"
#include "samgr_lite.h"
#include "stdio.h"
#include "hal/uart_ll.h"
#include "hiview_def.h"
#include "hiview_output_log.h"

#define NUM_2 2
#define SAFE_OFFSET 4
#define BUFF_MAX_LEN 512

static void s_vprintf(const char *fmt, va_list ap)
{
    int len;
    static char buf[NUM_2][BUFF_MAX_LEN];
    static char volatile bufLock = 0;
    char *pbuf;
    if (xPortInterruptedFromISRContext()) {
        pbuf = buf[1];
    } else {
        pbuf = buf[0];
        while (bufLock) {
            if (g_taskScheduled) {
                LOS_TaskDelay(0);
            }
        }
        bufLock = (char)TRUE;
    }
    len = vsnprintf_s(pbuf, BUFF_MAX_LEN - SAFE_OFFSET, BUFF_MAX_LEN - SAFE_OFFSET, fmt, ap);
    if (len > 0) {
        uint16_t fill_len;
        for (fill_len = uart_ll_get_txfifo_len(&UART0); fill_len < len;) {
            if (fill_len) {
                uart_ll_write_txfifo(&UART0, (uint8_t *)pbuf, fill_len);
                len -= fill_len;
                pbuf += fill_len;
            }
            if (bufLock && g_taskScheduled) {
                LOS_TaskDelay(0);
            }
            fill_len = uart_ll_get_txfifo_len(&UART0);
        }
        if (len > 0)
            uart_ll_write_txfifo(&UART0, (uint8_t *)pbuf, len);
    }
    if (!xPortInterruptedFromISRContext()) {
        bufLock = (char)FALSE;
    }
}

// Liteos_m的打印
int printf(const char *__restrict __format, ...)
{
    va_list ap;
    va_start(ap, __format);
    s_vprintf(__format, ap);
    va_end(ap);
    return 0;
}

int hal_trace_printf(int level, const char *fmt, ...)
{
    if (level <= PRINT_LEVEL) {
        va_list ap;
        va_start(ap, fmt);
        s_vprintf(fmt, ap);
        va_end(ap);
    }
    return 0;
}

bool HilogProc_Impl(const HiLogContent *hilogContent, uint32_t len)
{
    char tempOutStr[LOG_FMT_MAX_LEN];
    tempOutStr[0] = 0, tempOutStr[1] = 0;
    if (LogContentFmt(tempOutStr, sizeof(tempOutStr), hilogContent) > 0) {
        printf("%s", tempOutStr);
    }
    return true;
}

int HiLogWriteInternal(const char *buffer, size_t bufLen)
{
    if (!buffer) {
        return -1;
    }
    if (bufLen < NUM_2) {
        return 0;
    }
    if (buffer[bufLen - NUM_2] != '\n') {
        *((char *)buffer + bufLen - 1) = '\n';
    }
    printf("%s\n", buffer);
    return 0;
}

int init_trace_system(void)
{
    int ret = 1;
    HiviewRegisterHilogProc(HilogProc_Impl);
    return ret;
}
