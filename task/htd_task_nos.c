// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL       4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG       "task.nos"
#include "htd_task.h"
#include "htd_task_nos.h"

#ifdef _WIN32
    #include <windows.h>
    #include <WinBase.h>
#endif

#if defined(HTD_TASK_NONOS)
//////////////////////////////////////////////////////////////////////
// @brief 线程睡眠
void htd_sleep_ms(uint32_t ms)
{
    htd_delay_us(1000*ms);
}

// @brief 获取系统时间，单位毫秒(中断中调用)
uint32_t htd_time_ms_get_fisr(void)
{
    return htd_time_ms_get();
}

// @brief 修正系统 ticks
void htd_task_step_tick(uint32_t sleep_ms)
{
    
}


/////////////////////////////////互斥锁/////////////////////////////////////
static htd_mutex_lock_t m_mutex_lock;
// @brief 初始化互斥锁
bool htd_mutex_lock_init(htd_mutex_lock_t* p_lock, const char* p_name)
{
    ASSERT(p_lock);
    p_lock->val = 0;
    return true;
}
// @brief 去初始化互斥锁
int32_t htd_mutex_lock_deinit(htd_mutex_lock_t* p_lock)
{
    p_lock->val = 0;
    return 0;
}
// @brief 创建互斥锁
htd_mutex_lock_t* htd_mutex_lock_create(const char* p_name)
{   
    m_mutex_lock.val = 0;
    return &m_mutex_lock;
}
// @brief 销毁锁
void htd_mutex_lock_destroy(htd_mutex_lock_t* p_lock)
{
    m_mutex_lock.val = 0;
}
// @brief 加锁
void htd_mutex_lock(htd_mutex_lock_t* p_lock)
{
    uint32_t i = 0;
    while (p_lock->val) {
        htd_delay_us(100);
        i++;
        if (i >= 10*1000) {
            print_warning("mutex locking...");
            i = 0;
        }
    }
    p_lock->val++;
}
// @brief 解锁
void htd_mutex_unlock(htd_mutex_lock_t* p_lock)
{
    if (p_lock->val > 0) {
        p_lock->val--;
    }
}



/////////////////////////////////信号量/////////////////////////////////////
// @brief 创建信号量
htd_sem_t* htd_sem_create(const char* p_name, uint32_t init_count, uint32_t max_count)
{
    htd_sem_t* sem = htd_malloc(sizeof(htd_sem_t));
    ASSERT(sem);
    sem->init_count = init_count;
    sem->max_count = max_count;
    return sem;
}

// @brief 删除信号量
bool htd_sem_destroy(htd_sem_t* sem)
{
    if (sem) {
        htd_free(sem);
    }
    return 0;
}

// @brief 初始化信号量
int32_t htd_sem_init(htd_sem_t* sem, const char* p_name, uint32_t init_count, uint32_t max_count) 
{
    ASSERT(sem);
    sem->init_count = init_count;
    sem->max_count = max_count;
    return 0;
}

// @brief 去初始化信号量
int32_t htd_sem_deinit(htd_sem_t* sem)
{
    ASSERT(sem);
    htd_memset(sem, 0, sizeof(htd_sem_t));
    return 0;
}

// @brief 查看信号量(减一); 
bool htd_sem_take(htd_sem_t* sem, uint32_t wait_ms)
{
    ASSERT(sem);
    uint32_t start_ms = htd_time_ms_get();
    while (sem->init_count == 0) {
        if (htd_time_ms_diff(htd_time_ms_get(), start_ms) >= wait_ms) {
            return false;
        }
        htd_delay_us(10);
    }
    if (sem->init_count > 0) {
        sem->init_count--;
        return true;
    }
    return false;
}

// @brief 发送信号量(信号量加一)
bool htd_sem_give(htd_sem_t* sem)
{
    ASSERT(sem);
    if (sem->init_count < sem->max_count) {
        sem->init_count++;
    }
    return 1;
}

// @brief 设置信号量(加-)
bool htd_sem_give_fisr(htd_sem_t* sem)
{
    return htd_sem_give(sem);
}

// @brief 重置信号量
void htd_sem_reset(htd_sem_t* sem)
{
    ASSERT(sem);
    sem->init_count = 0;
}

#endif 


