// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_MEMHEAP_FF_H_
#define _HTD_MEMHEAP_FF_H_

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


// 内存堆命令
#define HTD_MEMHEAP_CMD_NONE            0x00
#define HTD_MEMHEAP_CMD_DUMP            0x01
#define HTD_MEMHEAP_CMD_INFO            0x02
#define HTD_MEMHEAP_CMD_PTR_TEST        0x03
typedef uint32_t htd_heap_cmd_t;

// 内存设备
typedef struct htd_memheap_dev_s htd_memheap_dev_t;
typedef void* (*memheap_alloc_handle_t)(htd_memheap_dev_t *p_dev, size_t size, void* func);
typedef void* (*memheap_realloc_handle_t)(htd_memheap_dev_t *p_dev, void *ptr, size_t newsize, void* fun);
typedef void (*memheap_free_handle_t)(void* ptr);
typedef int32_t (*memheap_ctrl_handle_t)(htd_memheap_dev_t *p_dev, htd_heap_cmd_t cmd, void* param, uint32_t param_len);
struct htd_memheap_dev_s{
    void* p_heap;
    memheap_alloc_handle_t alloc_handle;
    memheap_realloc_handle_t realloc_handle;
    memheap_free_handle_t free_handle;
    memheap_ctrl_handle_t ctrl_handle;
};


// @brief 初始化
htd_memheap_dev_t* htd_memheap_ff_init(void *start_addr, size_t size);


#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_MEMHEAP_FF_H_*/

