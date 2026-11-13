// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_TASK_SCH_H_
#define _HTD_TASK_SCH_H_

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

typedef void (*htd_task_timeout_handle_t)(void*);
typedef void (*htd_task_data_handle_t)(const uint8_t*, uint16_t);
typedef void (*htd_task_data_type_handle_t)(uint16_t, const uint8_t*, uint16_t);
typedef uint32_t (*htd_task_sleep_handle_t)(uint32_t timeout_ms);



// @brief 加载定时任务(不可以在中断中调用)
// @param timeout_ms 如果为0，is_repeat 必须为0
bool htd_task_load_timer_function(bool is_repeat, uint32_t timeout_ms, htd_task_timeout_handle_t handle, void* arg);
// @brief 删除定时任务
void htd_task_unload_timer_function(htd_task_timeout_handle_t handle);
// @brief 加载紧急任务(不可以在中断中调用)
void htd_task_load_urgent_function(htd_task_timeout_handle_t handle, void* arg);
// @brief 添加空闲任务(最低优先级任务，其他任务执行后才会执行这些回调函数, 不可以在中断中调用)
void htd_task_load_idle_function(htd_task_timeout_handle_t handle, void* arg);

// @brief 加载函数(可以在中断中调用)
bool htd_task_load_function_fisr(htd_task_timeout_handle_t handle, void* arg);
// @brief 加载函数(不可以在中断中调用)
bool htd_task_load_function(htd_task_timeout_handle_t handle, void* arg);
// @brief 加载函数数据(不可以在中断中调用)
bool htd_task_load_function_data(htd_task_data_handle_t handle, const void* p_data, uint16_t data_len);
// @brief 加载类型函数数据(不可以在中断中调用)
bool htd_task_load_function_type_data(htd_task_data_type_handle_t handle, uint16_t type, const uint8_t* p_data, uint16_t data_len);


// @brief 初始化任务调度器
// @param 参数只有在 HTD_TASK_USE_THREAD 打开时有效
int32_t htd_task_sch_init(uint8_t mq_count);
// @brief 系统循环调度
void htd_task_sch_loop(htd_task_sleep_handle_t sleep_handle);


#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_TASK_SCH_H_*/

