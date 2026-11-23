#ifndef _HTD_NETWORK_THREAD_H_
#define _HTD_NETWORK_THREAD_H_

#include "htd_components.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

#ifndef HTD_NETWORK_USE_THREAD
    #if (defined(HTD_USING_LWIP)) 
        #define HTD_NETWORK_USE_THREAD          0
    #else
        #define HTD_NETWORK_USE_THREAD          1
    #endif
#endif
#ifndef HTD_NETWORK_THREAD_PRIORITY
    #define HTD_NETWORK_THREAD_PRIORITY         10
#endif
#ifndef HTD_NETWORK_THREAD_STACK_SIZE
    #define HTD_NETWORK_THREAD_STACK_SIZE       4096
#endif 


#if defined(HTD_NETWORK_USE_THREAD) && (HTD_NETWORK_USE_THREAD == 1)
    // @brief 加载定时任务(不可以在中断中调用)
    int32_t network_thread_load_timer_function(bool is_repeat, uint32_t timeout_ms, htd_task_timeout_handle_t handle, void* arg);
    // @brief 加载紧急任务(不可以在中断中调用)
    void network_thread_load_urgent_function(htd_task_timeout_handle_t handle, void* arg);
    // @brief 添加空闲任务(最低优先级任务，其他任务执行后才会执行这些回调函数, 不可以在中断中调用)
    void network_thread_load_idle_function(htd_task_timeout_handle_t handle, void* arg);
    // @brief 加载函数(可以在中断中调用)
    bool network_thread_load_function_fisr(htd_task_timeout_handle_t handle, void* arg);
    // @brief 加载函数(不可以在中断中调用)
    bool network_thread_load_function(htd_task_timeout_handle_t handle, void* arg);
    // @brief 加载函数数据(不可以在中断中调用)
    bool network_thread_load_function_data(htd_task_data_handle_t handle, const void* p_data, uint16_t data_len);
    // @brief 加载类型函数数据(不可以在中断中调用)
    bool network_thread_load_function_type_data(htd_task_data_type_handle_t handle, uint16_t type, const uint8_t* p_data, uint16_t data_len);
    // @brief 初始化任务调度器
    int32_t network_thread_init(uint8_t mq_count);
    // @brief 去初始化
    void network_thread_uninit(void);
#else
    // @brief 加载定时任务(不可以在中断中调用)
    __STATIC_INLINE int32_t network_thread_load_timer_function(bool is_repeat, uint32_t timeout_ms, htd_task_timeout_handle_t handle, void* arg) 
    {
        return htd_task_load_timer_function(is_repeat, timeout_ms, handle, arg);
    }
    // @brief 加载紧急任务(不可以在中断中调用)
    __STATIC_INLINE void network_thread_load_urgent_function(htd_task_timeout_handle_t handle, void* arg)
    {
        htd_task_load_urgent_function(handle, arg);
    }
    // @brief 添加空闲任务(最低优先级任务，其他任务执行后才会执行这些回调函数, 不可以在中断中调用)
    __STATIC_INLINE void network_thread_load_idle_function(htd_task_timeout_handle_t handle, void* arg) 
    {
        htd_task_load_idle_function(handle, arg);
    }
    // @brief 加载函数(可以在中断中调用)
    __STATIC_INLINE bool network_thread_load_function_fisr(htd_task_timeout_handle_t handle, void* arg) 
    {
        return htd_task_load_function_fisr(handle, arg);
    }
    // @brief 加载函数(不可以在中断中调用)
    __STATIC_INLINE bool network_thread_load_function(htd_task_timeout_handle_t handle, void* arg) 
    {
        return htd_task_load_function(handle, arg);
    }
    // @brief 加载函数数据(不可以在中断中调用)
    __STATIC_INLINE bool network_thread_load_function_data(htd_task_data_handle_t handle, const void* p_data, uint16_t data_len)
    {
        return htd_task_load_function_data(handle, p_data, data_len);
    }
    // @brief 加载类型函数数据(不可以在中断中调用)
    __STATIC_INLINE bool network_thread_load_function_type_data(htd_task_data_type_handle_t handle, uint16_t type, const uint8_t* p_data, uint16_t data_len)
    {
        return htd_task_load_function_type_data(handle, type, p_data, data_len);
    }
    // @brief 初始化任务调度器
    __STATIC_INLINE int32_t network_thread_init(uint8_t mq_count)
    {
        return 0;
    }
    // @brief 去初始化
    __STATIC_INLINE void network_thread_uninit(void) 
    {
    }
#endif /*defined(HTD_NETWORK_USE_THREAD) && (HTD_NETWORK_USE_THREAD == 1)*/

#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_NETWORK_THREAD_H_*/

