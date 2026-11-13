// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_TASK_NOS_H_
#define _HTD_TASK_NOS_H_

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "htd_task_sch.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


#if defined(HTD_TASK_NONOS)
//////////////////////////////////////////////////////////////////////
// @brief 进去保护区(禁止系统中断)
__STATIC_INLINE uint32_t htd_interrupt_disable(void)
{
    return 0;
}
// @brief 退出保护区(使能系统中断)
__STATIC_INLINE void htd_interrupt_enable(uint32_t level)
{
    
}

// @brief 进去保护区(禁止系统中断)
__STATIC_INLINE void htd_critical_enter(void)
{
    
}
// @brief 退出保护区(使能系统中断)
__STATIC_INLINE void htd_critical_exit(void)
{
    
}

// @brief  禁止系统休眠锁上锁
__STATIC_INLINE void htd_wake_lock(void)
{
    //rt_pm_request(PM_SLEEP_MODE_IDLE);
}
// @brief  禁止系统休眠锁解锁
__STATIC_INLINE void htd_wake_unlock(void)
{
    //rt_pm_release(PM_SLEEP_MODE_IDLE);
}



/////////////////////////////////信号量/////////////////////////////////////
typedef struct {
    volatile uint32_t init_count;
    volatile uint32_t max_count;
}htd_sem_t;
// @brief 创建信号量
htd_sem_t* htd_sem_create(const char* p_name, uint32_t init_count, uint32_t max_count);
// @brief 删除信号量
bool htd_sem_destroy(htd_sem_t* sem);
// @brief 初始化信号量
int32_t htd_sem_init(htd_sem_t* sem, const char* p_name, uint32_t init_count, uint32_t max_count);
// @brief 去初始化信号量
int32_t htd_sem_deinit(htd_sem_t* sem);
// @brief 查看信号量(减一); 
bool htd_sem_take(htd_sem_t* sem, uint32_t wait_ms);
// @brief 发送信号量(信号量加一)
bool htd_sem_give(htd_sem_t* sem);
// @brief 设置信号量(加-)
bool htd_sem_give_fisr(htd_sem_t* sem);
// @brief 重置信号量
void htd_sem_reset(htd_sem_t* sem);



/////////////////////////////////信号量锁/////////////////////////////////////
typedef htd_sem_t htd_sem_lock_t;
// @brief 初始化信号量的锁
__STATIC_INLINE bool htd_sem_lock_init(htd_sem_lock_t* p_sem_lock, const char* p_name)
{
    return htd_sem_init(p_sem_lock, p_name, 1, 1) == 0 ? true : false;
}
// @brief 剥离信号量的锁
__STATIC_INLINE int32_t htd_sem_lock_deinit(htd_sem_lock_t *p_sem_lock)
{
    return htd_sem_deinit(p_sem_lock);
}
// @brief 创建信号量锁
__STATIC_INLINE htd_sem_lock_t* htd_sem_lock_create(const char* p_name)
{
    return htd_sem_create(p_name, 1, 1);
}
// @brief 销毁信号量锁
__STATIC_INLINE void htd_sem_lock_destroy(htd_sem_lock_t* p_sem_lock)
{
    htd_sem_destroy(p_sem_lock);
}
// @brief 加锁(-1)
__STATIC_INLINE void htd_sem_lock(htd_sem_lock_t* p_sem_lock)
{
    htd_sem_take(p_sem_lock, 0xffffffff);
}
// @brief 解锁(+1)
__STATIC_INLINE void htd_sem_unlock(htd_sem_lock_t* p_sem_lock)
{
    htd_sem_give(p_sem_lock);
}




/////////////////////////////////互斥锁/////////////////////////////////////
typedef struct {
    volatile uint32_t val;
} htd_mutex_lock_t;
// @brief 初始化互斥锁
bool htd_mutex_lock_init(htd_mutex_lock_t* p_lock, const char* p_name);
// @brief 去初始化互斥锁
int32_t htd_mutex_lock_deinit(htd_mutex_lock_t* p_lock);
// @brief 创建互斥锁
htd_mutex_lock_t* htd_mutex_lock_create(const char* p_name);
// @brief 销毁锁
void htd_mutex_lock_destroy(htd_mutex_lock_t* p_lock);
// @brief 加锁
void htd_mutex_lock(htd_mutex_lock_t* p_lock);
// @brief 解锁
void htd_mutex_unlock(htd_mutex_lock_t* p_lock);



