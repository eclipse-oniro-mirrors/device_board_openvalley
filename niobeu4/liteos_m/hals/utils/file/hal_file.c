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

#include "fcntl.h"
#include "lfs.h"
#include "littlefs.h"
#include "securec.h"
#include "sys/stat.h"
#include "utils_file.h"

#define OFFSET_FD 8

static int sHalFileGetPath(char *tmpPath, const char *path)
{
    char *ptr_path = path;
    if (!path || !tmpPath) {
        return -1;
    }
    for (; ptr_path[0] == '.'; ptr_path++) {
        ;
    }
    for (; ptr_path[0] == '/'; ptr_path++) {
        ;
    }
    (void)snprintf_s(tmpPath, LITTLEFS_MAX_LFN_LEN, LITTLEFS_MAX_LFN_LEN, "%s/%s", GetLittlefsMountPoint(), ptr_path);
    return 0;
}

int HalFileOpen(const char *path, int oflag, int mode)
{
    int fd, tflag = 0;
    char tmpPath[LITTLEFS_MAX_LFN_LEN];
    if (sHalFileGetPath(tmpPath, path)) {
        return -1;
    }
    if (O_CREAT_FS == (oflag & O_CREAT_FS)) {
        tflag |= O_CREAT;
    }
#if O_WRONLY_FS != 0
    if (O_RDWR_FS == (oflag & O_RDWR_FS)) {
        tflag |= O_RDWR;
    } else if (O_WRONLY_FS == (oflag & O_WRONLY_FS)) {
        tflag |= O_WRONLY;
    } else {
        tflag |= O_RDONLY;
    }
#else
    if (O_RDWR_FS == (oflag & O_RDWR_FS)) {
        tflag |= O_RDWR;
    } else if (O_RDONLY_FS == (oflag & O_RDONLY_FS)) {
        tflag |= O_RDONLY;
    } else {
        tflag |= O_WRONLY;
    }
#endif
    if (O_APPEND_FS == (oflag & O_APPEND_FS)) {
        tflag |= O_APPEND;
    }

    if (O_EXCL_FS == (oflag & O_EXCL_FS)) {
        tflag |= O_EXCL;
    }
    if (O_TRUNC_FS == (oflag & O_TRUNC_FS)) {
        tflag |= O_TRUNC;
    }

    fd = _open(tmpPath, tflag);
    if (fd < 0) {
        return fd;
    }
    return fd + OFFSET_FD;
}

int HalFileClose(int fd)
{
    if (fd < OFFSET_FD) {
        return -1;
    }
    return _close(fd - OFFSET_FD);
}

int HalFileRead(int fd, char *buf, unsigned int len)
{
    if (fd < OFFSET_FD) {
        return -1;
    }
    return _read(fd - OFFSET_FD, buf, len);
}

int HalFileWrite(int fd, const char *buf, unsigned int len)
{
    if (fd < OFFSET_FD) {
        return -1;
    }
    return _write(fd - OFFSET_FD, buf, len);
}

int HalFileDelete(const char *path)
{
    char tmpPath[LITTLEFS_MAX_LFN_LEN];
    if (sHalFileGetPath(tmpPath, path)) {
        return -1;
    }
    return _unlink(tmpPath);
}

int HalFileStat(const char *path, unsigned int *fileSize)
{
    off_t len;
    int fd;
    char tmpPath[LITTLEFS_MAX_LFN_LEN];
    if (sHalFileGetPath(tmpPath, path)) {
        return -1;
    }
    fd = _open(tmpPath, O_RDONLY);
    if (fd < 0) {
        return -1;
    }
    len = _lseek(fd, 0, SEEK_END);
    _close(fd);
    if (fileSize) {
        *fileSize = len;
    }
    return 0;
}

int HalFileSeek(int fd, int offset, unsigned int whence)
{
    int _offset = offset;
    int type = whence;
    int fd_tmp = fd;
    if (fd_tmp < OFFSET_FD) {
        return -1;
    }
    switch (type) {
        case SEEK_SET_FS:
            type = SEEK_SET;
            break;
        case SEEK_CUR_FS:
            type = SEEK_CUR;
            break;
        case SEEK_END_FS:
            type = SEEK_END;
            break;
        default:
            return -1;
    }
    fd_tmp -= OFFSET_FD;
    if ((SEEK_SET == type) || (SEEK_CUR == type)) {
        off_t len, len2;
        len = _lseek(fd_tmp, 0, SEEK_CUR);
        len2 = _lseek(fd_tmp, 0, SEEK_END);
        if (SEEK_CUR == type) {
            _offset += len;
        }
        if (_offset > len2) {
            _lseek(fd_tmp, len, SEEK_SET);
            return -1;
        }
        return _lseek(fd_tmp, (off_t)_offset, SEEK_SET);
    }
    return _lseek(fd_tmp, (off_t)_offset, type);
}
