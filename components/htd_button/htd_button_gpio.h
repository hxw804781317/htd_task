// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_BUTTON_GPIO_H_
#define _HTD_BUTTON_GPIO_H_

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "htd_button_port.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


// 按键id
#define HTD_BUTTON_ID_0                 (1UL<<0)
#define HTD_BUTTON_ID_1                 (1UL<<1)
#define HTD_BUTTON_ID_2                 (1UL<<2)
#define HTD_BUTTON_ID_3                 (1UL<<3)
#define HTD_BUTTON_ID_4                 (1UL<<4)
#define HTD_BUTTON_ID_5                 (1UL<<5)
#define HTD_BUTTON_ID_6                 (1UL<<6)
#define HTD_BUTTON_ID_7                 (1UL<<7)
#define HTD_BUTTON_ID_8                 (1UL<<8)
#define HTD_BUTTON_ID_9                 (1UL<<9)
typedef uint32_t htd_button_id_t;

// 该模块支持三种最简单的按钮事件
#define HTD_BUTTON_EV_TYPE_PCBD         0x01            // 按下去计时，弹起执行
#define HTD_BUTTON_EV_TYPE_PCPD         0x02            // 按下去计时，按下去执行
#define HTD_BUTTON_EV_TYPE_BCBD         0x03            // 弹起计时，弹起执行
#define HTD_BUTTON_EV_TYPE_REPEAT       0x04            // 连续事件, 只能是弹起执行
#define HTD_BUTTON_EV_TYPE_PRESS_HOLD   0x05            // 不断上报按下事件
typedef uint8_t htd_button_evt_t;

// 判断按键是否按下
typedef bool (*htd_button_is_pressed_handle_t)(void);
// 按键事件回调
typedef void (*htd_button_evt_func_t)(htd_button_id_t id, uint32_t func_idx);


// 描述一个按钮功能
#if (defined ( __CC_ARM ) || defined(__CLANG_ARM))
#pragma anon_unions
#endif
typedef struct {
    const htd_button_evt_t evt;
    union {
        const uint32_t pcbd_press_ms;               // 按下去持续时间
        const uint32_t pcpd_press_ms;               // 按下去持续时间
        const uint32_t bcbd_release_ms;             // 弹起超时时间
        const uint32_t repeat_inter_ms;             // 重复间隔时间
        const uint32_t press_hold_timeout_ms;
    };
    union {
        const uint32_t pcbd_timeout_ms;             //  
        const uint32_t repeat_count;                // 连续按键，配置连按几下
    };
} htd_button_func_t;
// 长按, 按着达到时间ms执行
#define HTD_BUTTON_FUNC_LONG_PRESS(ms)                  {.evt = HTD_BUTTON_EV_TYPE_PCPD, .pcpd_press_ms = ms}
// 短按，按下到弹起时时间超过ms, 小于timeout_ms执行
#define HTD_BUTTON_FUNC_SHORT_PRESS(ms, timeout_ms)     {.evt = HTD_BUTTON_EV_TYPE_PCBD, .pcbd_press_ms = ms, .pcbd_timeout_ms = timeout_ms}
// 重复按按键, cnt定义按按键次数
#define HTD_BUTTON_FUNC_REPEAT_PRESS(cnt)               {.evt = HTD_BUTTON_EV_TYPE_REPEAT, .repeat_inter_ms = 300, .repeat_count = cnt}
// 持续按键事件, timeout_ms 定义多久抛一次事件
#define HTD_BUTTON_FUNC_HOLD(timeout_ms)                {.evt = HTD_BUTTON_EV_TYPE_PRESS_HOLD, .press_hold_timeout_ms = timeout_ms}
// 按下
#define HTD_BUTTON_FUNC_PRESS(ms)                       {.evt = HTD_BUTTON_EV_TYPE_PCPD, .pcpd_press_ms = ms,}
// 弹起
#define HTD_BUTTON_FUNC_RELEASE(ms)                     {.evt = HTD_BUTTON_EV_TYPE_BCBD, .bcbd_release_ms = ms,}


// 描述一个按钮
typedef struct {
    const htd_button_id_t button_id;        // 按键id, 每个按键对应一个bit位，复合按键对应多个bit位
    const uint8_t func_count;               // 动作数量
    const htd_button_func_t* p_func;        // 一个按钮可能有多个功能，比如长按，短按，长按多久
    htd_button_is_pressed_handle_t is_pressed_handle;
} htd_button_t;


// @brief 初始化所有按键
// @param p_config 配置信息，该配置信息必须为全局变量
// @param p_button 需要先配置好对应的按键信息
// @return 成功返回对应的按键句柄，失败返回空
void* htd_button_handle_create(const htd_button_t* p_button, uint8_t count, htd_button_evt_func_t evt_func);
// @brief 销毁按键句柄
void htd_button_handle_destroy(void* p_handle);
// @brief 按键事件调用接口，比如按键中断来了，就调用这个接口
// @param p_handle 按键句柄，htd_button_create的返回值
// @node  这里面会调用 htd_button_load_timer_function 函数，如果中断
//        不可以调用该函数，需要将中断事件抛到线程中再调用 htd_button_process;
void htd_button_process(void* p_handle);


#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_BUTTON_GPIO_H_*/

