// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_TASK_WIN32_SDL2_H_
#define _HTD_TASK_WIN32_SDL2_H_

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


#if defined(HTD_TASK_WIN32_SDL2)
//////////////////////////////////////////////////////////////////////
// SDL不对main进行重命名
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_mutex.h>
#include <windows.h>

// @brief 进去保护区
__STATIC_INLINE void htd_critical_enter(void)
{
    
}
// @brief 退出保护区
__STATIC_INLINE void htd_critical_exit(void)
{
    
}

// @brief 进去保护区(禁止系统中断)
__STATIC_INLINE uint32_t htd_interrupt_disable(void)
{
    return 0;
}
// @brief 退出保护区(使能系统中断)
__STATIC_INLINE void htd_interrupt_enable(uint32_t level)
{
    
}


/////////////////////////////////唤醒/////////////////////////////////////
// @brief  禁止系统休眠锁上锁
__STATIC_INLINE void htd_wake_lock(void)
{
    
}
// @brief  禁止系统休眠锁解锁
__STATIC_INLINE void htd_wake_unlock(void)
{
    
}



/////////////////////////////////定时器/////////////////////////////////////
// @brief 创建定时器
// @return 成功返回定时器操作句柄，失败返回NULL
void* htd_timer_create(const char* name, unsigned int ms, bool is_repeat, void (*timer_handler_cb)(void*), void* param);
// @brief 销毁定时器
void htd_timer_destroy(void* handle);
// @brief 启动定时器
// @return 成功返回0，失败返回-1
int htd_timer_start(void *handle);
// @brief 重启定时器
// @return 成功返回0，失败返回-1
void htd_timer_restart(void *handle, unsigned int ms);
// @brief 暂停定时器
// @return 成功返回0，失败返回-1
void htd_timer_stop(void *handle);
// @brief 删除定时器
void htd_timer_delete(void* handle);
// @brief 判断定时器是否激活
bool htd_timer_is_active(void* handle);
// @brief 获取软件定时器参数
// @param param 为定时器回调函数的参数
void* htd_timer_param_get(void* param);





/////////////////////////////////信号量/////////////////////////////////////
typedef SDL_sem htd_sem_t;

// @brief 创建信号量
__STATIC_INLINE htd_sem_t* htd_sem_create(const char* p_name, uint32_t init_count, uint32_t max_count)
{
    return (htd_sem_t*)SDL_CreateSemaphore(init_count);
}
// @brief 删除信号量
__STATIC_INLINE bool htd_sem_destroy(htd_sem_t* sem)
{
    SDL_DestroySemaphore(sem);
    return 1;
}
// @brief 查看信号量(减一); 
__STATIC_INLINE bool htd_sem_take(htd_sem_t* sem, uint32_t wait_ms)
{
    return 0 == SDL_SemWaitTimeout(sem, wait_ms);
}
// @brief 设置信号量(加一)
__STATIC_INLINE bool htd_sem_give(htd_sem_t* sem)
{
    return SDL_SemPost(sem) == 0;
}
// @brief 设置信号量(加-)
__STATIC_INLINE bool htd_sem_give_fisr(htd_sem_t* sem)
{
    return htd_sem_give(sem);
}
// @brief 重置信号量，信号量重置为0
__STATIC_INLINE void htd_sem_reset(htd_sem_t* sem)
{
    while (SDL_SemTryWait(sem) == 0);
}



/////////////////////////////////信号量锁/////////////////////////////////////
typedef SDL_sem htd_sem_lock_t;


// @brief 创建信号量锁
__STATIC_INLINE htd_sem_lock_t* htd_sem_lock_create(const char* p_name)
{
    return (htd_sem_lock_t*)SDL_CreateSemaphore(1);
}
// @brief 销毁信号量锁
__STATIC_INLINE void htd_sem_lock_destroy(htd_sem_lock_t* p_sem_lock)
{
    SDL_DestroySemaphore(p_sem_lock);
}
// @brief 加锁(-1)
__STATIC_INLINE void htd_sem_lock(htd_sem_lock_t* p_sem_lock)
{
    SDL_SemWait(p_sem_lock);
}
// @brief 解锁(+1)
__STATIC_INLINE void htd_sem_unlock(htd_sem_lock_t* p_sem_lock)
{
    SDL_SemPost(p_sem_lock);
}



