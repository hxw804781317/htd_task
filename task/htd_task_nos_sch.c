// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL       2       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG       "task.nos_sch"
#include "htd_task.h"
#include "htd_task_sch.h"


#if defined(HTD_TASK_NONOS)
//////////////////////////////////////////////////////////////
// 无RTOS的任务调用
// 定时队列
typedef struct {
    htd_list_node_t node;
    uint32_t id;
    uint32_t reach_ms;
    uint32_t timeout_ms:31;
    uint32_t is_repeat:1;
    void *arg;
    void (*timeout_cb)(void* arg);
} nonos_timer_node_t;

typedef struct {
    void* p_param;
    void (*fun)(void*);
    htd_slist_node_t node;
} nonos_node_t;

typedef struct {
    void* func_handle;
    void* p_param;
    uint16_t param_len;
    uint16_t type;
} nonos_func_t;

typedef struct {
    nonos_func_t tasks[32];
    volatile size_t rpos;
    volatile size_t wpos;
} htd_nonos_fifo_t;
#define HTD_TASK_FIFO_MASK         (ARRAY_COUNT(((htd_nonos_fifo_t*)0)->tasks) - 1)
static htd_nonos_fifo_t m_task_fifo;
static htd_slist_t m_idle_slist;
static htd_slist_t m_urgent_slist;
static htd_list_head_t m_timer_list;
static htd_list_head_t m_timer_standby_list;
static uint32_t m_timer_id = 0;


// @brief 关闭所有定时器
void htd_nonos_timer_destory(htd_list_head_t* p_timer_list)
{
    nonos_timer_node_t *p_node;
    htd_list_node_t *p_pos, *p_temp;
    htd_list_for_each_safe(p_pos, p_temp, p_timer_list) {
        p_node = htd_list_entry(p_pos, nonos_timer_node_t, node);
        htd_list_del(p_pos);
        htd_free(p_node);
    }
}

