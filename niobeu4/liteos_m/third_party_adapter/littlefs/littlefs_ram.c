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

#include <string.h>
#include <dirent.h>
#include <sys/mount.h>
#include "sys/stat.h"
#include "fcntl.h"
#include "littlefs.h"
#include "lfs.h"
#include "los_memory.h"
#include "los_task.h"
#include "los_compiler.h"
#include "los_fs.h"
#include "ohos_init.h"
#include "ohos_types.h"


const char LITTLEFS_MOUNT_POINT[] = { "/Openvalley" };
static const char TAG[] = { "Littlefs" };

#define LFS_LOG printf
#define RAM_BUF_SIZE   (22*1024)   /* 可以被擦除的块数量，实际大小=BLOCK_SIZE*BLOCK_COUNT字节 */
#define READ_SIZE      32    /* 最小读取字节数，所有的读取操作字节数必须是它的倍数（影响内存消耗） */
#define PROG_SIZE      READ_SIZE    /* 最小写入字节数，所有的写入操作字节数必须是它的倍数（影响内存消耗） */
#define BLOCK_SIZE     128  /* 擦除块字节数，不会影响内存消耗，每个文件至少占用一个块，必须是READ_SIZE/PROG_SIZE的倍数 */
#define CACHE_SIZE     READ_SIZE    /* 块缓存的大小，缓存越大磁盘访问越小，性能越高，必须是READ_SIZE/PROG_SIZE的倍数，且是BLOCK_SIZE的因数 */
#define LOOKAHEAD_SIZE 16    /* 块分配预测深度，分配块时每次步进多少个块，必须为8的整数倍，对于内存消耗影响不大 */
#define BLOCK_CYCLES   (-1)    /* 逐出元数据日志并将元数据移动到另一个块之前的擦除周期数，值越大性能越好，但磨损越不均匀，-1将禁用块级磨损均衡 */


/* lfs配置变量，必须是全局内存或静态内存 */
static char LittlefsRamBuf[RAM_BUF_SIZE];

/* lfs读接口 */
int littlefs_block_read(const struct lfs_config *cfg, lfs_block_t block,
                        lfs_off_t off, char *buffer, lfs_size_t size)
{
    int ret;
    off = cfg->block_size * block + off;
    if ((off + size) > RAM_BUF_SIZE) {
        return LOS_NOK;
    }
    ret = memcpy_s(buffer, size, (char*)cfg->context + off, size);
    if (ret != 0) {
        return LOS_NOK;
    }
    return LOS_OK;
}

/* lfs写接口 */
int littlefs_block_write(const struct lfs_config *cfg, lfs_block_t block,
                        lfs_off_t off, const void *buffer, lfs_size_t size)
{
    int ret;
    off = cfg->block_size * block + off;
    if ((off + size) > RAM_BUF_SIZE) {
        return LOS_NOK;
    }
    ret = memcpy_s((char*)cfg->context + off, size, buffer, size);
    if (ret != 0) {
        return LOS_NOK;
    }
    return LOS_OK;
}

/* lfs擦除接口 */
int littlefs_block_erase(const struct lfs_config *cfg, lfs_block_t block)
{
    lfs_off_t off = cfg->block_size * block;
    if ((off + cfg->block_size) > RAM_BUF_SIZE) {
        return LOS_NOK;
    }
    int ret = memset_s((char*)cfg->context + off, cfg->block_size, 0xFF, cfg->block_size);
    if (ret != 0) {
        return LOS_NOK;
    }
    return LOS_OK;
}

// /* lfs同步接口 */
int littlefs_block_sync(const struct lfs_config *c)
{
    (void)c;
    return 0;
}

const char *GetLittlefsMountPoint(void)
{
    return LITTLEFS_MOUNT_POINT;
}

static int littlefs_config(struct PartitionCfg *pCfg)
{
    int ret;
    pCfg->partNo = (void*)LittlefsRamBuf;
    ret = memset_s(LittlefsRamBuf, RAM_BUF_SIZE, 0xFF, RAM_BUF_SIZE);
    if (ret != 0) {
        return LOS_NOK;
    }
    pCfg->blockCount = RAM_BUF_SIZE / BLOCK_SIZE;
    pCfg->readSize = READ_SIZE;
    pCfg->writeSize = PROG_SIZE;
    pCfg->cacheSize = CACHE_SIZE;
    pCfg->blockCycles = BLOCK_CYCLES;
    pCfg->lookaheadSize = LOOKAHEAD_SIZE;
    pCfg->blockSize = BLOCK_SIZE;
    pCfg->readFunc = NULL;
    pCfg->writeFunc = NULL;
    pCfg->eraseFunc = NULL;
    return LOS_OK;
}
// /* lfs初始化 */
static INT32 LittlefsInit(void)
{
    int err, ret;
    DIR* dir;
    struct PartitionCfg cfg  = {0};
    if (littlefs_config(&cfg) == LOS_NOK) {
        return LOS_NOK;
    }
    /* 设置挂载Littlefs */
    err = mount(NULL, LITTLEFS_MOUNT_POINT, "littlefs", 0, &cfg);
    if (err != LOS_OK) {
        LFS_LOG("Error %s.mount=0x%X\n", TAG, err);
        return LOS_NOK;
    }
    mkdir(LITTLEFS_MOUNT_POINT, S_IRUSR | S_IWUSR);
    return LOS_OK;
}

SYS_FEATURE_INIT(LittlefsInit);
