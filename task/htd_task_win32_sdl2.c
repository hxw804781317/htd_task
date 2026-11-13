// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL 4
#define DEBUG_TAG              "task.sdl2"
#include "htd_task.h"
#include "htd_task_win32_sdl2.h"

#if defined(HTD_TASK_WIN32_SDL2)
//////////////////////////////////////////////////////////////////////
// @brief 修正系统 ticks
void htd_task_step_tick(uint32_t sleep_ms)
{

}

// @brief 延迟us
void htd_delay_us(uint32_t us)
{
    if (us == 0) {
        return ;
    }

    LARGE_INTEGER freq, start, end;
    QueryPerformanceFrequency(&freq); // 获取定时器频率
    QueryPerformanceCounter(&start);
    LONGLONG target = start.QuadPart + us * freq.QuadPart / 1000000LL;
    do {
        QueryPerformanceCounter(&end);
    } while (end.QuadPart < target);
}

// @brief 线程睡眠
void htd_sleep_ms(uint32_t ms)
{
    SDL_Delay(ms);
}

// @brief 获取系统时间，单位毫秒
uint32_t htd_time_ms_get(void)
{
    //return SDL_GetTicks();
    return GetTickCount();
}

// @brief 获取系统时间，单位毫秒(中断中调用)
uint32_t htd_time_ms_get_fisr(void)
{
    return htd_time_ms_get();
}




//////////////////////////////////////////////////////////////////////
// 定义定时器结构体
typedef struct {
    char name[32];
    uint32_t interval_ms;
    bool is_repeat;
    uint16_t flag;
    SDL_TimerID id;
    void (*timeout_handle)(void*);
    void* param;
}sdl2_timer_t;
#define SDL2_TIMER_FLAG     0x232a

// @brief 定时器回调句柄
static uint32_t win32_sdl2_timer_handle(Uint32 interval, void *param)
{
    sdl2_timer_t* p_timer = (sdl2_timer_t*)param;
    if (p_timer->flag == SDL2_TIMER_FLAG) {
        p_timer->timeout_handle(p_timer->param);
        return p_timer->interval_ms;
    }
    return 0;
}
// @brief 创建定时器
void* htd_timer_create(const char* name, unsigned int ms, bool is_repeat, void (*timer_handler_cb)(void*), void* param)
{
    sdl2_timer_t* p_timer = htd_malloc(sizeof(sdl2_timer_t));
    ASSERT(p_timer);
    htd_memset(p_timer, 0, sizeof(sdl2_timer_t));
    strncpy(p_timer->name, name, sizeof(p_timer->name));
    p_timer->interval_ms = ms;
    p_timer->is_repeat = is_repeat;
    p_timer->timeout_handle = timer_handler_cb;
    p_timer->param = param;
    p_timer->id = 0;
    return p_timer;
}
// @brief 销毁定时器
void htd_timer_destroy(void* handle)
{
    sdl2_timer_t* p_timer = (sdl2_timer_t*)handle;
    if (SDL_RemoveTimer(p_timer->id)) {
        p_timer->flag = 0;
        htd_free(p_timer);
        return ;
    }
    ASSERT(0);
}
// @brief 启动定时器
// @return 成功返回0，失败返回-1
int htd_timer_start(void *handle)
{
    sdl2_timer_t* p_timer = (sdl2_timer_t*)handle;
    p_timer->id = SDL_AddTimer(p_timer->interval_ms, win32_sdl2_timer_handle, handle);
    if (p_timer->id == 0) {
        return -1;
    }
    p_timer->flag = SDL2_TIMER_FLAG;
    return 0;
}
// @brief 重启定时器
// @return 成功返回0，失败返回-1
void htd_timer_restart(void *handle, unsigned int ms)
{
    sdl2_timer_t* p_timer = (sdl2_timer_t*)handle;
    if (p_timer->id != 0) {
        SDL_RemoveTimer(p_timer->id);
    }
    p_timer->interval_ms = ms;
    p_timer->id = SDL_AddTimer(p_timer->interval_ms, win32_sdl2_timer_handle, handle);
    if (p_timer->id == 0) {
        return -1;
    }
    p_timer->flag = SDL2_TIMER_FLAG;
    return 0;
}
// @brief 暂停定时器
// @return 成功返回0，失败返回-1
void htd_timer_stop(void *handle)
{
    sdl2_timer_t* p_timer = (sdl2_timer_t*)handle;
    p_timer->flag = 0;
    return SDL_RemoveTimer(p_timer->id) ? 0 : -1;
}
// @brief 删除定时器
void htd_timer_delete(void* handle)
{
    htd_timer_destroy(handle);
}
// @brief 判断定时器是否激活
bool htd_timer_is_active(void* handle)
{
    sdl2_timer_t* p_timer = (sdl2_timer_t*)handle;
    return p_timer->flag == SDL2_TIMER_FLAG;
}
// @brief 获取软件定时器参数
// @param param 为定时器回调函数的参数
void* htd_timer_param_get(void* param)
{
    return param;
}



