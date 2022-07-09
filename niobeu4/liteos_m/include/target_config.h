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

#ifndef _TARGET_CONFIG_H
#define _TARGET_CONFIG_H

#include "los_compiler.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

int esp_rom_printf(const char *, ...);

#define LOSCFG_TASK_STRUCT_EXTENSION \
    UINT32 MPUSettings[1];           \
    UINT16 basePriority;             \
    void *LocalStoragePointer[1];    \
    void *LocalDelCallback[1]

void TaskDeleteExtensionHook(void *);

#define LOSCFG_TASK_DELETE_EXTENSION_HOOK(taskCB) TaskDeleteExtensionHook(taskCB)
#define LOSCFG_STACK_POINT_ALIGN_SIZE 16
#define OS_HWI_WITH_ARG 1
#define OS_TICK_INT_NUM 6
/*=============================================================================
                                        System clock module configuration
=============================================================================*/
#define OS_SYS_CLOCK (100UL)                          // 160000000
#define LOSCFG_BASE_CORE_TICK_PER_SECOND OS_SYS_CLOCK // (100UL)
#define LOSCFG_BASE_CORE_TICK_HW_TIME 0
#define LOSCFG_BASE_CORE_TICK_WTIMER 1
#define LOSCFG_BASE_CORE_TICK_RESPONSE_MAX OS_SYS_CLOCK // 1600000

/*=============================================================================
                                        Hardware interrupt module configuration
=============================================================================*/
#define LOSCFG_PLATFORM_HWI 1
#define OS_TICK_INT_NUM 6
#define LOSCFG_PLATFORM_HWI_LIMIT 32
/*=============================================================================
                                       Task module configuration
=============================================================================*/
#define LOSCFG_BASE_CORE_TSK_LIMIT 24
#define LOSCFG_BASE_CORE_TSK_IDLE_STACK_SIZE (0x800U)
#define LOSCFG_BASE_CORE_TSK_DEFAULT_STACK_SIZE (0x1000U)
#define LOSCFG_BASE_CORE_TSK_MIN_STACK_SIZE (256U)
#define LOSCFG_BASE_CORE_TIMESLICE 1
#define LOSCFG_BASE_CORE_TIMESLICE_TIMEOUT 20000

/*=============================================================================
                                       Semaphore module configuration
=============================================================================*/
#define LOSCFG_BASE_IPC_SEM 1
#define LOSCFG_BASE_IPC_SEM_LIMIT 48

/*=============================================================================
                                       Mutex module configuration
=============================================================================*/
#define LOSCFG_BASE_IPC_MUX 1
#define LOSCFG_BASE_IPC_MUX_LIMIT 24

/*=============================================================================
                                       Queue module configuration
=============================================================================*/
#define LOSCFG_BASE_IPC_QUEUE 1
#define LOSCFG_BASE_IPC_QUEUE_LIMIT 24

/*=============================================================================
                                       Software timer module configuration
=============================================================================*/
#define LOSCFG_BASE_CORE_SWTMR 1
#define LOSCFG_BASE_CORE_SWTMR_ALIGN 1
#define LOSCFG_BASE_CORE_SWTMR_LIMIT 48

/*=============================================================================
                                       Memory module configuration
=============================================================================*/
#define LOSCFG_SYS_EXTERNAL_HEAP 1
#define LOSCFG_SYS_HEAP_ADDR m_aucSysMem0
#define LOSCFG_SYS_HEAP_SIZE 0x14000UL
#define LOSCFG_MEM_MUL_POOL 1
#define OS_SYS_MEM_NUM 8
#define LOSCFG_MEM_FREE_BY_TASKID 0
#define LOSCFG_BASE_MEM_NODE_INTEGRITY_CHECK 1
#define LOSCFG_MEM_LEAKCHECK 0
#define LOSCFG_MEMORY_BESTFIT 1
#define LOSCFG_KERNEL_MEM_SLAB 0
/*=============================================================================
                                        Exception module configuration
=============================================================================*/
#define LOSCFG_PLATFORM_EXC 0

/* =============================================================================
                                       printf module configuration
============================================================================= */
#define LOSCFG_KERNEL_PRINTF 1
#define LOSCFG_BACKTRACE_TYPE 0
#define LOSCFG_BACKTRACE_DEPTH 4
#define LOSCFG_DEBUG_HOOK 0
#define LOS_KERNEL_TEST_NOT_SMOKE 0

/*=============================================================================
                                       shell module configuration
=============================================================================*/
#define LOSCFG_USE_SHELL 1
#define PRINT_LEVEL LOG_DEBUG_LEVEL

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* _TARGET_CONFIG_H */