/////////////////////////////////定时器/////////////////////////////////////
// @brief 创建软件定时器(不可以在中断中调用)
// @return 成功返回定时器操作句柄，失败返回NULL
void* htd_timer_create(const char* name, unsigned int timeout_ms, bool is_repeat, htd_task_timeout_handle_t timeout_handle, void* param);
// @brief 启动软件定时器(不可以在中断中调用)
int htd_timer_start(void *handle);
// @brief 重启软件定时器(不可以在中断中调用)
void htd_timer_restart(void *handle, unsigned int ms);
// @brief 暂停软件定时器(不可以在中断中调用)
void htd_timer_stop(void *handle);
// @brief 销毁软件定时器(不可以在中断中调用)
void htd_timer_destroy(void* handle);
// @brief 删除软件定时器(不可以在中断中调用)
void htd_timer_delete(void* handle);
// @brief 判断软件定时器是否激活(不可以在中断中调用)
bool htd_timer_is_active(void* handle);
// @brief 获取软件定时器参数
// @param param 为定时器回调函数的参数
void* htd_timer_param_get(void* param);





/////////////////////////////////消息队列/////////////////////////////////////
typedef uint8_t* htd_mq_t;
// @brief 创建消息队列
__STATIC_INLINE htd_mq_t htd_mq_create(const char *name, uint32_t msg_size, uint32_t max_msgs)
{
    ASSERT(0);
    return NULL;
}
// @brief 销毁消息队列
__STATIC_INLINE int32_t htd_mq_destroy(htd_mq_t mq)
{
    ASSERT(0);
    return 0;
}
// @brief 发送消息
__STATIC_INLINE int32_t htd_mq_send(htd_mq_t mq, void *buffer, uint32_t size)
{
    ASSERT(0);
    return 0;
}
// @brief 发送消息
__STATIC_INLINE int32_t htd_mq_send_fisr(htd_mq_t mq, void *buffer, uint32_t size)
{
    ASSERT(0);
    return 0;
}
// @brief 等待发送
__STATIC_INLINE int32_t htd_mq_send_wait(htd_mq_t mq, void *buffer, uint32_t size, uint32_t timeout_ms)
{
    ASSERT(0);
    return 0;
}
// @brief 发送紧急消息
__STATIC_INLINE int32_t htd_mq_urgent(htd_mq_t mq, void *buffer, uint32_t size)
{
    ASSERT(0);
    return 0;
}
// @brief 接收消息
__STATIC_INLINE int32_t htd_mq_recv(htd_mq_t     mq, void *buffer, uint32_t size, int32_t timeout_ms)
{
    ASSERT(0);
    return 0;
}
// @brief 消息队列参数控制
__STATIC_INLINE int32_t htd_mq_control(htd_mq_t mq, int cmd, void *arg)
{
    ASSERT(0);
    return 0;
}
// @brief 判断队列是否为空
__STATIC_INLINE bool htd_mq_is_empty(htd_mq_t mq)
{
    ASSERT(0);
    return true;
}



/////////////////////////////////线程/////////////////////////////////////
typedef void* htd_thread_t;
// @brief 创建线程
__STATIC_INLINE htd_thread_t htd_thread_create(const char *name,
                             void (*entry)(void *parameter), void *parameter,
                             uint32_t stack_size, uint8_t  priority, uint32_t tick)
{
    ASSERT(0);
    return NULL;
}
// @brief 线程销毁
__STATIC_INLINE int32_t htd_thread_destroy(htd_thread_t thread)
{
    ASSERT(0);
    return 0;
}
// @brief 获取自身线程句柄
__STATIC_INLINE htd_thread_t htd_thread_self(void)
{
    ASSERT(0);
    return NULL;
}
// @brief 线程启动
__STATIC_INLINE int32_t htd_thread_startup(htd_thread_t thread)
{
    ASSERT(0);
    return 0;
}
// @brief 线程退出
__STATIC_INLINE void htd_thread_exit(void)
{
    ASSERT(0);
}
// @brief 获取系统当前运行的线程
__STATIC_INLINE bool htd_thread_is_self(htd_thread_t thread)
{
    ASSERT(0);
    return false;
}
// @brief 线程查找
__STATIC_INLINE htd_thread_t htd_thread_find(char *name)
{
    ASSERT(0);
    return NULL;
}
// @brief 线程调度启动
__STATIC_INLINE void htd_thread_scheduler_start(void)
{
    
}
#endif /*defined(HTD_OS_NONOS)*/


#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_TASK_NOS_H_*/

