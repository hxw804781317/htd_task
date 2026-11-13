// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
//#define DEBUG_LVL     4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG       "os.thread"
#include "htd_task.h"
#include "htd_task_thread_sch.h"


#if !defined(HTD_TASK_NONOS)
//////////////////////////////////////////////////////////////////////
// 消息队列是否支持超时唤醒
#ifndef OS_SUPPORT_MQ_TIMEROUT_WAKEUP
    #define OS_SUPPORT_MQ_TIMEROUT_WAKEUP      1
#endif

// 使用了RTOS的线程调度
// 定时队列
typedef struct {
    htd_list_node_t node;
    uint32_t reach_ms;
    uint32_t timeout_ms:31;
    uint32_t is_repeat:1;
    void *arg;
    void (*timeout_cb)(void* arg);
} os_timer_node_t;

typedef struct {
    void* p_param;
    void (*fun)(void*);
    htd_slist_node_t node;
} os_task_node_t;

// 线程消息结构体
typedef struct {
    uint16_t type;
    uint16_t param_len;
    void* p_param;
    void* fun;
} os_msg_t;

typedef struct {
    const char* p_name;             // 名字
    uint8_t mq_count;               // 消息邮箱个数
    uint8_t thread_priority;        // 线程优先级
    uint16_t thread_stack_size;     // 线程栈大小

    htd_list_head_t timer_list;
    htd_slist_t idle_slist;
    htd_slist_t urgent_slist;
    htd_thread_t thread;
    htd_mq_t mq; 
#if OS_SUPPORT_MQ_TIMEROUT_WAKEUP == 0
    void* timer_wkup;
#endif /*OS_SUPPORT_MQ_TIMEROUT_WAKEUP == 0*/
} htd_os_task_t;


//////////////////////////////////////////////////////////////////////
// @brief 判断当前执行环境是否为 vtask thread
static bool htd_os_thread_sch_is_self(void* p_object)
{
    htd_os_task_t* p_task = (htd_os_task_t*)p_object;
    ASSERT(p_task);
    return htd_thread_is_self(p_task->thread);
}

// @brief 关闭所有定时器
static void htd_os_timer_destory(htd_list_head_t* p_timer_list)
{
    os_timer_node_t *p_node;
    htd_list_node_t *p_pos, *p_temp;
    htd_list_for_each_safe(p_pos, p_temp, p_timer_list) {
        p_node = htd_list_entry(p_pos, os_timer_node_t, node);
        htd_list_del(p_pos);
        htd_free(p_node);
    }
}

#if OS_SUPPORT_MQ_TIMEROUT_WAKEUP == 0
// @brief 唤醒定时器
void htd_os_timer_wakeup_handle(htd_timer_ctrl_t * p_timer)
{
    void* p_object = htd_timer_param_get(p_timer);
    htd_os_load_function(p_object, NULL, NULL);
}
#endif /*OS_SUPPORT_MQ_TIMEROUT_WAKEUP == 0*/

// @brief 定时器超时句柄
static uint32_t htd_os_timer_detect_handle(htd_list_head_t* p_timer_list)
{
    if (htd_list_is_empty(p_timer_list)) {
        return 0xffffffff;
    }
    
    htd_list_node_t *p_pos = p_timer_list->next;
    os_timer_node_t *p_node = htd_list_entry(p_pos, os_timer_node_t, node);
    uint32_t now_ms = htd_time_ms_get();
    if (now_ms >= p_node->reach_ms && htd_time_ms_diff(now_ms, p_node->reach_ms) < 0x7fffffff) {
        htd_list_del(p_pos);
        if (p_node->is_repeat) {
            // 重复事件，重新加到队列中
            os_timer_node_t *p_find;
            uint32_t interval_ms;
            p_node->reach_ms += p_node->timeout_ms;
            htd_list_for_each(p_pos, p_timer_list) {
                p_find = htd_list_entry(p_pos, os_timer_node_t, node);
                interval_ms = htd_time_ms_diff(p_find->reach_ms, now_ms);
                if (interval_ms < 0x7fffffff && interval_ms > p_node->timeout_ms) {
                    break;
                }
            }
            htd_list_insert_before(&p_node->node, p_pos);     // 插入到p_pos前面
            // 执行
            p_node->timeout_cb(p_node->arg);
        } else {
            // 执行
            p_node->timeout_cb(p_node->arg);
            htd_free(p_node);
        }
    }
    
    if (htd_list_is_empty(p_timer_list)) {
        return 0xffffffff;
    }

    now_ms = htd_time_ms_get();
    p_node = htd_list_entry(p_timer_list->next, os_timer_node_t, node);
    uint32_t timeout_ms = htd_time_ms_diff(p_node->reach_ms, now_ms);
    if (timeout_ms > 24 * 3600 * 1000) {
        if (timeout_ms > 0x7fffffff) {
            return 0;
        }
        print_debug("timeout_ms=%lu, reach_ms=%lu, now_ms=%lu", timeout_ms, p_node->reach_ms, now_ms);
        ASSERT(0);
    }
    return timeout_ms;
}

