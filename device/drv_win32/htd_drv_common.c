// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL   4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG   "drv.common"
#include "htd_device.h"
#include "htd_drv_common.h"


// @brief 申请内存
void* htd_drv_malloc(uint32_t size)
{
    return htd_malloc(size);
}

// @brief 释放内存
void htd_drv_free(void* p_ptr)
{
    htd_free(p_ptr);
}