// @brief 定时器超时句柄
static uint32_t htd_nonos_timer_ex(htd_list_head_t* p_timer_list)
{
    if (htd_list_is_empty(p_timer_list)) {
        return 0xffffffff;
    }

    uint32_t now_ms = htd_time_ms_get();
    htd_list_node_t *p_pos = p_timer_list->next;
    nonos_timer_node_t *p_node = htd_list_entry(p_pos, nonos_timer_node_t, node);
    if (now_ms >= p_node->reach_ms && htd_time_ms_diff(now_ms, p_node->reach_ms) < 0x7fffffff) {
        htd_list_del(p_pos);
        if (p_node->is_repeat) {
            // 重复事件，重新加到队列中
            nonos_timer_node_t *p_find;
            uint32_t interval_ms;
            p_node->reach_ms += p_node->timeout_ms;
            htd_list_for_each(p_pos, p_timer_list) {
                p_find = htd_list_entry(p_pos, nonos_timer_node_t, node);
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

    // 获取第一个超时时间
    uint32_t timeout_ms;
    now_ms = htd_time_ms_get();
    p_node = htd_list_entry(p_timer_list->next, nonos_timer_node_t, node);
    timeout_ms = htd_time_ms_diff(p_node->reach_ms, now_ms);
    if (timeout_ms > 24 * 3600 * 1000) {
        if (timeout_ms > 0x7fffffff) {
            return 0;
        }
        print_debug("timeout_ms=%lu, reach_ms=%lu, now_ms=%lu", timeout_ms, p_node->reach_ms, now_ms);
        ASSERT(0);
    }
    return timeout_ms;
}

// @brief 加载定时任务(不可以在中断中调用)
// @param timeout_ms 如果为0，is_repeat 必须为0
bool htd_task_load_timer_function(bool is_repeat, uint32_t timeout_ms, htd_task_timeout_handle_t handle, void* arg)
{
    if (timeout_ms == 0) {
        ASSERT(is_repeat == false);
        htd_task_load_function(handle, arg);
        return true;
    }

    ASSERT(timeout_ms < 24 * 3600 * 1000);
    if (handle == NULL) {
        return false;
    }
    
    uint32_t now_ms = htd_time_ms_get();
    uint32_t reach_ms = now_ms + timeout_ms;
    print_debug("now_ms=%d, reach_ms=%d", now_ms, reach_ms);

    nonos_timer_node_t* p_new = NULL;
    nonos_timer_node_t* p_node;
    htd_list_node_t* p_pos;
    htd_list_for_each(p_pos, &m_timer_list) {
        p_node = htd_list_entry(p_pos, nonos_timer_node_t, node);
        if (p_node->timeout_cb == handle) {
            // 相同的回调，只允许有一个
            htd_list_del(p_pos);
            p_new = p_node;
            break;
        }
    }
    if (p_new == NULL) {
        p_new = (nonos_timer_node_t*)htd_malloc(sizeof(nonos_timer_node_t));
        ASSERT(p_new);
        htd_list_head_init(&p_new->node);
    }
    p_new->id = m_timer_id++;
    p_new->is_repeat = is_repeat ? 1 : 0;
    p_new->timeout_ms = timeout_ms;
    p_new->reach_ms = reach_ms;
    p_new->timeout_cb = handle;
    p_new->arg = arg;

    uint32_t interval_ms;
    htd_list_for_each(p_pos, &m_timer_list) {
        p_node = htd_list_entry(p_pos, nonos_timer_node_t, node);
        interval_ms = htd_time_ms_diff(p_node->reach_ms, now_ms);
        if (interval_ms < 0x7fffffff && interval_ms > timeout_ms) {
            break;
        }
    }
    // 插入到p_pos前面
    htd_list_insert_before(&p_new->node, p_pos);
    print_debug("add timer. timeout_ms=%u, reach_ms=%u, now_ms=%u", timeout_ms, reach_ms, now_ms);

    // 第一个
    if (p_pos == m_timer_list.next) {
        htd_task_load_function(NULL, NULL);
    }
    return true;
}

// @brief 删除定时任务
void htd_task_unload_timer_function(htd_task_timeout_handle_t handle)
{
    nonos_timer_node_t* p_node;
    htd_list_node_t* p_pos;
    htd_list_for_each(p_pos, &m_timer_list) {
        p_node = htd_list_entry(p_pos, nonos_timer_node_t, node);
        if (p_node->timeout_cb == handle) {
            // 相同的回调，只允许有一个
            htd_list_del(p_pos);
            htd_free(p_node);
            break;
        }
    }
}

// @brief 执行紧急任务
static bool htd_nonos_urgent_ex(htd_slist_t* p_urgent_slist)
{
    htd_slist_node_t* p_pos = htd_slist_first(p_urgent_slist);
    if (p_pos) {
        nonos_node_t* p_info = htd_slist_entry(p_pos, nonos_node_t, node);
        p_info->fun(p_info->p_param);
        htd_slist_remove(p_urgent_slist, p_pos);
        htd_free(p_info);
        return true;
    }
    return false;
}

// @brief 加载紧急任务(不可以在中断中调用)
void htd_task_load_urgent_function(htd_task_timeout_handle_t handle, void* arg)
{
    ASSERT(handle);

    htd_slist_node_t* pos;
    nonos_node_t* p_node;
    htd_slist_for_each(pos, &m_urgent_slist) {
        p_node = htd_slist_entry(pos, nonos_node_t, node);
        if (p_node->fun == handle) {
            return;
        }
    }
    
    p_node = htd_malloc(sizeof(nonos_node_t));
    ASSERT(p_node);
    p_node->fun = handle;
    p_node->p_param = arg;
    htd_slist_append(&m_urgent_slist, &p_node->node);
}

// @brief 执行空闲任务
static bool htd_nonos_idle_ex(htd_slist_t* p_idle_slist)
{
    htd_slist_node_t* p_pos = htd_slist_first(p_idle_slist);
    if (p_pos == NULL) {
        return false;
    } 
    nonos_node_t* p_info = htd_slist_entry(p_pos, nonos_node_t, node);
    p_info->fun(p_info->p_param);
    htd_slist_remove(p_idle_slist, p_pos);
    htd_free(p_info);
    return true;
}

// @brief 添加空闲任务(最低优先级任务，其他任务执行后才会执行这些回调函数, 不可以在中断中调用)
void htd_task_load_idle_function(htd_task_timeout_handle_t handle, void* arg)
{
    ASSERT(handle);
    htd_slist_node_t* pos;
    nonos_node_t* p_node;
    htd_slist_for_each(pos, &m_idle_slist) {
        p_node = htd_slist_entry(pos, nonos_node_t, node);
        if (p_node->fun == handle) {
            return;
        }
    }
    
    p_node = htd_malloc(sizeof(nonos_node_t));
    ASSERT(p_node);
    p_node->fun = handle;
    p_node->p_param = arg;
    htd_slist_append(&m_idle_slist, &p_node->node);
}

// @brief 加载函数(可以在中断中调用)
bool htd_task_load_function(htd_task_timeout_handle_t handle, void* arg)
{
    size_t rpos, wpos;
    uint32_t level = htd_interrupt_disable();
    rpos = m_task_fifo.rpos;
    wpos = m_task_fifo.wpos;
    if ((wpos - rpos) > HTD_TASK_FIFO_MASK) {
        htd_interrupt_enable(level);
        return false;
    }
    m_task_fifo.wpos = wpos + 1;        // 先占位
    htd_interrupt_enable(level);

    // 后操作
    nonos_func_t* p_task = &m_task_fifo.tasks[wpos & HTD_TASK_FIFO_MASK];
    p_task->type = 0;
    p_task->func_handle = (void*)handle;
    p_task->p_param = arg;
    return true;
}

// @brief 加载函数(可以在中断中调用)
bool htd_task_load_function_fisr(htd_task_timeout_handle_t handle, void* arg)
{
    return htd_task_load_function(handle, arg);
}

// @brief 加载函数数据(不可以在中断中调用)
bool htd_task_load_function_data(htd_task_data_handle_t handle, const void* p_data, uint16_t data_len)
{
    size_t rpos, wpos;

    uint32_t level = htd_interrupt_disable();
    rpos = m_task_fifo.rpos;
    wpos = m_task_fifo.wpos;
    if ((wpos - rpos) > HTD_TASK_FIFO_MASK) {
        htd_interrupt_enable(level);
        return false;
    }
    m_task_fifo.wpos = wpos + 1;    // 先占位
    htd_interrupt_enable(level);
    
    nonos_func_t* p_task = &m_task_fifo.tasks[wpos & HTD_TASK_FIFO_MASK];
    htd_memset(p_task, 0, sizeof(nonos_func_t));
    p_task->type = 0xffff;
    p_task->func_handle = (void*)handle;
    if (p_data && data_len) {
        p_task->p_param = htd_malloc(data_len);
        ASSERT(p_task->p_param);
        htd_memcpy(p_task->p_param, p_data, data_len);
        p_task->param_len = data_len;
    }
    return true;
}

// @brief 加载类型函数数据(不可以在中断中调用)
bool htd_task_load_function_type_data(htd_task_data_type_handle_t handle, uint16_t type, const uint8_t* p_data, uint16_t data_len)
{
    size_t rpos, wpos;
    ASSERT(type != 0 && type != 0xffff && type != 0xfffe);

    uint32_t level = htd_interrupt_disable();
    rpos = m_task_fifo.rpos;
    wpos = m_task_fifo.wpos;
    if ((wpos - rpos) > HTD_TASK_FIFO_MASK) {
        htd_interrupt_enable(level);
        return false;
    }
    m_task_fifo.wpos = wpos + 1;        // 先占位
    htd_interrupt_enable(level);
    
    nonos_func_t* p_task = &m_task_fifo.tasks[wpos & HTD_TASK_FIFO_MASK];
    htd_memset(p_task, 0, sizeof(nonos_func_t));
    p_task->type = type;
    p_task->func_handle = (void*)handle;
    if (p_data && data_len) {
        p_task->p_param = htd_malloc(data_len);
        ASSERT(p_task->p_param);
        htd_memcpy(p_task->p_param, p_data, data_len);
        p_task->param_len = data_len;
    }
    return true;
}

// @brief 弹出一个任务
static int32_t htd_nonos_pop(nonos_func_t* p_info)
{
    size_t rpos = m_task_fifo.rpos;
    size_t wpos = m_task_fifo.wpos;
    if (rpos >= wpos) {
        return -1;
    }

    nonos_func_t* p_task = &m_task_fifo.tasks[rpos & HTD_TASK_FIFO_MASK];
    htd_memcpy(p_info, p_task, sizeof(nonos_func_t));
    m_task_fifo.rpos++;
    return 0;
}

// @brief 系统循环调度
void htd_task_sch_loop(htd_task_sleep_handle_t sleep_handle)
{
    uint32_t waite_ms;
    bool sleep_flag;
    nonos_func_t task;
    for (;;) {
        while (htd_nonos_urgent_ex(&m_urgent_slist));          // 执行紧急事件
        if (0 == htd_nonos_pop(&task)) {        // 执行队列事件
            if (task.type == 0) {
                htd_task_timeout_handle_t handle = (htd_task_timeout_handle_t)task.func_handle;
                print_log("func[0x%08x] start. time_ms=%u", handle, htd_time_ms_get());
                handle(task.p_param);
                print_log("func end. time_ms=%u", htd_time_ms_get());
            } else if (task.type == 0xffff) {
                htd_task_data_handle_t handle = (htd_task_data_handle_t)task.func_handle;
                print_log("func[0x%08x] start. time_ms=%u", handle, htd_time_ms_get());
                handle(task.p_param, task.param_len);
                print_log("func end. time_ms=%u", htd_time_ms_get());
                if (task.p_param) {
                    htd_free(task.p_param);
                }
            } else {
                htd_task_data_type_handle_t handle = (htd_task_data_type_handle_t)task.func_handle;
                print_log("func[0x%08x] start. time_ms=%u", handle, htd_time_ms_get());
                handle(task.type, task.p_param, task.param_len);
                print_log("func end. time_ms=%u", htd_time_ms_get());
                if (task.p_param) {
                    htd_free(task.p_param);
                }
            }
            continue;
        } 

        // 定时任务
        waite_ms = htd_nonos_timer_ex(&m_timer_list);
        
        // 执行空闲事件
        if (waite_ms != 0) {
            sleep_flag = true;
            while (htd_nonos_idle_ex(&m_idle_slist)) {
                sleep_flag = false;
            }
            if (sleep_flag && sleep_handle) {
                sleep_handle(waite_ms);          // 系统休眠
            }
        }
    }
}

// @brief 初始化任务调度器
int32_t htd_task_sch_init(uint8_t mq_count)
{
    htd_slist_init(&m_idle_slist);
    htd_slist_init(&m_urgent_slist);
    htd_list_head_init(&m_timer_list);
    htd_list_head_init(&m_timer_standby_list);
    htd_memset(&m_task_fifo, 0, sizeof(m_task_fifo));
    m_timer_id = 1;
    return 0;
}


// @brief 从待机队列中查找
static void task_timer_standby_list_append(nonos_timer_node_t* p_timer)
{
    nonos_timer_node_t* p_node;
    htd_list_node_t* p_pos;
    htd_list_for_each(p_pos, &m_timer_standby_list) {
        p_node = htd_list_entry(p_pos, nonos_timer_node_t, node);
        if (p_node == p_timer) {
            return ;
        }
    }
    htd_list_insert_before(&p_timer->node, &m_timer_standby_list);
}

// @brief 添加到执行队列中
static void task_timer_list_insert(nonos_timer_node_t* p_timer, uint32_t now_ms)
{
    nonos_timer_node_t* p_node;
    htd_list_node_t* p_pos;
    uint32_t interval_ms;
    htd_list_for_each(p_pos, &m_timer_list) {
        p_node = htd_list_entry(p_pos, nonos_timer_node_t, node);
        interval_ms = htd_time_ms_diff(p_node->reach_ms, now_ms);
        if (interval_ms < 0x7fffffff && interval_ms > p_timer->timeout_ms) {
            break;
        }
    }
    // 插入到p_pos前面
    htd_list_insert_before(&p_timer->node, p_pos);
    // 第一个
    if (p_pos == m_timer_list.next) {
        htd_task_load_function(NULL, NULL);
    }
}

// @brief 从执行队列中移除
static nonos_timer_node_t* task_timer_list_remove(nonos_timer_node_t* p_timer)
{
    // 从当前的列表中移除
    nonos_timer_node_t* p_node;
    htd_list_node_t* p_pos;
    htd_list_for_each(p_pos, &m_timer_list) {
        p_node = htd_list_entry(p_pos, nonos_timer_node_t, node);
        if (p_node->id == p_timer->id) {
            htd_list_del(p_pos);
            return p_node;
        }
    }
    return NULL;
}

// @brief 创建软件定时器
// @return 成功返回定时器操作句柄，失败返回NULL
void* htd_timer_create(const char* name, unsigned int timeout_ms, bool is_repeat, htd_task_timeout_handle_t timeout_handle, void* arg)
{
    nonos_timer_node_t* p_node = (nonos_timer_node_t*)htd_malloc(sizeof(nonos_timer_node_t));
    ASSERT(p_node);
    htd_list_head_init(&p_node->node);
    p_node->id = m_timer_id++;
    p_node->is_repeat = is_repeat ? 1 : 0;
    p_node->timeout_ms = timeout_ms;
    p_node->reach_ms = 0;
    p_node->timeout_cb = timeout_handle;
    p_node->arg = arg;
    // 插入到最后面
    htd_list_insert_before(&p_node->node, &m_timer_standby_list);
    print_debug("create timer. timeout_ms=%u, is_repeat=%d", timeout_ms, is_repeat);
    return p_node;
}

// @brief 启动软件定时器
int htd_timer_start(void *handle)
{
    ASSERT(handle);
    nonos_timer_node_t* p_timer = (nonos_timer_node_t*)handle;
    uint32_t now_ms = htd_time_ms_get();
    
    // 添加到 standby 队列
    task_timer_standby_list_append(p_timer);
    
    // 创建备份定时器节点
    nonos_timer_node_t* p_new = htd_malloc(sizeof(nonos_timer_node_t));
    ASSERT(p_new);
    htd_memcpy(p_new, p_timer, sizeof(nonos_timer_node_t));
    htd_list_head_init(&p_new->node);
    p_new->reach_ms = now_ms + p_timer->timeout_ms;
    
    // 添加到启动队列中
    task_timer_list_insert(p_new, now_ms);
    return 0;
}

// @brief 重启软件定时器
void htd_timer_restart(void *handle, unsigned int ms)
{
    ASSERT(handle);
    nonos_timer_node_t* p_timer = (nonos_timer_node_t*)handle;
    uint32_t now_ms = htd_time_ms_get();

    // 添加到 standby 队列
    task_timer_standby_list_append(p_timer);

    // 从执行队列移除
    nonos_timer_node_t* p_new = task_timer_list_remove(p_timer);
    if (p_new == NULL) {
        p_new = htd_malloc(sizeof(nonos_timer_node_t));
        ASSERT(p_new);
    }

    // 更新定时器信息
    p_timer->timeout_ms = ms;
    htd_memcpy(p_new, p_timer, sizeof(nonos_timer_node_t));
    htd_list_head_init(&p_new->node);
    p_new->reach_ms = now_ms + p_timer->timeout_ms;

    // 添加到启动队列中
    task_timer_list_insert(p_new, now_ms);
}

// @brief 暂停软件定时器
void htd_timer_stop(void *handle)
{
    ASSERT(handle);
    nonos_timer_node_t* p_timer = (nonos_timer_node_t*)handle;
    nonos_timer_node_t* p_node = task_timer_list_remove(p_timer);
    if (p_node) {
        htd_free(p_node);
    }
}

// @brief 删除软件定时器
void htd_timer_delete(void* handle)
{
    htd_timer_stop(handle);
}

// @brief 销毁软件定时器
void htd_timer_destroy(void* handle)
{
    htd_timer_stop(handle);
    if (handle) {
        // 从 standby 队列删除
        nonos_timer_node_t* p_timer = (nonos_timer_node_t*)handle;
        nonos_timer_node_t* p_node;
        htd_list_node_t* p_pos;
        htd_list_for_each(p_pos, &m_timer_standby_list) {
            p_node = htd_list_entry(p_pos, nonos_timer_node_t, node);
            if (p_node == p_timer) {
                htd_list_del(&p_timer->node);
                htd_free(p_timer);
                return;
            }
        }
    }
}

// @brief 判断软件定时器是否激活
bool htd_timer_is_active(void* handle)
{
    nonos_timer_node_t* p_timer = (nonos_timer_node_t*)handle;
    // 从当前的列表中移除
    nonos_timer_node_t* p_node;
    htd_list_node_t* p_pos;
    htd_list_for_each(p_pos, &m_timer_list) {
        p_node = htd_list_entry(p_pos, nonos_timer_node_t, node);
        if (p_node->id == p_timer->id) {
            return true;
        }
    }
    return false;
}

// @brief 获取软件定时器参数
// @param param 为定时器回调函数的参数
void* htd_timer_param_get(void* param)
{
    return param;
}
#endif

