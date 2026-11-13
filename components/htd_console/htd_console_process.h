// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_CONSOLE_PROCESS_H_
#define _HTD_CONSOLE_PROCESS_H_

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>

#include "htd_console_port.h"
#include "htd_console_section.h"
#include "htd_console_slist.h"


#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


//////////////////////////////////////////////////////////////////////
#ifndef CONSOLE_PROMPT
    #define CONSOLE_PROMPT                      "htd >"
#endif


#define HTD_CSL_INPUT_STAT_NONE                 0
#define HTD_CSL_INPUT_STAT_WAIT_SPEC_KEY        1
#define HTD_CSL_INPUT_STAT_WAIT_FUNC_KEY        2
typedef uint8_t htd_csl_input_stat_t;
typedef int32_t (*htd_csl_write_handle_t)(const uint8_t* p_data, int32_t data_len);

typedef struct {
    htd_slist_node_t node;
    char history_cmd[HTD_CONSOLE_CMD_SIZE];
} history_cmd_t;


typedef struct {
#if HTD_CONSOLE_HISTORY_LINES > 0
    htd_slist_node_t history_head;
    history_cmd_t* history_current;
    uint8_t history_count;
#endif
    
    uint8_t line_position;
    uint8_t line_curpos;
    htd_csl_input_stat_t stat;
    char line[HTD_CONSOLE_CMD_SIZE];
    
    htd_csl_write_handle_t write_handle;
    uint32_t *cmd_table_begin; 
    uint32_t *cmd_table_end;
} htd_console_t;




//////////////////////////////////////////////////////////////////////
// 定义命令回调函数
typedef int (*cmd_function_t)(int argc, char **argv);
typedef struct {
    const char* p_name; 
    cmd_function_t func;
} csl_cmd_cb_t;


#if (defined HTD_CONSOLE_ENABLE) && defined(HTD_CONSOLE_SUPPORT_CMD)
    #define CONSOLE_SECTION_NAME    htd_console
    #define CONSOLE_CMD_EXPORT(name, cmd)                                       \
        HTD_SECTION_ITEM_REGISTER(CONSOLE_SECTION_NAME, const csl_cmd_cb_t CONCAT_2(_sh_, cmd)) = {\
            .p_name = STRINGIFY(CONCAT_2(_cmd_, cmd)),                          \
            .func = (cmd_function_t)&name                                       \
        };
#else 
    #define CONSOLE_CMD_EXPORT(name, cmd)
#endif




//////////////////////////////////////////////////////////////////////
// @brief 初始化控制台, 只能初始化一次
// @param write_handle 控制台打印输出接口
int32_t htd_console_process_init(htd_csl_write_handle_t write_handle);
// @brief 控制台输入字符函数
void htd_console_process_loop(uint8_t ch);
// @brief 控制台打印, 通过初始化输入的write_handle输出数据
int32_t htd_console_print(const char *fmt, ...);
// @brief 控制台打印输出，返回打印的字符长度
uint32_t htd_console_fprintf(char const *p_fmt, va_list *p_args);



#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_CONSOLE_PROCESS_H_*/