// @brief 加载定时任务(只能在p_object对应的线程调用)
// @param timeout_ms 如果为0，is_repeat 必须为0
bool htd_os_load_timer_function(void* p_object, bool is_repeat, uint32_t timeout_ms, void (*timeout_cb)(void*), void* arg)
{
    if (timeout_ms == 0) {
        ASSERT(is_repeat == false);
        htd_os_load_function(p_object, timeout_cb, arg);
        return true;
    }

    ASSERT(timeout_ms < 24 * 3600 * 1000);
    ASSERT(htd_os_thread_sch_is_self(p_object));
    if (timeout_cb == NULL) {
        return false;
    }
    
    uint32_t now_ms = htd_time_ms_get();
    uint32_t reach_ms = now_ms + timeout_ms;

    htd_os_task_t* p_task = (htd_os_task_t*)p_object;
    os_timer_node_t* p_new = NULL;
    os_timer_node_t* p_node;
    htd_list_node_t* p_pos;
    htd_list_for_each(p_pos, &p_task->timer_list) {
        p_node = htd_list_entry(p_pos, os_timer_node_t, node);
        if (p_node->timeout_cb == timeout_cb) {
            // 相同的回调，只允许有一个
            htd_list_del(p_pos);
            p_new = p_node;
            break;
        }
    }
    if (p_new == NULL) {
        p_new = (os_timer_node_t*)htd_malloc(sizeof(os_timer_node_t));
        ASSERT(p_new);
        htd_list_head_init(&p_new->node);
    }
    p_new->reach_ms = reach_ms;
    p_new->timeout_ms = timeout_ms;
    p_new->is_repeat = is_repeat ? 1 : 0;
    p_new->timeout_cb = timeout_cb;
    p_new->arg = arg;
    
    uint32_t interval_ms;
    htd_list_for_each(p_pos, &p_task->timer_list) {
        p_node = htd_list_entry(p_pos, os_timer_node_t, node);
        interval_ms = htd_time_ms_diff(p_node->reach_ms, now_ms);
        if (interval_ms < 0x7fffffff && interval_ms > timeout_ms) {
            break;
        }
    }
    // 插入到p_pos前面
    htd_list_insert_before(&p_new->node, p_pos);
    print_debug("add (%s) timer. timeout_ms=%u, reach_ms=%u, now_ms=%u", p_task->p_name, timeout_ms, reach_ms, now_ms);

#if OS_SUPPORT_MQ_TIMEROUT_WAKEUP == 0
    if (p_task->timer_wkup == NULL) {
        p_task->timer_wkup = htd_timer_create(p_task->p_name, timeout_ms, false, htd_os_timer_wakeup_handle, p_object);
        ASSERT(p_task->timer_wkup);
        htd_timer_start(p_task->timer_wkup);
    }
#endif /*OS_SUPPORT_MQ_TIMEROUT_WAKEUP == 0*/
    return true;
}

// @brief 删除定时任务
void htd_os_unload_timer_function(void* p_object, void (*timeout_cb)(void*))
{
    ASSERT(p_object);
    ASSERT(timeout_cb);
    ASSERT(htd_os_thread_sch_is_self(p_object));

    htd_os_task_t* p_task = (htd_os_task_t*)p_object;
    os_timer_node_t* p_node;
    htd_list_node_t* p_pos;
    htd_list_for_each(p_pos, &p_task->timer_list) {
        p_node = htd_list_entry(p_pos, os_timer_node_t, node);
        if (p_node->timeout_cb == timeout_cb) {
            // 相同的回调，只允许有一个
            htd_list_del(p_pos);
            htd_free(p_node);
            break;
        }
    }
}



