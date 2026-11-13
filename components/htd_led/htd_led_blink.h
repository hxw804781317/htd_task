// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_LED_BLINK_H_
#define _HTD_LED_BLINK_H_

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "htd_led_port.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


// 控制开关/亮度函数
typedef void (*htd_led_onoff_func_t)(uint32_t idx, uint8_t bright_percent);


// @brief 初始化
// @param count 控制的 led 数量
int32_t htd_led_init(htd_led_onoff_func_t onoff_handle, uint32_t count);
// @brief 打印所有led信息
void htd_led_dump(void);
// @brief 亮灯/灭灯
int32_t htd_led_onoff(uint32_t idx, uint8_t onoff);
// @brief 判断灯光是否为亮
bool htd_led_is_on(uint32_t idx);
// @brief 打开某个灯，time_ms后灭掉
int32_t htd_led_on_time_ms(uint32_t idx, uint32_t time_ms);
// @brief 周期闪烁
// @param blink_count 闪烁次数
// @param blink_period_ms 闪烁周期，单位毫秒；一亮一灭为一个周期
int32_t htd_led_period_blink(uint32_t idx, uint32_t blink_count, uint32_t blink_period_ms);
// @brief 自定义闪烁;
// @param p_array 定义亮->灭，亮->灭时长数组，单位毫秒
// @param array_count 必须为偶数
int32_t htd_led_custom_blink(uint32_t idx, uint16_t* p_array, uint32_t array_count);
// @brief 呼吸控制(未实现)
int32_t htd_led_breath(uint32_t idx);

#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_LED_BLINK_H_*/

