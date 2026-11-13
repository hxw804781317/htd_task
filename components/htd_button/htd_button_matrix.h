// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_BUTTON_MATRIX_H_
#define _HTD_BUTTON_MATRIX_H_

#include "htd_button_port.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


//////////////////////////////////////////////////////////////////////
// 注意：
//   1、不支持多按键同时按下检测
//   2、行引脚必须为中断引脚
//   3、行内部上拉，列推挽输出低电平，按下时行引脚被拉低，触发中断，启动扫描



//////////////////////////////////////////////////////////////////////
// 是否打开重复按键功能，打开后，短按会延迟上报事件
#ifndef BUTTON_MATRIX_SUPPORT_REPEAT
    #define BUTTON_MATRIX_SUPPORT_REPEAT        0
#endif
// 按下间隔上报时间，默认100毫秒
#ifndef BUTTON_MATRIX_HOLD_INTERVAL_MS
    #define BUTTON_MATRIX_HOLD_INTERVAL_MS      100
#endif
// 短按时间
#ifndef BUTTON_MATRIX_SHORT_PRESS_MS
    #define BUTTON_MATRIX_SHORT_PRESS_MS        80
#endif
// 短按超时时间
#ifndef BUTTON_MATRIX_SHORT_TIMEOUT_MS
    #define BUTTON_MATRIX_SHORT_TIMEOUT_MS      500
#endif
// 长按时间, 超过该时间都为长按
#ifndef BUTTON_MATRIX_LONG_PRESS_MS
    #define BUTTON_MATRIX_LONG_PRESS_MS         2000
#endif





//////////////////////////////////////////////////////////////////////
// 按键事件类型
#define BUTTON_MATRIX_EVT_SHORT         1           // 短按，弹起时触发，参数为按下去持续的时间，单位毫秒
#define BUTTON_MATRIX_EVT_LONG          2           // 弹起时触发，参数为按下去持续的时间，单位毫秒
#define BUTTON_MATRIX_EVT_REPEAT        3           // 参数为重复按下次数
#define BUTTON_MATRIX_EVT_HOLD          4           // 按键持续时间，参数为按下去持续时间，单位毫秒
typedef uint8_t button_matrix_evt_t;

// 矩阵扫描按键
typedef void (*button_matrix_evt_notify_t)(uint32_t id, button_matrix_evt_t evt, uint32_t param);
typedef struct {
    button_matrix_evt_notify_t evt_func;
    pin_num_t *pin_row_table;
    pin_num_t *pin_col_table;
    uint8_t pin_row_count;
    uint8_t pin_col_count;
    
    uint16_t active_idx;
    uint8_t is_running;
    uint8_t repeat_count;
    uint16_t continuous_cnt;
    uint32_t active_time_ms;
    uint32_t release_time_ms;
} button_matrix_t;


// @brief 创建一个矩阵控制按键对象
button_matrix_t* htd_button_matrix_create(const pin_num_t *pin_row, uint8_t pin_row_count, const pin_num_t* pin_column, uint8_t pin_column_count, button_matrix_evt_notify_t evt_func);

// @brief 销毁矩阵控制按键对象
int32_t htd_button_matrix_destroy(button_matrix_t* p_object);

// @brief 中断回调函数
void htd_button_matrix_irq_evt_handler(button_matrix_t* p_object);

// @brief 判断某个按键是否按下
bool htd_button_matrix_is_pressed(button_matrix_t* p_object, uint32_t idx);

// @brief 显示按键矩阵序号
void htd_button_matrix_show(button_matrix_t* p_object);



#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_BUTTON_MATRIX_H_*/

