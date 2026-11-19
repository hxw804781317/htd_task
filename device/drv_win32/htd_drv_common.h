// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_DRV_COMMON_H_
#define _HTD_DRV_COMMON_H_

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif


// @brief 申请内存
void* htd_drv_malloc(uint32_t size);
// @brief 释放内存
void htd_drv_free(void* p_ptr);


#ifdef __cplusplus
extern "C" {
#endif
#endif /*_HTD_DRV_COMMON_H_*/

