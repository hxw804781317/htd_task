// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_VFS_ROMFS_H_
#define _HTD_VFS_ROMFS_H_

#include "htd_vfs_port.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


#define FSROM_DIRENT_FILE   0x00
#define FSROM_DIRENT_DIR    0x01
typedef struct {
    uint32_t type;
    const char *name; 
    const uint8_t *data;
    size_t size;
} vfs_romfs_dirent_t;

// 构建只读文件系统样例; 可以使用python脚本 mkromfs.py 自动构建
const static unsigned char fsrom_root_txt1_1[] = {
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
};
const static vfs_romfs_dirent_t fsrom_root_dir1[] = {
    {FSROM_DIRENT_FILE, "dummy.txt", fsrom_root_txt1_1, sizeof(fsrom_root_txt1_1)},
};
const static unsigned char fsrom_root_txt1[] = {
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
};
const static vfs_romfs_dirent_t fsrom_root[] = {
    {FSROM_DIRENT_DIR, "dummy", (uint8_t*)fsrom_root_dir1, ARRAY_COUNT(fsrom_root_dir1)},
    {FSROM_DIRENT_FILE, "dummy.txt", fsrom_root_txt1, sizeof(fsrom_root_txt1)},
};
const static vfs_romfs_dirent_t m_romfs_root = {
    FSROM_DIRENT_DIR, "/", (uint8_t *)fsrom_root, ARRAY_COUNT(fsrom_root)
};



#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_VFS_ROMFS_H_*/

