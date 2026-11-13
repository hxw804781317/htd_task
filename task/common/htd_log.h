// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_LOG_H_
#define _HTD_LOG_H_

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
#define HTD_LOG_LVL_L       4
#define HTD_LOG_LVL_D       3
#define HTD_LOG_LVL_W       2
#define HTD_LOG_LVL_E       1


// 打印接口
#if defined(HTD_VER_RELEASE)
    #define htd_print(fmt, ...) 
    #define gui_kprint(fmt, ...)
#elif defined(HTD_CONSOLE_ENABLE)
    extern int32_t htd_console_print(const char *fmt, ...);
    #define htd_print(fmt, ...)                 htd_console_print(fmt, ##__VA_ARGS__)
    #define gui_kprint(fmt, ...)                htd_console_print(fmt, ##__VA_ARGS__)
#elif defined(LIB_SEGGER_RTT_ENABLE)
    // @brief segger-rtt 打印输出
    int32_t htd_segger_rtt_print(const char *fmt, ...); 
    #define htd_print(fmt, ...)                 htd_segger_rtt_print(fmt, ##__VA_ARGS__)
    #define gui_kprint(fmt, ...)                htd_segger_rtt_print(fmt, ##__VA_ARGS__)
#else 
    #define htd_print(fmt, ...)                 printf(fmt, ##__VA_ARGS__)
    #define gui_kprint(fmt, ...)                printf(fmt, ##__VA_ARGS__)
#endif
#define gui_print_hex(tag, buff, buff_len)      htd_print_hex(tag, buff, buff_len)
#define print_hex(tag, buff, buff_len)          htd_print_hex(tag, buff, buff_len)


#if (defined(DEBUG_LVL) && (DEBUG_LVL >= HTD_LOG_LVL_E))
    #define gui_error(fmt, ...)    do {   \
        gui_kprint("\033[01;35m%s(%d):"fmt"\r\n\033[0;0m", DEBUG_TAG, __LINE__, ##__VA_ARGS__);  \
    }while(0)
    #define print_error(fmt, ...)    do {   \
        htd_print("\033[0;31m%s(%d):"fmt"\r\n\033[0;0m", DEBUG_TAG, __LINE__, ##__VA_ARGS__);  \
    }while(0)
#else
    #define gui_error(fmt, ...)
    #define print_error(fmt, ...)
#endif

#if (defined(DEBUG_LVL) && (DEBUG_LVL >= HTD_LOG_LVL_W))
    #define gui_warning(fmt, ...)    do {   \
        gui_kprint("\033[0;36m%s(%d):"fmt"\r\n\033[0;0m", DEBUG_TAG, __LINE__, ##__VA_ARGS__);  \
    }while(0)
    #define print_warning(fmt, ...)    do {   \
        htd_print("\033[0;33m%s(%d):"fmt"\r\n\033[0;0m", DEBUG_TAG, __LINE__, ##__VA_ARGS__);  \
    }while(0)
#else
    #define gui_warning(fmt, ...)
    #define print_warning(fmt, ...)
#endif

#if (defined(DEBUG_LVL) && (DEBUG_LVL >= HTD_LOG_LVL_D) && !defined(VER_RELEASE))
    #define gui_debug(fmt, ...)   do { \
        gui_kprint("\033[0;33m%s(%d):"fmt"\r\n\033[0;0m", DEBUG_TAG, __LINE__, ##__VA_ARGS__);  \
    }while(0)
    #define print_debug(fmt, ...)   do { \
        htd_print("%s(%d):"fmt"\r\n", DEBUG_TAG, __LINE__, ##__VA_ARGS__);  \
    }while(0)
#else
    #define gui_debug(fmt, ...)
    #define print_debug(fmt, ...)
#endif

#if (defined(DEBUG_LVL) && (DEBUG_LVL >= HTD_LOG_LVL_L) && !defined(VER_RELEASE))
    #define gui_log(fmt, ...)   do { \
        gui_kprint("%s(%d):"fmt"\r\n", DEBUG_TAG, __LINE__, ##__VA_ARGS__);  \
    }while(0)
    #define print_log(fmt, ...)   do { \
        htd_print("%s(%d):"fmt"\r\n", DEBUG_TAG, __LINE__, ##__VA_ARGS__);  \
    }while(0)
#else
    #define gui_log(fmt, ...)
    #define print_log(fmt, ...)
#endif



// @brief 打印 16 进制
void htd_print_hex(const char* p_tag, const uint8_t* p_buff, uint16_t buff_len);
// @brief 内存数据显示
void htd_hexdump(const uint8_t *p_data, uint32_t data_len);


#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_LOG_H_*/

