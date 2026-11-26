// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_BAROMETER_FILTER_H_
#define _HTD_BAROMETER_FILTER_H_

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
 
#ifdef __cplusplus
 extern "C" {
#endif /*__cplusplus*/


// 中位数滤波
typedef uint32_t barometer_val_t;
typedef struct {
    barometer_val_t* p_val;
    uint8_t size;
    uint8_t count;
    uint8_t wpos;
    barometer_val_t last_val;
} barometer_filter_t;

// @brief 初始化滤波器
// @param count 数量必须是奇数且大于5 
void barometer_filter_init(barometer_filter_t* p_filter, barometer_val_t* p_val, uint8_t count);
// @brief 往滤波器添加一个数值
void barometer_filter_add(barometer_filter_t* p_filter, barometer_val_t val);
// @brief 获取滤波值
barometer_val_t barometer_filter_get(barometer_filter_t* p_filter);
// @brief 获取过滤器里面数据个数
uint32_t barometer_filter_data_count_get(barometer_filter_t* p_filter);
// @brief 打印所有数据
void barometer_filter_print(barometer_filter_t* p_filter);



#ifdef __cplusplus
}
#endif
#endif /*_HTD_BAROMETER_FILTER_H_*/

