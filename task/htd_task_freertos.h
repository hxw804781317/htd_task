// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_TASK_FREERTOS_H_
#define _HTD_TASK_FREERTOS_H_

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


#if defined(HTD_TASK_FREERTOS)
//////////////////////////////////////////////////////////////////////
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"
#include "queue.h"
#include "list.h"


// @brief 毫秒转tick数
__STATIC_INLINE uint32_t ms2tick(uint32_t ms)
{
    return pdMS_TO_TICKS(ms);
}
// @brief 进去保护区(禁止系统中断)
__STATIC_INLINE void htd_critical_enter(void)
{
    taskENTER_CRITICAL();
}
// @brief 退出保护区(使能系统中断)
__STATIC_INLINE void htd_critical_exit(void)
{
    taskEXIT_CRITICAL();
}

// @brief 进去保护区(禁止系统中断)
__STATIC_INLINE uint32_t htd_interrupt_disable(void)
{
    taskDISABLE_INTERRUPTS();
    return 0;
}
// @brief 退出保护区(使能系统中断)
__STATIC_INLINE void htd_interrupt_enable(uint32_t level)
{
    taskENABLE_INTERRUPTS();
}


/////////////////////////////////唤醒/////////////////////////////////////
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



/////////////////////////////////定时器/////////////////////////////////////
#define USE_TIMER_LIST_CMD          0

typedef struct tmrTimerControl htd_timer_ctrl_t;
typedef struct {
    void (*timer_handler_cb)(void*);
    void* arg;
    htd_timer_ctrl_t* timer_id;
} timer_param_t;

// @brief 创建定时器
// @return 成功返回定时器操作句柄，失败返回NULL
void* htd_timer_create(const char* name, unsigned int ms, bool is_repeat, void (*timer_handler_cb)(void*), void* param);
// @brief 销毁定时器
void htd_timer_destroy(void* handle);
// @brief 启动定时器
// @return 成功返回0，失败返回-1
int htd_timer_start(void *handle);
// @brief 重启定时器
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


/**
 * @brief 注册一个软件定时器到全局列表(用于调试查看)
 * @param handle      定时器句柄
 * @param name        定时器名称（需为静态字符串或持久化内存）
 * @param period      定时器周期（ticks）
 * @param autoReload  是否自动重载（pdTRUE/pdFALSE）
 */
void freertos_timer_register(TimerHandle_t handle);
/**
 * @brief 从全局列表中注销一个软件定时器
 * @param handle 定时器句柄
 */
void freertos_timer_unregister(TimerHandle_t handle);





/////////////////////////////////信号量/////////////////////////////////////
typedef StaticSemaphore_t htd_sem_t;
// @brief 创建信号量
htd_sem_t* htd_sem_create(const char* p_name, uint32_t init_count, uint32_t max_count);
// @brief 删除信号量
bool htd_sem_destroy(htd_sem_t* sem);
// @brief 查看信号量(减一); 
bool htd_sem_take(htd_sem_t* sem, uint32_t wait_ms);
// @brief 设置信号量(加一)
bool htd_sem_give(htd_sem_t* sem);
// @brief 设置信号量(加-)
bool htd_sem_give_fisr(htd_sem_t* sem);
// @brief 重置信号量
void htd_sem_reset(htd_sem_t* sem);




/////////////////////////////////信号量锁/////////////////////////////////////
typedef StaticSemaphore_t htd_sem_lock_t;
// @brief 创建信号量锁
htd_sem_lock_t* htd_sem_lock_create(const char* p_name);
// @brief 销毁信号量锁
void htd_sem_lock_destroy(htd_sem_lock_t* p_sem_lock);
// @brief 加锁(-1)
void htd_sem_lock(htd_sem_lock_t* p_sem_lock);
// @brief 解锁(+1)
void htd_sem_unlock(htd_sem_lock_t* p_sem_lock);




/////////////////////////////////互斥锁/////////////////////////////////////
typedef StaticSemaphore_t htd_mutex_lock_t;
// @brief 创建互斥锁
htd_mutex_lock_t* htd_mutex_lock_create(const char* p_name);
// @brief 销毁锁
void htd_mutex_lock_destroy(htd_mutex_lock_t* p_lock);
// @brief 加锁
void htd_mutex_lock(htd_mutex_lock_t* p_lock);
// @brief 解锁
void htd_mutex_unlock(htd_mutex_lock_t* p_lock);



/////////////////////////////////消息队列/////////////////////////////////////
#define USE_MSG_QUEUE_LIST_CMD          0

typedef QueueHandle_t htd_mq_t;

/**
 * @brief 注册一个消息队列到全局列表(用来调试跟踪当前的消息队列)
 * @param handle     队列句柄
 * @param name       队列名称（需为静态字符串或持久化内存）
 * @param itemSize   消息大小（字节）
 * @param queueLength队列长度（容量）
 */
