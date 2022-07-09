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

#include "los_memory.h"
#include "ohos_mem_pool.h"

/**
 * @brief implement for ohos_mem_pool.h
 */
void *OhosMalloc(MemType type, uint32 size)
{
    return LOS_MemAlloc(OS_SYS_MEM_ADDR, size);
}

void OhosFree(void *ptr)
{
    LOS_MemFree(OS_SYS_MEM_ADDR, ptr);
}
