// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL       2       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG       "task.task_sch"
#include "htd_task.h"
#include "htd_task_sch.h"
#include "htd_task_thread_sch.h"

#if (!defined(HTD_TASK_NONOS))
//////////////////////////////////////////////////////////////
static void* m_task_thread = NULL;

// @brief 加载定时任务(不可以在中断中调用)
bool htd_task_load_timer_function(bool is_repeat, uint32_t timeout_ms, htd_task_timeout_handle_t handle, void* arg)
{
    return htd_os_load_timer_function(m_task_thread, is_repeat, timeout_ms, handle, arg);
}

// @brief 删除定时任务
void htd_task_unload_timer_function(htd_task_timeout_handle_t handle)
{
    htd_os_unload_timer_function(m_task_thread, handle);
}

// @brief 加载紧急任务(不可以在中断中调用)
void htd_task_load_urgent_function(htd_task_timeout_handle_t handle, void* arg)
{
    htd_os_load_urgent_function(m_task_thread, handle, arg);
}

// @brief 添加空闲任务(最低优先级任务，其他任务执行后才会执行这些回调函数, 不可以在中断中调用)
void htd_task_load_idle_function(htd_task_timeout_handle_t handle, void* arg)
{
    htd_os_load_idle_function(m_task_thread, handle, arg);
}

// @brief 加载函数(可以在中断中调用)
bool htd_task_load_function_fisr(htd_task_timeout_handle_t handle, void * arg)
{
    return htd_os_load_function_fisr(m_task_thread, handle, arg);
}

// @brief 加载函数(不可以在中断中调用)
bool htd_task_load_function(htd_task_timeout_handle_t handle, void* arg)
{
    return htd_os_load_function(m_task_thread, handle, arg);
}

// @brief 加载函数数据(不可以在中断中调用)
bool htd_task_load_function_data(htd_task_data_handle_t handle, const void* p_data, uint16_t data_len)
{
    return htd_os_load_function_data(m_task_thread, handle, p_data, data_len);
}

// @brief 加载类型函数数据(不可以在中断中调用)
bool htd_task_load_function_type_data(htd_task_data_type_handle_t handle, uint16_t type, const uint8_t* p_data, uint16_t data_len)
{
    return htd_os_load_function_type_data(m_task_thread, handle, type, p_data, data_len);
}

// @brief 初始化任务调度器
int32_t htd_task_sch_init(uint8_t mq_count)
{
    if (m_task_thread == NULL) {
        m_task_thread = htd_os_sch_init("TASK", mq_count, HTD_TASK_THREAD_PRIORITY, HTD_TASK_THREAD_STACK_SIZE);
        ASSERT(m_task_thread);
    }
    return 0;
}

// @brief 系统循环调度
void htd_task_sch_loop(htd_task_sleep_handle_t sleep_handle)
{
    htd_thread_scheduler_start();
}
#endif 

