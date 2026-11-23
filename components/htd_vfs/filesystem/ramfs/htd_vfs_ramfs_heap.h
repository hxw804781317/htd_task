// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_VFS_RAMFS_HEAP_H_
#define _HTD_VFS_RAMFS_HEAP_H_

#include "htd_vfs_port.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


// @brief 初始化动态内存
void* ramfs_heap_init(void* p_start_addr, size_t size);
// @brief 动态申请内存
// @param size 申请的内存大小
// @return 成功返回申请到的内存地址，失败返回NULL
void *ramfs_heap_malloc(void* p_mem, uint32_t size);
// @brief 动态申请内存
// @param size 申请的内存大小
// @return 成功返回申请到的内存地址，失败返回NULL
void *ramfs_heap_realloc(void* p_mem, void* p_ptr, uint32_t size);
// @brief 动态释放内存
// @param ptr 释放的内存地址
void ramfs_heap_free(void *ptr);
// @brief 判断 p_ptr 指针是否在内存池内
uint32_t ramfs_heap_ptr_is_in_range(void* p_mem, uintptr_t ptr);
// @brief 获取内存大小
uint32_t ramfs_heap_size_get(void* p_mem);
// @brief 获取有效的大小
uint32_t ramfs_heap_available_size_get(void* p_mem);


#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_VFS_RAMFS_HEAP_H_*/

