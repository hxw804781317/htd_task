// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_CONSOLE_PORT_H_
#define _HTD_CONSOLE_PORT_H_

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


//////////////////////////////////////////////////////////////////////
#include "htd_task.h"

// 是否支持命令输入
//#define HTD_CONSOLE_SUPPORT_CMD                 1
// 是否支持命令存储
//define HTD_CONSOLE_SUPPORT_CMD_HISTORY          1

// 命令行长度(可输入字符长度)
#ifndef HTD_CONSOLE_CMD_SIZE
    #define HTD_CONSOLE_CMD_SIZE                48
#endif
// 保存的命令行行数
#ifndef HTD_CONSOLE_HISTORY_LINES
    #define HTD_CONSOLE_HISTORY_LINES           5
#endif
// 命令行参数个数
#ifndef HTD_CONSOLE_ARG_MAX
    #define HTD_CONSOLE_ARG_MAX                 8
#endif






//////////////////////////////////////////////////////////////////////
#ifndef ASSERT
    #define ASSERT(a)                           
#endif
#ifndef htd_memset
    #define htd_memset(ptr, v, n)               memset(ptr, v, n)
#endif
#ifndef htd_memcpy
    #define htd_memcpy(dst, src, n)             memcpy(dst, src, n)
#endif
#ifndef htd_memmove
    #define htd_memmove(dst, src, n)            memmove(dst, src, n)
#endif
#ifndef htd_memcmp
    #define htd_memcmp(v1, v2, n)               memcmp(v1, v2, n)
#endif
#ifndef htd_malloc
    #define htd_malloc(size)                    malloc(size)
#endif
#ifndef htd_free
    #define htd_free(p_ptr)                     free(p_ptr)
#endif




//////////////////////////////////////////////////////////////////////
#ifndef CONCAT_2
    #define CONCAT_2_(p1, p2)                   p1##p2
    #define CONCAT_2(p1, p2)                    CONCAT_2_(p1, p2)
#endif
#ifndef STR
    #define STR(s)                              #s
#endif 
#ifndef STRINGIFY
    #define STRINGIFY(val)                      STR(val)
#endif 







//////////////////////////////////////////////////////////////////////
// @brief 初始化控制台(打开字符输出)
// @note 内部使用了 htd_malloc，必须在 ram_heap_init 初始化后再调用
int32_t app_console_init(void);
// @brief 去初始化控制台
int32_t app_console_uninit(void);
// @brief 控制台开始接收数据
void app_console_read_start(void);
// @brief 刷新
// @return 返回0说明已经刷新完毕，返回大于0，表示还有多长数据要发送
int32_t app_console_tx_flush(uint32_t waite_timeout_ms);





#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_CONSOLE_PORT_H_*/

