// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_CONSOLE_HISTORY_H_
#define _HTD_CONSOLE_HISTORY_H_

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>
#include "htd_console_port.h"
#include "htd_console_process.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/



// @brief 上键
void console_key_up(htd_console_t *p_console);
// @brief 下键
void console_key_down(htd_console_t *p_console);
// @brief 保存历史指令
void console_history_cmd_save(htd_console_t *p_console);
// @brief 重置历史指令
void console_history_cmd_reset(htd_console_t *p_console);


#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_CONSOLE_HISTORY_H_*/

