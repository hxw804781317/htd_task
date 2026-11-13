// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL       4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG       "task.port"
#include "htd_task.h"
#include "htd_task_port.h"






//////////////////////////////////////////////////////////////////////
// @brief 断言失败回调
__WEAK void htd_assert_failed(const char* file, uint32_t line)
{
    while (1) {
        print_error("assert: file=%s, line=%d", file, line);
        htd_sleep_ms(1000);
    }
}


#if !defined(HTD_TASK_WIN32_SDL2)
// @brief 延迟us
__WEAK void htd_delay_us(uint32_t us)
{

}


#if defined(HTD_TASK_NONOS)
//////////////////////////////////////////////////////////////////////
// @brief 获取系统时间，单位毫秒(weak 函数，需要外部实现)
__WEAK uint32_t htd_time_ms_get(void)
{
    print_error("weak function: htd_time_ms_get");
    ASSERT(0);
    return 0;
}
#endif
#endif /*!defined(HTD_TASK_WIN32_SDL2)*/


