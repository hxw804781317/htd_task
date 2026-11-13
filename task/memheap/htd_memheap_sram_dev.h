// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_MEMHEAP_SRAM_DEV_H_
#define _HTD_MEMHEAP_SRAM_DEV_H_

#include "htd_memheap_ff.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


//////////////////////////////////////////////////////////////////////
// 定义可注册的内存设备数量
#ifndef HTD_MEMHEAP_SRAM_DEV_COUNT
    #define HTD_MEMHEAP_SRAM_DEV_COUNT  2
#endif


// @brief 添加内存设备
int32_t htd_memheap_sram_dev_add(htd_memheap_dev_t* p_dev);
// @brief 删除内存设备
int32_t htd_memheap_sram_dev_del(htd_memheap_dev_t* p_dev);
// @brief 申请内存
void* htd_memheap_sram_malloc(size_t size, void* fun);
// @brief 申请内存
void* htd_memheap_sram_calloc(uint32_t count, size_t size, void* fun);
// @brief 重新申请内存
void* htd_memheap_sram_realloc(void *p_ptr, size_t newsize, void* fun);
// @brief 释放内存
void htd_memheap_sram_free(const void *p_ptr);
// @brief 显示所有内存信息
void htd_memheap_sram_dump(void);






//////////////////////////////////////////////////////////////////////
// 内存接口
#define htd_malloc(size)                htd_memheap_sram_malloc(size, (void*)__func__)
#define htd_calloc(count, size)         htd_memheap_sram_calloc(count, size, (void*)__func__)
#define htd_realloc(p_ptr, newsize)     htd_memheap_sram_realloc(p_ptr, newsize, (void*)__func__)
#define htd_free(p_ptr)                 htd_memheap_sram_free(p_ptr)


#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_MEMHEAP_SRAM_DEV_H_*/

