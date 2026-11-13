// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_BUTTON_PORT_H_
#define _HTD_BUTTON_PORT_H_

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

// 对接头文件
#include "htd_task.h"
#include "htd_device.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


//////////////////////////////////////////////////////////////////////
// 重复按键间隔时间
#ifndef HTD_BUTTON_REPEAT_INTER_TIME_MS
    #define HTD_BUTTON_REPEAT_INTER_TIME_MS 200
#endif

#ifndef PIN_NUM_T_DEFINED
    typedef uint16_t pin_num_t;
#endif /*PIN_NUM_T_DEFINED*/
#ifndef __BIT
    #define __BIT(x)                        (1uL<<(x))
#endif
#ifndef ALIGN_UP
    #define ALIGN_UP(n, align)              (((n) + (align) - 1) & ~((align) - 1))
#endif 




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
    #define DELAY_US(us)                    htd_delay_us(us)
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
// @brief 标记bitmap表第pos位为1或者0，is_enable为true，标记为1，否则为0
void button_bitmap_pos_set(uint32_t* p_bitmap_table, uint32_t count, uint16_t pos, bool is_enable);
// @brief 判断bitmap表第pos是否被标记为1
bool button_bitmap_pos_is_set(uint32_t *p_bitmap_table, uint32_t count, uint16_t pos);


// @brief 加载一个任务
bool button_load_func(void (*timeout_cb)(void*), void* arg);
// @brief 加载一个定时任务
bool button_load_timer_func(bool is_repeat, uint32_t timeout_ms, void (*timeout_cb)(void*), void* arg);
// @brief 卸载定时任务
void button_unload_timer_function(void (*timeout_cb)(void*));


// @brief 配置引脚为高阻态
void button_pin_hiz_set(pin_num_t pin_num);
// @brief 配置引脚为推挽输出
void button_pin_output_pp_set(pin_num_t pin_num);
// @brief 配置电平
void button_pin_write(pin_num_t pin_num, uint8_t is_set);
// @brief 配置为上拉输入
void button_pin_input_pullup_set(pin_num_t pin_num);
// @brief 配置引脚为下降沿中断，上拉输入
void button_pin_int_falling_set(pin_num_t pin_num, bool is_enable);
// @brief 读取引脚电平
bool button_pin_read(pin_num_t pin_num);



#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_BUTTON_PORT_H_*/

