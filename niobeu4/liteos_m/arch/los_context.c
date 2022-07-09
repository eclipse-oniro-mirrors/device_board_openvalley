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

#include "los_context.h"
#include "los_arch_context.h"
#include "los_arch_interrupt.h"
#include "los_arch_regs.h"
#include "los_arch_timer.h"
#include "los_debug.h"
#include "los_interrupt.h"
#include "los_sched.h"
#include "los_task.h"
#include "los_timer.h"
#include "securec.h"

UINT64 OsTickCount = 0;

LITE_OS_SEC_TEXT_INIT VOID ArchInit(VOID)
{
}

static VOID ArchSysTickLock()
{
}

static VOID ArchSysTickUnlock()
{
}

LITE_OS_SEC_TEXT_MINOR VOID ArchSysExit(VOID)
{
    PRINTK("ArchSysExit");
    LOS_IntLock();
    while (1) {
        ;
    }
}

LITE_OS_SEC_TEXT_INIT VOID *ArchTskStackInit(UINT32 taskID, UINT32 stackSize, VOID *topStack)
{
    vPortStoreTaskSettings((void *)topStack, stackSize, taskID);
    return (void *)pxPortInitialiseStack((uint8_t *)topStack + stackSize - 1,
                                         ((void (*)(void *))OsTaskEntry), (void *)taskID, 0);
}

LITE_OS_SEC_TEXT_INIT VOID *ArchSignalContextInit(VOID *stackPointer, VOID *stackTop,
                                                  UINTPTR sigHandler, UINT32 param)
{
    (VOID) stackTop;
    (VOID) sigHandler;
    (VOID) param;
    return (void *)stackPointer;
}

void xPortSysTickHandler(void)
{
    OsTickCount++;
    OsTickHandler();
}

static UINT32 ArchTickStart(void *handler)
{
    OsTickCount = 0;
    return LOS_OK;
}

static UINT64 ArchGetTickCycle(UINT32 *period)
{
    return (UINT64)OsTickCount * (OS_SYS_CLOCK / LOSCFG_BASE_CORE_TICK_PER_SECOND);
}

UINT32 ArchStartSchedule(VOID)
{
    ArchIntLock();
    OsSchedStart();
    xPortStartScheduler();
    return LOS_OK;
}

VOID ArchTaskSchedule(VOID)
{
    if (OS_INT_ACTIVE) {
        _frxt_setup_switch();
        return;
    }
    vPortYield();
    return;
}

static VOID ArchSysTickReload(UINT64 nextResponseTime)
{
}

UINT32 ArchEnterSleep(VOID)
{
    __asm__ volatile("dsync\n waiti 0"
                     :
                     :
                     : "memory");
    return LOS_OK;
}

STATIC ArchTickTimer g_archTickTimer = {
    .freq = OS_SYS_CLOCK,
    .irqNum = OS_TICK_INT_NUM,
    .init = ArchTickStart,
    .getCycle = ArchGetTickCycle,
    .reload = ArchSysTickReload,
    .lock = ArchSysTickLock,
    .unlock = ArchSysTickUnlock,
    .tickHandler = NULL,
};

ArchTickTimer *ArchSysTickTimerGet(VOID)
{
    return &g_archTickTimer;
}
