// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
//#define DEBUG_LVL       2       // 1:error; 2:warning; 3:debug; 4:log
//#define DEBUG_TAG       "dev.wdt"
#include "htd_dev_wdt.h"
#include "htd_device.h"



// @brief 初始化看门狗
__WEAK int32_t htd_dev_wdt_init(void)
{
    print_warning("weak function: htd_dev_wdt_init");
    return 0;
}

// @brief 去初始化看门狗
__WEAK int32_t htd_dev_wdt_deinit(void)
{
    print_warning("weak function: htd_dev_wdt_deinit");
    return 0;
}

// @brief 喂狗
__WEAK void htd_dev_wdt_feed(void)
{
    print_warning("weak function: htd_dev_wdt_feed");
}


