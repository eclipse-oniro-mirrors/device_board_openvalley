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
#include <unistd.h>
#include "los_compiler.h"
#include "los_task.h"
#include "los_debug.h"
#include "samgr_lite.h"
#include "ohos_init.h"
#include "ohos_types.h"
#include "nvs.h"
#include "cmsis_os2.h"

#include "hiview_def.h"
#include "hiview_output_log.h"

#define SYSTEM_INIT_TASK_PRIO (LOSCFG_BASE_CORE_TSK_DEFAULT_PRIO + 1)
#define SYSTEM_INIT_TASK_STACK_SIZE (LOSCFG_BASE_CORE_TSK_DEFAULT_STACK_SIZE * 2)
#define DEALY_10_TICKS 10
#define MIN_STACK_SIZE 2048

void* __attribute__((weak)) OHOS_APP_FUNC_ENTRY = NULL;
void* __attribute__((weak)) BEFORE_OHOS_RUN_FUNC_ENTRY = NULL;

int __attribute__((weak)) DeviceManagerStart(void)
{
    return 0;
}

int __attribute__((weak)) DeviceWifiStart(void)
{
    return 0;
}

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

__attribute__((weak)) UINT32 lwip_ifconfig(INT32 argc, const CHAR** argv)
{
    return 0;
}

__attribute__((weak)) UINT32 OsShellPing(INT32 argc, const CHAR** argv)
{
    return 0;
}

void TaskCreateExtensionHook(VOID* taskCB)
{
    LosTaskCB* p = (LosTaskCB*)taskCB;
    UINTPTR topOfStack;
    if (!p) {
        return;
    }
    if ((p->stackSize <= 0) || (p->stackSize >= MIN_STACK_SIZE)) {
        return;
    }
    p->stackSize = MIN_STACK_SIZE;
    topOfStack = (UINTPTR)LOS_MemAllocAlign(OS_TASK_STACK_ADDR, p->stackSize,
        LOSCFG_STACK_POINT_ALIGN_SIZE);
    p->stackPointer = ArchTskStackInit(p->taskID, p->stackSize, (VOID*)topOfStack);
    LOS_MemFree(OS_TASK_STACK_ADDR, (void*)p->topOfStack);
    p->topOfStack = topOfStack;
}
#else
void TaskCreateExtensionHook(VOID* taskCB)
{
    (void)taskCB;
}
#endif

VOID* OHOS_SystemInitEntry(VOID)
{
    int err;
    printf("Code Build Time:%s %s\n", __DATE__, __TIME__);
    before_ohos_run();
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        ret = nvs_flash_init();
    }
    DeviceWifiStart();
    init_trace_system();
    err = DeviceManagerStart();
    if (err) {
        printf("DeviceManagerStart.ret=0x%X\n", err);
    }
    OHOS_SystemInit();
    ohos_app_main();
    return NULL;
}

void app_main(void)
{
    UINT32 ret;
    UINT32 taskID;
    TSK_INIT_PARAM_S stTask = { 0 };
    stTask.pfnTaskEntry = (TSK_ENTRY_FUNC)OHOS_SystemInitEntry;
    stTask.uwStackSize = SYSTEM_INIT_TASK_STACK_SIZE;
    stTask.pcName = "OHOS";
    stTask.usTaskPrio = SYSTEM_INIT_TASK_PRIO;
    ret = LOS_TaskCreate(&taskID, &stTask);
    if (ret != LOS_OK) {
        printf("OHOS_SystemInitEntry create failed!\n");
    }
    LOS_Start();
}