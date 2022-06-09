/*
 * Copyright (c) 2022 OpenValley Digital Co., Ltd.
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
#include <dirent.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include "fcntl.h"
#include "lfs.h"
#include "los_compiler.h"
#include "los_memory.h"
#include "los_task.h"
#include "ohos_init.h"
#include "ohos_types.h"

static const char *LITTLEFS_MOUNT_POINT = "/Talkweb";
static const char TAG[] = {"Littlefs"};
#define LFS_LOG printf
#define LSF_ERROR (-1)

#include "esp_partition.h"
/* ESP32的分区类型， 0x00-0x3F系统保留分区类型，0x40-0xFE自定义分区 */
#define LITTLEFS_PARTITION_TYPE ESP_PARTITION_TYPE_DATA
/* ESP32的子分区类型，0x00-0xFE，0xFF(ESP_PARTITION_SUBTYPE_ANY)所有子分区 */
#define LITTLEFS_PARTITION_SUBTYPE ESP_PARTITION_SUBTYPE_DATA_SPIFFS
/* ESP32的分区名称 */
#define LITTLEFS_PARTITION_NAME NULL

#define READ_SIZE 256     /* 最小读取字节数，所有的读取操作字节数必须是它的倍数（影响内存消耗） */
#define PROG_SIZE 256     /* 最小写入字节数，所有的写入操作字节数必须是它的倍数（影响内存消耗） */
#define BLOCK_SIZE 4096   /* 擦除块字节数，不会影响内存消耗，每个文件至少占用一个块，必须是READ_SIZE/PROG_SIZE的倍数 */
#define CACHE_SIZE 256    /* 块缓存的大小，缓存越大磁盘访问越小，性能越高，必须是READ_SIZE/PROG_SIZE的倍数，且是BLOCK_SIZE的因数 */
#define LOOKAHEAD_SIZE 16 /* 块分配预测深度，分配块时每次步进多少个块，必须为8的整数倍，对于内存消耗影响不大 */
#define BLOCK_CYCLES 16   /* 逐出元数据日志并将元数据移动到另一个块之前的擦除周期数，值越大性能越好，但磨损越不均匀，-1将禁用块级磨损均衡 */

#define BLOCK_SIZE_BIT_MOVE 10

static lfs_size_t calc_lfs_size(lfs_size_t block_count, lfs_size_t block_size)
{
    return ((block_count)*(block_size))>>BLOCK_SIZE_BIT_MOVE;
}

const char *GetLittlefsMountPoint(void)
{
    return LITTLEFS_MOUNT_POINT;
}

/* lfs读接口 */
static int LittlefsRead(const struct lfs_config *cfg, lfs_block_t block,
                        lfs_off_t off, char *buffer, lfs_size_t size)
{
    return spi_flash_read((size_t)cfg->context + cfg->block_size * block + off, buffer, size);
}

/* lfs写接口 */
static int LittlefsProg(const struct lfs_config *cfg, lfs_block_t block,
                        lfs_off_t off, const char *buffer, lfs_size_t size)
{
    return spi_flash_write((size_t)cfg->context + cfg->block_size * block + off, buffer, size);
}

/* lfs擦除接口 */
static int LittlefsErase(const struct lfs_config *cfg, lfs_block_t block)
{
    return spi_flash_erase_range((size_t)cfg->context + cfg->block_size * block, cfg->block_size);
}

/* lfs同步接口 */
static int LittlefsSync(const struct lfs_config *cfg)
{
    return LFS_ERR_OK;
}

/* 接口兼容 */
int __attribute__((weak)) SetDefaultMountPath(int pathNameIndex, const char *target)
{
    return LOS_OK;
}

/* lfs初始化 */
static INT32 LittlefsInit(void)
{
    int err = 0;
    static struct lfs_config s_lfsConfig = {0};   /* lfs配置变量，必须是全局内存或静态内存 */
    s_lfsConfig.read = LittlefsRead;   /* lfs 读接口 */
    s_lfsConfig.prog = LittlefsProg;   /* lfs 写接口 */
    s_lfsConfig.erase = LittlefsErase; /* lfs 擦除接口 */
    s_lfsConfig.sync = LittlefsSync;   /* lfs 同步接口 */
#if defined(LITTLEFS_PHYS_ADDR) && defined(BLOCK_COUNT)
    s_lfsConfig.context = (void *)LITTLEFS_PHYS_ADDR;
    s_lfsConfig.block_count = BLOCK_COUNT,
#else
    const esp_partition_t *part;
    part = esp_partition_find_first(LITTLEFS_PARTITION_TYPE, LITTLEFS_PARTITION_SUBTYPE, LITTLEFS_PARTITION_NAME);
    if (!part) {
        LFS_LOG("Error %s.esp_partition_find_first\n", TAG);
        return LSF_ERROR;
    }
    s_lfsConfig.context = (void *)part->address;
    s_lfsConfig.block_count = part->size / BLOCK_SIZE;
#endif
    s_lfsConfig.read_size = READ_SIZE;
    s_lfsConfig.prog_size = PROG_SIZE;
    s_lfsConfig.block_size = BLOCK_SIZE;
    s_lfsConfig.cache_size = CACHE_SIZE;
    s_lfsConfig.lookahead_size = LOOKAHEAD_SIZE;
    s_lfsConfig.block_cycles = BLOCK_CYCLES;
    s_lfsConfig.read_buffer = NULL;
    s_lfsConfig.prog_buffer = NULL;
    s_lfsConfig.lookahead_buffer = NULL;
    s_lfsConfig.name_max = 0;
    s_lfsConfig.file_max = 0;
    s_lfsConfig.attr_max = 0;
    err = SetDefaultMountPath(0, LITTLEFS_MOUNT_POINT); /* 设置缺省挂载点 */
    if (err != LOS_OK) {
        LFS_LOG("Error %s.SetDefaultMountPath=0x%X\n", TAG, err);
        return LSF_ERROR;
    }
    err = mount(NULL, LITTLEFS_MOUNT_POINT, "littlefs", 0, &s_lfsConfig); /* 设置挂载Littlefs */
    if (err != LOS_OK) {
        LFS_LOG("Error %s.mount=0x%X\n", TAG, err);
        return LSF_ERROR;
    }
    LFS_LOG("%s.mount=%s addr=0x%X size=%dK OK!!!\n", TAG, LITTLEFS_MOUNT_POINT, (size_t)s_lfsConfig.context,
            calc_lfs_size(s_lfsConfig.block_count, s_lfsConfig.block_size));
    return 0;
}

SYS_SERVICE_INIT(LittlefsInit);
