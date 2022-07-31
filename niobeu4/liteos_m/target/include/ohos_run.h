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

#ifndef __OHOS_RUN_H__
#define __OHOS_RUN_H__

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief 用与指定系统运行之前的程序入口函数
 *
 */
#define BEFORE_OHOS_RUN(func) void *BEFORE_OHOS_RUN_FUNC_ENTRY = (func)

/**
 * @brief 用与指定系统运行之后的程序入口函数
 *
 */
#define OHOS_APP_RUN(func) void *OHOS_APP_FUNC_ENTRY = (func)

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __OHOS_RUN_H__ */