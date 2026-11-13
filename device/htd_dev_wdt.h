// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_DEV_WDT_H_
#define _HTD_DEV_WDT_H_

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

#ifndef BSP_WDT_TIMEOUT_MS
    #define BSP_WDT_TIMEOUT_MS          5000
#endif


// @brief 初始化看门狗
int32_t htd_dev_wdt_init(void);
// @brief 去初始化看门狗
int32_t htd_dev_wdt_deinit(void);
// @brief 喂狗
void htd_dev_wdt_feed(void);


#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_DEV_WDT_H_*/