//////////////////////////////////////////////////////////////////////
// @brief 销毁
static void htd_os_task_slist_destroy(htd_slist_t* p_urgent_slist, htd_slist_t* p_idle_slist)
{
    os_task_node_t* p_info;
    htd_slist_node_t* p_pos;
    while (1) {
        p_pos = htd_slist_first(p_urgent_slist);
        if (p_pos == NULL) {
            break;
        }
        p_info = htd_slist_entry(p_pos, os_task_node_t, node);
        htd_slist_remove(p_urgent_slist, p_pos);
        htd_free(p_info);
    }

    while (1) {
        p_pos = htd_slist_first(p_idle_slist);
        if (p_pos == NULL) {
            break;
        }
        p_info = htd_slist_entry(p_pos, os_task_node_t, node);
        htd_slist_remove(p_idle_slist, p_pos);
        htd_free(p_info);
    }
}

// @brief 加载紧急任务(只能在对应线程内调用)
void htd_os_load_urgent_function(void* p_object, void (*fun)(void*), void* arg)
{
    ASSERT(htd_os_thread_sch_is_self(p_object));
    htd_os_task_t* p_task = (htd_os_task_t*)p_object;
    htd_slist_node_t* pos;
    os_task_node_t* p_node;
    htd_slist_for_each(pos, &p_task->urgent_slist) {
        p_node = htd_slist_entry(pos, os_task_node_t, node);
        if (p_node->fun == fun) {
            return;
        }
    }
    
    p_node = htd_malloc(sizeof(os_task_node_t));
    ASSERT(p_node);
    p_node->fun = fun;
    p_node->p_param = arg;
    htd_slist_append(&p_task->urgent_slist, &p_node->node);
    htd_os_load_function(p_object, NULL, NULL);
}

// @brief 执行紧急任务
static bool htd_os_urgent_task_ex(htd_slist_t* p_urgent_slist)
{
    htd_slist_node_t* p_pos = htd_slist_first(p_urgent_slist);
    if (p_pos) {
        os_task_node_t* p_info = htd_slist_entry(p_pos, os_task_node_t, node);
        p_info->fun(p_info->p_param);
        htd_slist_remove(p_urgent_slist, p_pos);
        htd_free(p_info);
        return true;
    }
    return false;
}

// @brief 添加空闲任务(低优先级任务，其他任务执行后才会执行这些回调函数, 只能在对应线程内调用)
void htd_os_load_idle_function(void* p_object, void (*fun)(void*), void* arg)
{
    ASSERT(htd_os_thread_sch_is_self(p_object));
    htd_os_task_t* p_task = (htd_os_task_t*)p_object;
    htd_slist_node_t* pos;
    os_task_node_t* p_node;
    htd_slist_for_each(pos, &p_task->idle_slist) {
        p_node = htd_slist_entry(pos, os_task_node_t, node);
        if (p_node->fun == fun) {
            return;
        }
    }
    
    p_node = htd_malloc(sizeof(os_task_node_t));
    ASSERT(p_node);
    p_node->fun = fun;
    p_node->p_param = arg;
    htd_slist_append(&p_task->idle_slist, &p_node->node);
    htd_os_load_function(p_object, NULL, NULL);
}

// @brief 执行空闲任务
static bool htd_os_idle_task_ex(htd_slist_t* p_idle_slist)
{
    htd_slist_node_t* p_pos = htd_slist_first(p_idle_slist);
    if (p_pos == NULL) {
        return false;
    } 
    os_task_node_t* p_info = htd_slist_entry(p_pos, os_task_node_t, node);
    p_info->fun(p_info->p_param);
    htd_slist_remove(p_idle_slist, p_pos);
    htd_free(p_info);
    return true;
}


//////////////////////////////////////////////////////////////////////
// @brief 处理函数指针处理类消息
static void os_msg_function_handle(htd_os_task_t* p_task, os_msg_t* p_msg)
{
    typedef void (*callback_fun_t)(void*);
    callback_fun_t callback_function;
    
    // 执行回调函数
    callback_function = (callback_fun_t)(p_msg->fun);
    //print_warning("[%s]load function: 0x%p. start=%u", p_task->p_name, callback_function, htd_time_ms_get());
    callback_function(p_msg->p_param);
    //print_warning("load function: 0x%p. end=%u", callback_function, htd_time_ms_get());
}