//////////////////////////////////////////////////////////////////////
// @brief 创建消息队列(不能在中断中调用)
htd_msg_queue_t* htd_mq_create(const char *name, uint32_t msg_size, uint32_t max_msgs)
{
    htd_msg_queue_t* p_mq = (htd_msg_queue_t*)htd_malloc(sizeof(htd_msg_queue_t) + msg_size*max_msgs);
    ASSERT(p_mq);
    htd_memset(p_mq, 0, sizeof(htd_msg_queue_t));
    p_mq->msg_buff = (uint8_t*)(p_mq + 1);
    p_mq->lock = htd_mutex_lock_create(name);
    ASSERT(p_mq->lock);
    p_mq->sem = htd_sem_create(name, 0, 1);
    ASSERT(p_mq->sem);
    p_mq->msg_size = msg_size;
    p_mq->msg_max = max_msgs;
    return p_mq;
}
// @brief 销毁消息队列(不能在中断中调用)
int32_t htd_mq_destroy(htd_msg_queue_t* p_mq)
{
    ASSERT(p_mq);
    htd_mutex_lock(p_mq->lock);
    htd_sem_destroy(p_mq->sem);
    p_mq->sem = NULL;
    htd_mutex_unlock(p_mq->lock);
    
    htd_mutex_lock_destroy(p_mq->lock);
    p_mq->lock = NULL;
    htd_free(p_mq);
    return 0;
}

// @brief 等待发送
int32_t htd_mq_send_wait(htd_msg_queue_t* p_mq, void *buffer, uint32_t size, uint32_t timeout_ms)
{
    ASSERT(p_mq);
    ASSERT(p_mq->msg_size >= size);
    uint32_t wpos, length;
    htd_mutex_lock(p_mq->lock);
    length = p_mq->msg_wpos - p_mq->msg_rpos;
    if (length >= p_mq->msg_max) {
        p_mq->msg_rpos++;
    }
    wpos = p_mq->msg_wpos % p_mq->msg_max;
    htd_memcpy(&p_mq->msg_buff[wpos * p_mq->msg_size], buffer, size);
    p_mq->msg_wpos++;
    htd_mutex_unlock(p_mq->lock);
    htd_sem_give(p_mq->sem);
    return 0;
}

// @brief 发送消息(不能在中断中调用)
// @return 成功返回0，失败返回-1
int32_t htd_mq_send(htd_msg_queue_t* p_mq, void *buffer, uint32_t size)
{
    return htd_mq_send_wait(p_mq, buffer, size, 0);
}

// @brief 发送消息(中断中调用)
int32_t htd_mq_send_fisr(htd_msg_queue_t* p_mq, void* buffer, uint32_t size)
{
    return htd_mq_send(p_mq, buffer, size);
}

// @brief 发送紧急消息
int32_t htd_mq_urgent(htd_msg_queue_t* p_mq, void *buffer, uint32_t size)
{
    // 不支持
    ASSERT(0);
    return htd_mq_send(p_mq, buffer, size);
}

// @brief 接收消息
int32_t htd_mq_recv(htd_msg_queue_t*     p_mq, void *buffer, uint32_t size, int32_t timeout_ms)
{
    ASSERT(p_mq);
    ASSERT(size >= p_mq->msg_size);

    // 接收信号量
    if (!htd_sem_take(p_mq->sem, timeout_ms)) {
        return -1;
    }

    uint32_t rpos;
    htd_mutex_lock(p_mq->lock);
    if (p_mq->msg_wpos <= p_mq->msg_rpos) {
        p_mq->msg_rpos = p_mq->msg_wpos = 0;
        htd_mutex_unlock(p_mq->lock);
        return -1;
    }
    rpos = p_mq->msg_rpos % p_mq->msg_max;
    htd_memcpy(buffer, &p_mq->msg_buff[rpos * p_mq->msg_size], size);
    p_mq->msg_rpos++;
    htd_mutex_unlock(p_mq->lock);
    return 0;
}

// @brief 消息队列参数控制
int32_t htd_mq_control(htd_msg_queue_t* p_mq, int cmd, void *arg)
{
    return 0;
}

// @brief 判断队列是否为空
bool htd_mq_is_empty(htd_msg_queue_t* p_mq)
{
    return p_mq->msg_wpos == p_mq->msg_rpos;
}

#endif /*defined(HTD_TASK_WIN32_SDL2)*/

