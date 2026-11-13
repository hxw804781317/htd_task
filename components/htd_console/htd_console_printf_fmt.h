// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_CONSOLE_PRINTF_FMT_H_
#define _HTD_CONSOLE_PRINTF_FMT_H_

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>
#include "htd_console_port.h"

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(HTD_TASK_ENABLE)
//////////////////////////////////////////////////////////////////////
typedef void (* htd_fprintf_fwrite)(void const * p_user_ctx, char const * p_str, size_t length);

// printf 格式化结构体
typedef struct {
    char* p_io_buffer;              ///< 格式化指针
    size_t io_buffer_size;          ///< 格式化缓冲区大小
    size_t io_buffer_cnt;           ///< 当前格式化字符长度
    size_t out_buffer_cnt;          ///< 输出多少字符
    
    void* p_user_ctx;               ///< Pointer to user data to be passed to the fwrite funciton.
    htd_fprintf_fwrite fwrite;      ///< Pointer to function sending data stream.
} htd_printf_ctx_t;

// @brief 格式化输出字符串, 返回输出的数据长度
uint32_t htd_fprintf(htd_printf_ctx_t* const p_ctx, char const *p_fmt, ...);
// @brief 格式化输出字符串, 返回输出的数据长度
uint32_t htd_vfprintf(htd_printf_ctx_t* const p_ctx, char const *p_fmt, va_list *p_args);

// @brief 格式化数据htd_fprintf_ctx_t
uint32_t htd_snprintf(char* p_data, uint32_t data_size, char const * p_fmt, ...);
// @brief 格式化输出
uint32_t htd_vsnprintf(char* p_data, uint32_t data_size, char const * p_fmt, va_list *p_args);


#endif /*!defined(HTD_TASK_ENABLE)*/
#ifdef __cplusplus
}
#endif
#endif /* _HTD_CONSOLE_PRINTF_FMT_H_ */

