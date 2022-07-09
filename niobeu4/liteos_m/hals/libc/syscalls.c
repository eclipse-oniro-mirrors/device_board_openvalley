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
#include <unistd.h>
#include "cmsis_os2.h"
#include "los_compiler.h"
#include "los_debug.h"
#include "los_task.h"
#include "nvs.h"
#include "ohos_init.h"
#include "ohos_types.h"
#include "pthread.h"
#include "samgr_lite.h"
#include "stdio.h"

#define DELAY_1S 1000
#define DELAY_10S (DELAY_1S * 10)

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wattribute-alias"
static int s_raise(int sig)
{
    if (SIGABRT == sig) {
        LOS_TaskDelete(LOS_CurTaskIDGet());
        while (1) {
            LOS_Msleep(DELAY_1S);
        }
    }
    return 0;
}

int raise(int sig) __attribute__((alias("s_raise")));

static int s_raise_r(struct _reent *r, int sig)
{
    return 0;
}
int _raise_r(struct _reent *r, int sig) __attribute__((alias("s_raise_r")));

#pragma GCC diagnostic pop

int _open_r(struct _reent *r, const char *path, int flags, int mode)
{
    return _open(path, flags, mode);
}
int _close_r(struct _reent *r, int fd)
{
    return _close(fd);
}
off_t _lseek_r(struct _reent *r, int fd, off_t size, int mode)
{
    return _lseek(fd, size, mode);
}

int _link_r(struct _reent *r, const char *n1, const char *n2)
{
    return link(n1, n2);
}
int _unlink_r(struct _reent *r, const char *path)
{
    return _unlink(path);
}
int _stat_r(struct _reent *r, const char *path, struct stat *st)
{
    return _stat(path, st);
}

int _rename_r(struct _reent *r, const char *src, const char *dst)
{
    return rename(src, dst);
}

int _fstat_r(struct _reent *r, int fd, struct stat *st)
{
    return _fstat(fd, st);
}

int _getpid_r(struct _reent *r)
{
    return LOS_CurTaskIDGet();
}

int _kill_r(struct _reent *r, int pid, int sig)
{
    return LOS_TaskDelete(pid);
}

void *_sbrk_r(struct _reent *r, ptrdiff_t sz)
{
    char *name;
    UINT32 taskId = LOS_CurTaskIDGet();
    name = LOS_TaskNameGet(taskId);
    esp_rom_printf("\e[0;36msbrk.taskName=%s taskId=%d\e[0m\r\n", name ? name : "NULL", taskId);
    LOS_TaskDelete(taskId);
    LOS_TaskSuspend(taskId);
    while (1) {
        LOS_TaskDelay(DELAY_10S);
    }
}