// @brief 执行函数数据
static void os_msg_function_data_handle(htd_os_task_t* p_task, os_msg_t* p_msg)
{
    typedef void (*callback_fun_t)(const uint8_t*, uint16_t);
    callback_fun_t callback_function;
    
    // 执行回调函数
    callback_function = (callback_fun_t)(p_msg->fun);
    //print_warning("[%s]load function data: 0x%p, start=%u", p_task->p_name, callback_function, htd_time_ms_get());
    callback_function((const uint8_t*)p_msg->p_param, p_msg->param_len);
    //print_warning("load function data: end=%u", htd_time_ms_get());
}

// @brief 执行函数类型数据
static void os_msg_function_type_data_handle(htd_os_task_t* p_task, os_msg_t* p_msg)
{
    typedef void (*callback_fun_t)(uint16_t, const uint8_t*, uint16_t);
    callback_fun_t callback_function;
    
    // 执行回调函数
    callback_function = (callback_fun_t)(p_msg->fun);
    //print_warning("[%s]load function type data: 0x%p, start=%u", p_task->p_name, callback_function, htd_time_ms_get());
    callback_function(p_msg->type, p_msg->p_param, p_msg->param_len);
    //print_warning("load function type data: end=%u", htd_time_ms_get());
}

// @brief app 线程入口
static void htd_os_entry_handle(void *parameter)
{
    htd_os_task_t* p_task = (htd_os_task_t*)parameter;
    ASSERT(p_task);
    
    int32_t ret;
    uint32_t waite_ms;
    os_msg_t msg;
    while (1) {
        // 获取定时任务最近的时间差
        waite_ms = htd_os_timer_detect_handle(&p_task->timer_list);
        if (waite_ms == 0) {
            continue;
        }
        
        // 执行空闲任务
        if (htd_mq_is_empty(p_task->mq)) {
            if (htd_os_idle_task_ex(&p_task->idle_slist)) {
                continue;
            }
        }
        print_debug("waite_ms=%u->0x%08x, now_ms=%u", waite_ms, waite_ms, htd_time_ms_get());

    #if OS_SUPPORT_MQ_TIMEROUT_WAKEUP == 0
        // 使用软件定时器唤醒
        if (p_task->timer_wkup && waite_ms != 0xffffffff) {
            htd_timer_restart(p_task->timer_wkup, waite_ms);
            waite_ms = 0xffffffff;
        }
    #endif /*OS_SUPPORT_MQ_TIMEROUT_WAKEUP == 0*/

        htd_memset(&msg, 0, sizeof(msg));
        ret = htd_mq_recv(p_task->mq, &msg, sizeof(msg), waite_ms);
        print_debug("finish waite. now_ms=%u", htd_time_ms_get());
        while (htd_os_urgent_task_ex(&p_task->urgent_slist));            // 执行紧急任务
        if (0 == ret) {
            if (msg.fun) {
                if (msg.type == 0) {
                    os_msg_function_handle(p_task, &msg);
                } else if (msg.type == 0xffff) {
                    os_msg_function_data_handle(p_task, &msg);
                } else {
                    os_msg_function_type_data_handle(p_task, &msg);
                }
                if (msg.p_param && msg.param_len > 0) {
                    htd_free(msg.p_param);
                }
            } else {
                if (msg.type == 0xffff) {               // 退出
                    break;
                }
            }
        }
    }

    // 销毁队列
    htd_os_task_slist_destroy(&p_task->urgent_slist, &p_task->idle_slist);
    // 销毁定时器
    htd_os_timer_destory(&p_task->timer_list);
#if OS_SUPPORT_MQ_TIMEROUT_WAKEUP == 0
    if (p_task->timer_wkup) {
        htd_timer_stop(p_task->timer_wkup);
        htd_timer_destroy(p_task->timer_wkup);
        p_task->timer_wkup = NULL;
    }
#endif /*OS_SUPPORT_MQ_TIMEROUT_WAKEUP == 0*/
    // 销毁队列
    htd_mq_destroy(p_task->mq);
    // 销毁线程
    htd_thread_destroy(p_task->thread);
    // 销毁整个任务调度器
    htd_free(p_task);
}