void freertos_msgqueue_register(QueueHandle_t handle, const char *name, uint16_t itemSize, uint16_t queueLength);
/**
 * @brief 从全局列表中注销一个消息队列
 * @param handle 队列句柄
 */
void freertos_msgqueue_unregister(QueueHandle_t handle);



// @brief 创建消息队列(不能在中断中调用)
__STATIC_INLINE htd_mq_t htd_mq_create(const char *name, uint32_t msg_size, uint32_t max_msgs)
{
    QueueHandle_t handle = xQueueCreate(max_msgs, msg_size);
#if defined(USE_MSG_QUEUE_LIST_CMD) && USE_MSG_QUEUE_LIST_CMD == 1
    freertos_msgqueue_register(handle, name, msg_size, max_msgs);
#endif /*defined(USE_MSG_QUEUE_LIST_CMD) && USE_MSG_QUEUE_LIST_CMD == 1*/
    return handle;
}
// @brief 销毁消息队列(不能在中断中调用)
__STATIC_INLINE int32_t htd_mq_destroy(htd_mq_t mq)
{
#if defined(USE_MSG_QUEUE_LIST_CMD) && USE_MSG_QUEUE_LIST_CMD == 1
    freertos_msgqueue_unregister(mq);
#endif /*defined(USE_MSG_QUEUE_LIST_CMD) && USE_MSG_QUEUE_LIST_CMD == 1*/
    vQueueDelete(mq);
    return 0;
}
// @brief 发送消息(不能在中断中调用)
// @return 成功返回0，失败返回-1
__STATIC_INLINE int32_t htd_mq_send(htd_mq_t mq, void *buffer, uint32_t size)
{
    return (xQueueSend(mq, buffer, 0) ? 0 : (-1));
}
// @brief 等待发送
__STATIC_INLINE int32_t htd_mq_send_wait(htd_mq_t mq, void *buffer, uint32_t size, uint32_t timeout_ms)
{
    timeout_ms = ((timeout_ms == 0xffffffff) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms));
    return (xQueueSend(mq, buffer, timeout_ms) ? 0 : (-1));
}
// @brief 发送消息(中断中调用)
__STATIC_INLINE int32_t htd_mq_send_fisr(htd_mq_t mq, void* buffer, uint32_t size)
{
    BaseType_t xHigherPriorityTaskWoken = 0;
    if (xQueueSendFromISR(mq, buffer, &xHigherPriorityTaskWoken)) {
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        return 0;
    }
    return -1;
}
// @brief 发送紧急消息
__STATIC_INLINE int32_t htd_mq_urgent(htd_mq_t mq, void *buffer, uint32_t size)
{
    return (xQueueSendToFront(mq, buffer, 0) ? 0 : (-1));
}
// @brief 接收消息
__STATIC_INLINE int32_t htd_mq_recv(htd_mq_t     mq, void *buffer, uint32_t size, int32_t timeout_ms)
{
    TickType_t ticks = ((timeout_ms == -1) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms));
    return (xQueueReceive(mq, buffer, ticks) ? 0 : (-1));
}
// @brief 消息队列参数控制
__STATIC_INLINE int32_t htd_mq_control(htd_mq_t mq, int cmd, void *arg)
{
    return 0;
}
// @brief 判断队列是否为空
__STATIC_INLINE bool htd_mq_is_empty(htd_mq_t mq)
{
    return (uxQueueMessagesWaiting(mq) == 0);
    //return xQueueIsQueueEmptyFromISR(mq);
}



/////////////////////////////////线程/////////////////////////////////////
typedef TaskHandle_t htd_thread_t;
// @brief 创建线程
// @param priority 线程优先级(freeRTOS 数值越高，优先级越高)
__STATIC_INLINE htd_thread_t htd_thread_create(const char *name, void (*entry)(void *parameter), void *parameter,
                                                      uint32_t stack_size, uint8_t priority, uint32_t tick)
{
    TaskHandle_t xHandle = NULL;
    if (pdPASS == xTaskCreate(entry, name, stack_size / sizeof(StackType_t), parameter, priority, &xHandle)) {
        return xHandle;
    }
    return NULL;
}
// @brief 线程销毁
__STATIC_INLINE int32_t htd_thread_destroy(htd_thread_t thread)
{
    vTaskDelete(thread);
    return 0;
}
// @brief 获取自身线程句柄
__STATIC_INLINE htd_thread_t htd_thread_self(void)
{
    return xTaskGetCurrentTaskHandle();
}
// @brief 线程启动
__STATIC_INLINE int32_t htd_thread_startup(htd_thread_t thread)
{
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
    return ((xTaskGetCurrentTaskHandle() == thread) ? true : false);
}
// @brief 线程查找
__STATIC_INLINE htd_thread_t htd_thread_find(char *name)
{
    return xTaskGetHandle(name);
}

// @brief 线程调度启动
__STATIC_INLINE void htd_thread_scheduler_start(void)
{
    vTaskStartScheduler(); 
}

#endif /*HTD_TASK_FREERTOS*/
#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_TASK_FREERTOS_H_*/

