// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_BUTTON_CHARLIE_H_
#define _HTD_BUTTON_CHARLIE_H_

#include "htd_button_port.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


//////////////////////////////////////////////////////////////////////
// 注意点：
// 1：查理复用按键阵列，引脚的排列规则为：假设有N个引脚，可以复用的按键数量为 N*(N-1)
//    P0(拉高)P1(拉低); P0(拉高)P2(拉低); ...; P0(拉高)Pn(拉低)
//    P1(拉高)P0(拉低); P1(拉高)P2(拉低); ...; P1(拉高)Pn(拉低)
//    ...
//    Pn(拉高)P0(拉低); Pn(拉高)P1(拉低); ...; Pn(拉高)Pn-1(拉低)
// 2：查理复用按键无法检测多按键同时按下
// 3：查理复用按键智能使用轮询检测，无法使用中断触发




//////////////////////////////////////////////////////////////////////
// 是否打开重复按键功能，打开后，短按会延迟上报事件
#ifndef BUTTON_CHARLIE_SUPPORT_REPEAT
    #define BUTTON_CHARLIE_SUPPORT_REPEAT       0
#endif
// 按下间隔上报时间，默认100毫秒
#ifndef BUTTON_CHARLIE_HOLD_INTERVAL_MS
    #define BUTTON_CHARLIE_HOLD_INTERVAL_MS     100
#endif
// 短按时间
#ifndef BUTTON_CHARLIE_SHORT_PRESS_MS
    #define BUTTON_CHARLIE_SHORT_PRESS_MS       80
#endif
// 短按超时时间
#ifndef BUTTON_CHARLIE_SHORT_TIMEOUT_MS
    #define BUTTON_CHARLIE_SHORT_TIMEOUT_MS     500
#endif
// 长按时间, 超过该时间都为长按
#ifndef BUTTON_CHARLIE_LONG_PRESS_MS
    #define BUTTON_CHARLIE_LONG_PRESS_MS        2000
#endif





//////////////////////////////////////////////////////////////////////
// 按键序列号，超过255个需要重新定义 button_idx_t 数据类型
typedef uint8_t button_idx_t;
#define BUTTON_INDEX_MAX                255

// 按键事件类型
#define BUTTON_CHARLIE_EVT_SHORT        1           // 短按，弹起时触发，参数为按下去持续的时间，单位毫秒
#define BUTTON_CHARLIE_EVT_LONG         2           // 弹起时触发，参数为按下去持续的时间，单位毫秒
#define BUTTON_CHARLIE_EVT_REPEAT       3           // 参数为重复按下次数
#define BUTTON_CHARLIE_EVT_HOLD         4           // 按键持续时间，参数为按下去持续时间，单位毫秒
typedef uint8_t button_charlie_evt_t;

// 查理复用
typedef void (*button_charlie_evt_notify_t)(uint32_t id, button_charlie_evt_t evt, uint32_t param);
typedef struct {
    button_charlie_evt_notify_t evt_func;
    button_idx_t *button_idx_table;
    pin_num_t *pin_table;
    uint16_t button_idx_table_len;
    uint8_t pin_count;
    
    uint16_t active_idx;
    uint8_t is_running;
    uint8_t repeat_count;
    uint16_t continuous_cnt;
    uint32_t active_time_ms;
    uint32_t release_time_ms;
}button_charlie_t;

// @brief 创建一个对象
button_charlie_t* htd_button_charlie_create(const pin_num_t* pin_num_table, uint8_t pin_num_table_count, button_charlie_evt_notify_t evt_func);
// @brief 销毁对象
int32_t htd_button_charlie_destroy(button_charlie_t* p_object);
// @brief 启动查理复用
int32_t htd_button_charlie_start(button_charlie_t* p_object);
// @brief 关闭查理复用扫描, 所有灯都会灭
void htd_button_charlie_stop(button_charlie_t* p_object);
// @brief 判断某个按键是否按下
bool htd_button_charlie_is_pressed(button_charlie_t* p_object, uint32_t idx);
// @brief 显示按键矩阵序号
void htd_button_charlie_show(button_charlie_t* p_object);



#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_BUTTON_CHARLIE_H_*/

