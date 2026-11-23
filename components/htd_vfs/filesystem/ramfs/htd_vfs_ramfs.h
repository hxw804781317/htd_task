// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_VFS_RAMFS_H_
#define _HTD_VFS_RAMFS_H_

#include "htd_vfs_port.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


// @brief 创建一个内存文件系统
void *htd_vfs_ramfs_create(uint8_t *p_mem_block, size_t mem_block_size);
// @brief 添加内存块到文件系统中
int32_t htd_vfs_ramfs_append(void* ram_fs, uint8_t* p_mem_block, size_t mem_block_size);


#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_VFS_RAMFS_H_*/