/////////////////////////////////互斥锁/////////////////////////////////////
typedef SDL_mutex htd_mutex_lock_t;

// @brief 创建互斥锁
__STATIC_INLINE htd_mutex_lock_t* htd_mutex_lock_create(const char* p_name)
{
    return (htd_mutex_lock_t*)SDL_CreateMutex();
}
// @brief 销毁锁
__STATIC_INLINE void htd_mutex_lock_destroy(htd_mutex_lock_t* p_lock)
{
    SDL_DestroyMutex(p_lock);
}
// @brief 加锁
__STATIC_INLINE void htd_mutex_lock(htd_mutex_lock_t* p_lock)
{
    SDL_LockMutex(p_lock);
}
// @brief 解锁
__STATIC_INLINE void htd_mutex_unlock(htd_mutex_lock_t* p_lock)
{
    SDL_UnlockMutex(p_lock);
}



/////////////////////////////////消息队列/////////////////////////////////////
typedef struct {
    htd_mutex_lock_t* lock;
    htd_sem_t* sem;
    uint8_t *msg_buff;
    uint32_t msg_rpos;
    uint32_t msg_wpos;
    uint32_t msg_size;
    uint32_t msg_max;
} htd_msg_queue_t;
typedef htd_msg_queue_t* htd_mq_t;

// @brief 创建消息队列(不能在中断中调用)
htd_mq_t htd_mq_create(const char *name, uint32_t msg_size, uint32_t max_msgs);
// @brief 销毁消息队列(不能在中断中调用)
int32_t htd_mq_destroy(htd_mq_t mq);
// @brief 发送消息(不能在中断中调用)
// @return 成功返回0，失败返回-1
int32_t htd_mq_send(htd_mq_t mq, void *buffer, uint32_t size);
// @brief 等待发送
int32_t htd_mq_send_wait(htd_mq_t mq, void *buffer, uint32_t size, uint32_t timeout_ms);
// @brief 发送消息(中断中调用)
int32_t htd_mq_send_fisr(htd_mq_t mq, void* buffer, uint32_t size);
// @brief 发送紧急消息
int32_t htd_mq_urgent(htd_mq_t mq, void *buffer, uint32_t size);
// @brief 接收消息
int32_t htd_mq_recv(htd_mq_t     mq, void *buffer, uint32_t size, int32_t timeout_ms);
// @brief 消息队列参数控制
int32_t htd_mq_control(htd_mq_t mq, int cmd, void *arg);
// @brief 判断队列是否为空
bool htd_mq_is_empty(htd_mq_t mq);



/////////////////////////////////线程/////////////////////////////////////
typedef SDL_Thread* htd_thread_t;
// @brief 创建线程
__STATIC_INLINE htd_thread_t htd_thread_create(const char *name, int (*entry)(void *parameter), void *parameter,
                                                      uint32_t stack_size, uint8_t priority, uint32_t tick)
{
    return SDL_CreateThread(entry, name, parameter);
}
// @brief 线程销毁
__STATIC_INLINE int32_t htd_thread_destroy(htd_thread_t thread)
{
    SDL_DetachThread(thread);
    return 0;
}
// @brief 获取自身线程句柄(不支持)
__STATIC_INLINE htd_thread_t htd_thread_self(void)
{
    ASSERT(0);
    return NULL;
}
// @brief 线程启动
__STATIC_INLINE int32_t htd_thread_startup(htd_thread_t thread)
{
    return 0;
}
// @brief 线程退出
__STATIC_INLINE void htd_thread_exit(void)
{
    SDL_DetachThread(htd_thread_self());
}
// @brief 获取系统当前运行的线程
__STATIC_INLINE bool htd_thread_is_self(htd_thread_t thread)
{
    return ((SDL_GetThreadID(thread) == SDL_ThreadID()) ? true : false);
}
// @brief 线程查找(不支持)
__STATIC_INLINE htd_thread_t htd_thread_find(char *name)
{
    ASSERT(0);
    return NULL;
}
// @brief 线程调度启动
__STATIC_INLINE void htd_thread_scheduler_start(void)
{
    while (1) {
        htd_sleep_ms(1000);
    }
}

#endif /*defined(HTD_TASK_WIN32_SDL2)*/
#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/
#endif /*_HTD_TASK_SDL2_H_*/

