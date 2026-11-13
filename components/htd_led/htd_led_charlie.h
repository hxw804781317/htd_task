// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_LED_CHARLIE_H_
#define _HTD_LED_CHARLIE_H_

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "htd_led_port.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/



// LED序列号，超过255个需要重新定义 led_idx_t 数据类型
typedef uint8_t led_idx_t;
#define LED_INDEX_MAX           255

// 帧率，默认100
#define HTD_LED_CHARLIE_FRAME_RATE  100

// 查理复用 LED
typedef struct {
    uint32_t *state_table; 
    led_idx_t *led_idx_table;
    pin_num_t *pin_table;
    uint8_t pin_count;
    uint8_t state_table_count;
    uint16_t led_idx_table_len;
    uint32_t is_running;
    uint32_t row_on_us;                 // 行延迟亮时间，单位微秒

#ifdef HTD_LED_USE_HW_TIMER
    void* hw_timer_handle;
    uint8_t on_off;
    led_idx_t on_off_idx;
    uint8_t row_idx;
    uint8_t row_on;
#endif
}led_charlie_t;

// @brief 创建一个查理复用LED对象
led_charlie_t* htd_led_charlie_create(const pin_num_t* pin_num_table, uint8_t pin_num_table_count);
// @brief 销毁查理复用 LED 对象
int32_t htd_led_charlie_destroy(led_charlie_t* p_object);
// @brief 启动查理复用
int32_t htd_led_charlie_start(led_charlie_t* p_object);
// @brief 关闭查理复用扫描, 所有灯都会灭
void htd_led_charlie_stop(led_charlie_t* p_object);
// @brief 控制某个灯亮/灭
int32_t htd_led_charlie_onoff(led_charlie_t* p_object, uint32_t led_idx, uint8_t onoff);
// @brief 显示灯序列号
void htd_led_charlie_show(led_charlie_t* p_object);


#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_LED_CHARLIE_H_*/

