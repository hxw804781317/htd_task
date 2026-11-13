// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_TASK_PORT_H_
#define _HTD_TASK_PORT_H_

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>

#include "htd_config.h"         // 引用配置文件

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


///////////////////////////////配置选项///////////////////////////////////////
#if !defined(HTD_TASK_ENABLE)
    #define HTD_TASK_ENABLE                         1
    // 定义使用哪一种 RTOS，默认不使用RTOS
    #define HTD_TASK_NONOS                          1
    //#define HTD_TASK_FREERTOS                       1
    //#define HTD_TASK_WIN32_SDL2                     1
    // 定义内存堆
    #define HTD_MEMHEAP_FF_ENABLE                   1
    // 是否使用内存堆调试功能，默认不使用
    //#define HTD_MEMHEAP_FF_USE_TRACE                1
    // memcpy/memset 是否使用字节拷贝, 默认不使用
    //#define HTD_MEM_USE_BYTELOOP                    1
    // printf是否支持浮点数，默认不支持
    //#define HTD_PRINTF_SUPPORT_FLOAT                1
    // 是否使用 SEGGER-RTT
    //#define LIB_SEGGER_RTT_ENABLE                   1
    // 是否为正式版本，默认为调试版本，打开后，所有 ASSERT 定义为空
    //#define VER_RELEASE                             1
#endif
// 使用RTOS时，定时主线程栈大小和优先级
#if !defined(HTD_TASK_NONOS)
    #ifndef HTD_TASK_THREAD_PRIORITY
        #define HTD_TASK_THREAD_PRIORITY            28
    #endif
    #ifndef HTD_TASK_THREAD_STACK_SIZE
        #define HTD_TASK_THREAD_STACK_SIZE          4096
    #endif
#endif





///////////////////////////////对接接口///////////////////////////////////////
// @brief 延迟us
void htd_delay_us(uint32_t us);
// @brief 断言失败回调
void htd_assert_failed(const char* file, uint32_t line);
// @brief 获取系统时间，单位毫秒(weak 函数，需要外部实现)
uint32_t htd_time_ms_get(void);


#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_TASK_PORT_H_*/

