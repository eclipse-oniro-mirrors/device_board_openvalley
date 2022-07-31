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
#include <stdio.h>
#include "los_task.h"

#define DEALY_10_TICKS 10
#define NUM_2048 2048

void *__attribute__((weak)) OHOS_APP_FUNC_ENTRY = NULL;
void *__attribute__((weak)) BEFORE_OHOS_RUN_FUNC_ENTRY = NULL;

void ohos_app_main()
{
    LOS_TaskDelay(DEALY_10_TICKS);
    if (OHOS_APP_FUNC_ENTRY) {
        printf("\n\033[1;32m<--------------- OHOS Application Start Here --------------->\033[0m\n");
        ((void (*)(void))OHOS_APP_FUNC_ENTRY)();
    } else {
        printf("\n\033[1;31m<--------------- OHOS_APP_FUNC_ENTRY is NULL --------------->\033[0m\n");
    }
    while (1) {
        LOS_TaskDelay(DEALY_10_TICKS * DEALY_10_TICKS);
        IoTWatchDogKick();
    }
}

void before_ohos_run()
{
    if (BEFORE_OHOS_RUN_FUNC_ENTRY) {
        ((void (*)(void))BEFORE_OHOS_RUN_FUNC_ENTRY)();
    }
}

#ifdef CONFIG_BUILD_TYPE_TEST

__attribute__((weak)) UINT32 lwip_ifconfig(INT32 argc, const CHAR **argv)
{
    return 0;
}

__attribute__((weak)) UINT32 OsShellPing(INT32 argc, const CHAR **argv)
{
    return 0;
}

void TaskCreateExtensionHook(VOID *taskCB)
{
    LosTaskCB *p = (LosTaskCB *)taskCB;
    UINTPTR topOfStack;
    if (!p) {
        return;
    }
    if ((p->stackSize <= 0) || (p->stackSize >= NUM_2048)) {
        return;
    }
    p->stackSize = NUM_2048;
    topOfStack = (UINTPTR)LOS_MemAllocAlign(OS_TASK_STACK_ADDR, p->stackSize,
        LOSCFG_STACK_POINT_ALIGN_SIZE);
    p->stackPointer = ArchTskStackInit(p->taskID, p->stackSize, (VOID *)topOfStack);
    LOS_MemFree(OS_TASK_STACK_ADDR, (void *)p->topOfStack);
    p->topOfStack = topOfStack;
}
#else
void TaskCreateExtensionHook(VOID *taskCB)
{
    (void)taskCB;
}
#endif
