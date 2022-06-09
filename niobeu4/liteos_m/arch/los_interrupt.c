/*
 * Copyright (c) 2013-2019 Huawei Technologies Co., Ltd. All rights reserved.
 * Copyright (c) 2020-2021 Huawei Device Co., Ltd. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 *    conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 *    of conditions and the following disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used
 *    to endorse or promote products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <stdarg.h>
#include <stdint.h>
#include "los_interrupt.h"
#include "los_arch_context.h"
#include "los_arch_interrupt.h"
#include "los_arch_regs.h"
#include "los_context.h"
#include "los_debug.h"
#include "los_hook.h"
#include "los_membox.h"
#include "los_memory.h"
#include "los_sched.h"
#include "los_task.h"
#include "securec.h"

#define IRAM_ATTR __attribute__((section(".iram1.__COUNTER__")))

static const char DefVectorName[] = {"LiteosVector"};

#if (OS_HWI_WITH_ARG == 1)
VOID OsSetVector(UINT32 num, HWI_PROC_FUNC vector, VOID *arg)
{
    xt_set_interrupt_handler(num, (void (*)(void *))vector, arg ? arg : DefVectorName);
}
#else
VOID OsSetVector(UINT32 num, HWI_PROC_FUNC vector)
{
}
#endif

INLINE IRAM_ATTR UINT32 HwiNumValid(UINT32 num)
{
    return (num >= OS_SYS_VECTOR_CNT) && (num <= OS_VECTOR_CNT);
}

IRAM_ATTR UINT32 HalIrqUnmask(HWI_HANDLE_T hwiNum)
{
    if (!HwiNumValid(hwiNum)) {
        return OS_ERRNO_HWI_NUM_INVALID;
    }
    xt_ints_on(1 << hwiNum);
    return LOS_OK;
}

IRAM_ATTR UINT32 HalIrqMask(HWI_HANDLE_T hwiNum)
{
    if (!HwiNumValid(hwiNum)) {
        return OS_ERRNO_HWI_NUM_INVALID;
    }
    xt_ints_off(1 << hwiNum);
    return LOS_OK;
}