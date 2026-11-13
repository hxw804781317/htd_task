// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_LED_PORT_H_
#define _HTD_LED_PORT_H_

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// 对接头文件
#include "htd_task.h"
#include "htd_device.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/



//////////////////////////////////////////////////////////////////////
// 对接对应的接口
#ifndef ASSERT
    #define ASSERT(x)
#endif
#ifndef htd_memset
    #define htd_memset(ptr, v, n)           memset(ptr, v, n)
#endif
#ifndef htd_memcpy
    #define htd_memcpy(dst, src, n)         memcpy(dst, src, n)
#endif
#ifndef htd_malloc
    #define htd_malloc(s)                   malloc(s)
#endif
#ifndef htd_free
    #define htd_free(p)                     free(p)
#endif
#ifndef DELAY_US
    #define DELAY_US(ms)                    htd_delay_us(ms)
#endif
#ifndef TIME_MS_GET                         /*获取系统运行毫秒数*/
    #define TIME_MS_GET()                   htd_time_ms_get()
#endif
#ifndef TIME_MS_DIFF
    #define TIME_MS_DIFF(now_ms, prev_ms)   (((now_ms) - (prev_ms)) & 0xffffffff)
#endif

#ifndef print_debug
    #define print_debug(...)
#endif
#ifndef print_warning
    #define print_warning(...)
#endif
#ifndef print_error
    #define print_error(...)
#endif


//////////////////////////////////////////////////////////////////////
#ifndef PIN_NUM_T_DEFINED
    typedef uint16_t pin_num_t;
#endif /*PIN_NUM_T_DEFINED*/

#ifndef __BIT
    #define __BIT(x)                        (1uL<<(x))
#endif
#ifndef ALIGN_UP
    #define ALIGN_UP(n, align)              (((n) + (align) - 1) & ~((align) - 1))
#endif 
// 判断是否为2的指数倍数
#ifndef IS_POWER_OF_TWO
    #define IS_POWER_OF_TWO(A)              (((A) != 0) && ((((A) - 1) & (A)) == 0))
#endif
#ifndef ARRAY_COUNT
    #define ARRAY_COUNT(X)                  (sizeof(X) / sizeof(X[0]))
#endif



//////////////////////////////////////////////////////////////////////
// @brief 标记bitmap表第pos位为1或者0，is_enable为true，标记为1，否则为0
void led_bitmap_pos_set(uint32_t* p_bitmap_table, uint32_t count, uint16_t pos, bool is_enable);
// @brief 判断bitmap表第pos是否被标记为1
bool led_bitmap_pos_is_set(uint32_t *p_bitmap_table, uint32_t count, uint16_t pos);


// @brief 加载一个任务
bool led_load_func(void (*timeout_cb)(void*), void* arg);
// @brief 加载一个定时任务(软件定时器)
bool led_load_timer_func(bool is_repeat, uint32_t timeout_ms, void (*timeout_cb)(void*), void* arg);
// @brief 删除定时任务
void led_unload_timer_function(void (*timeout_cb)(void*));


// @brief 创建锁
void* led_lock_create(const char* p_name);
// @brief 加锁
void led_lock(void* p_lock);
// @brief 解锁
void led_unlock(void* p_lock);


// @brief 某个引脚控制为输入高阻态模式(浮空模式)
void led_pin_input_nopull_set(pin_num_t pin_num);
// @brief 某个引脚控制为输出
void led_pin_output_pp_set(pin_num_t pin_num);
// @brief 控制某个引脚高低电平
void led_pin_write(pin_num_t pin_num, uint8_t is_set);


// @brief 创建硬件定时器
void* led_hw_timer_create(const char* p_name, uint32_t timeout_us, bool is_repeat, void (*timeout_cb)(void*), void* param);
// @brief 销毁定时器
int32_t led_hw_timer_destroy(void* p_handle);
// @brief 启动硬件定时器
int32_t led_hw_timer_start(void* p_handle);
// @brief 重启定时器
int32_t led_hw_timer_restart(void* p_handle, uint32_t timeout_us);
// @brief 停止硬件定时器
int32_t led_hw_timer_stop(void* p_handle);
// @brief 获取 led_hw_timer_create 传进去的参数 param
void* led_hw_timer_param_get(void* p_handle);





#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_LED_PORT_H_*/

