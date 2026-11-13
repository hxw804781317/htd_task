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
#ifndef MEMSET
    #define MEMSET(ptr, v, n)                   htd_memset(ptr, v, n)
#endif
#ifndef MEMCPY
    #define MEMCPY(dst, src, n)                 htd_memcpy(dst, src, n)
#endif
#ifndef MEMMOVE
    #define MEMMOVE(dst, src, n)                htd_memmove(dst, src, n)
#endif
#ifndef MEMCMP
    #define MEMCMP(v1, v2, n)                   htd_memcmp(v1, v2, n)
#endif
#ifndef MALLOC
    #define MALLOC(size)                        htd_malloc(size)
#endif
#ifndef FREE
    #define FREE(p_ptr)                         htd_free(p_ptr)
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


#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_CONSOLE_PORT_H_*/

