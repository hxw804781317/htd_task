// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_TASK_THREAD_SCH_H_
#define _HTD_TASK_THREAD_SCH_H_

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


// @brief 加载定时任务(只能在p_object对应的线程调用)
// @param timeout_ms 如果为0，is_repeat 必须为0
bool htd_os_load_timer_function(void* p_object, bool is_repeat, uint32_t timeout_ms, void (*timeout_cb)(void*), void* arg);
// @brief 删除定时任务
void htd_os_unload_timer_function(void* p_object, void (*timeout_cb)(void*));

// @brief 加载紧急任务(不可以在中断中调用)
void htd_os_load_urgent_function(void* p_object, void (*fun)(void*), void* arg);
// @brief 添加空闲任务(最低优先级任务，其他任务执行后才会执行这些回调函数, 不可以在中断中调用)
void htd_os_load_idle_function(void* p_object, void (*fun)(void*), void* arg);

// @brief 加载函数(可以在中断中调用)
bool htd_os_load_function_fisr(void* p_object, void (*fun)(void*), void* arg);
// @brief 加载函数(可以在中断中调用)
bool htd_os_load_function(void* p_object, void (*fun)(void*), void* arg);
// @brief 加载函数数据(不可以在中断中调用)
bool htd_os_load_function_data(void* p_object, void (*fun)(const uint8_t*, uint16_t), const void* p_data, uint16_t data_len);
// @brief 加载类型函数数据(不可以在中断中调用)
bool htd_os_load_function_type_data(void* p_object, void (*fun)(uint16_t, const uint8_t*, uint16_t), uint16_t type, const uint8_t* p_data, uint16_t data_len);


// @brief 初始化任务调度器
// @return 返回 p_object 为其他接口函数的第一个参数
void* htd_os_sch_init(const char* p_name, uint8_t mq_count, uint8_t thread_priority, uint32_t thread_stack_size);
// @brief 去初始化任务调度器
void htd_os_sch_uninit(void* p_object);


#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_TASK_THREAD_SCH_H_*/