// @brief 发送消息
static bool htd_os_msg_send_handle(void* p_object, uint16_t type, void* fun, const void* p_param, uint16_t param_len)
{
    htd_os_task_t* p_task = (htd_os_task_t*)p_object;
    ASSERT(p_task);
    if (p_task->mq == NULL) {
        return false;
    }

    void* p_ptr = NULL;
    os_msg_t msg;
    htd_memset(&msg, 0, sizeof(os_msg_t));
    msg.type = type;
    msg.fun = fun;
    msg.param_len = param_len;
    msg.p_param = (void*)p_param;
    if (p_param != NULL && param_len > 0) {
        p_ptr = htd_malloc(ALIGN_UP(param_len, 4));
        ASSERT(p_ptr);
        htd_memcpy(p_ptr, p_param, param_len);
        msg.p_param = p_ptr;
    }
    if (0 == htd_mq_send(p_task->mq, &msg, sizeof(os_msg_t))) {
        return true;
    }
    if (p_ptr) {
        htd_free(p_ptr);
    }
    ASSERT(0);
    print_error("send msg failed. type=%d, fun=0x%p, p_param=0x%p, param_len=%d", type, fun, p_param, param_len);
    return false;
}

// @brief 加载函数
bool htd_os_load_function(void* p_object, void (*fun)(void*), void* arg)
{
    return htd_os_msg_send_handle(p_object, 0, (void*)fun, arg, 0);
}

// @brief 加载函数(可以在中断中调用)
bool htd_os_load_function_fisr(void* p_object, void (*fun)(void*), void* arg)
{
    htd_os_task_t* p_task = (htd_os_task_t*)p_object;
    ASSERT(p_task);
    if (p_task->mq == NULL) {
        return false;
    }

    os_msg_t msg;
    htd_memset(&msg, 0, sizeof(os_msg_t));
    msg.type = 0;
    msg.fun = fun;
    msg.param_len = 0;
    msg.p_param = arg;
    if (0 == htd_mq_send_fisr(p_task->mq, &msg, sizeof(os_msg_t))) {
        return true;
    }
    ASSERT(0);
    return false;
}


// @brief 加载函数数据
bool htd_os_load_function_data(void* p_object, void (*fun)(const uint8_t*, uint16_t), const void* p_data, uint16_t data_len)
{
    return htd_os_msg_send_handle(p_object, 0xffff, (void*)fun, p_data, data_len);
}

// @brief 加载类型函数数据
bool htd_os_load_function_type_data(void* p_object, void (*fun)(uint16_t, const uint8_t*, uint16_t), uint16_t type, const uint8_t* p_data, uint16_t data_len)
{
    if (type == 0 || type == 0xffff) {
        print_error("load function type failed. type=0x%04x", type);
        ASSERT(0);
        return false;
    }
    return htd_os_msg_send_handle(p_object, type, (void*)fun, p_data, data_len);
}

// @brief 初始化分发线程
// @return 返回 p_object 为其他接口函数的第一个参数
void* htd_os_sch_init(const char* p_name, uint8_t mq_count, uint8_t thread_priority, uint32_t thread_stack_size)
{
    htd_os_task_t* p_task = (htd_os_task_t*)htd_malloc(sizeof(htd_os_task_t));
    ASSERT(p_task);
    htd_memset(p_task, 0, sizeof(htd_os_task_t));
    p_task->p_name = p_name;
    htd_list_head_init(&p_task->timer_list);
    htd_slist_init(&p_task->idle_slist);
    htd_slist_init(&p_task->urgent_slist);
    p_task->mq = htd_mq_create(p_name, sizeof(os_msg_t), mq_count);
    ASSERT(p_task->mq);

    p_task->thread = htd_thread_create(p_name, htd_os_entry_handle, p_task, thread_stack_size, thread_priority, 1);
    ASSERT(p_task->thread);
    
    int32_t ret = htd_thread_startup(p_task->thread);
    ASSERT(0 == ret);
    return p_task;
}

// @brief 去初始化线程
void htd_os_sch_uninit(void* p_object)
{
    htd_os_msg_send_handle(p_object, 0xffff, NULL, NULL, 0);
}
#endif /*!defined(BSP_USING_NONOS)*/

