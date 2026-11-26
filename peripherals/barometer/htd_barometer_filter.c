// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL       4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG       "baro.filter"
#include "htd_device.h"
#include "htd_barometer_filter.h"


// @brief 初始化滤波器
// @param count 数量必须是奇数且大于5
void barometer_filter_init(barometer_filter_t* p_filter, barometer_val_t* p_val, uint8_t count)
{
    ASSERT(p_filter);
    ASSERT(p_val);
    ASSERT(count >= 5 && (count & 0x01));
    p_filter->p_val = p_val;
    p_filter->count = 0;
    p_filter->wpos = 0;
    p_filter->size = count;
}
// @brief 往滤波器添加一个数值
void barometer_filter_add(barometer_filter_t* p_filter, barometer_val_t val)
{
    ASSERT(p_filter);

    p_filter->last_val = val;
    p_filter->p_val[p_filter->wpos++] = val; 
    if (p_filter->wpos >= p_filter->size) {
        p_filter->wpos = 0;
    }
    if (p_filter->count < p_filter->size) {
        p_filter->count++;
    }
}
// @brief 获取滤波值
barometer_val_t barometer_filter_get(barometer_filter_t* p_filter)
{
    uint32_t i, j;
    barometer_val_t temp_val;
    barometer_val_t total;
    barometer_val_t *p_table; 

    ASSERT(p_filter);
    ASSERT(p_filter->count > 0);
    
    // 数量少于5个，直接使用平均值滤波
    if (p_filter->count < 5) {
        for (i=0; i<p_filter->count; ++i) {
            total += p_filter->p_val[i];
        }
        return total / i;
    }
    
    // 数量超过5个，使用中位数滤波
    p_table = (barometer_val_t*)htd_malloc(p_filter->size * sizeof(barometer_val_t));
    ASSERT(p_table);
    memcpy(p_table, p_filter->p_val, p_filter->count * sizeof(barometer_val_t));
    for (i=0; i<p_filter->count; ++i) {
        for (j=i+1; j<p_filter->count; ++j) {
            if (p_table[i] < p_table[j]) {
                temp_val = p_table[i];
                p_table[i] = p_table[j];
                p_table[j] = temp_val;
            }
        }
    }
    // 取中间三个求平均值
    i = (p_filter->count - 3) / 2;
    total = p_table[i] + p_table[i+1] + p_table[i+2];
    htd_free(p_table);
    return total / 3;
}

// @brief 获取过滤器里面数据个数
uint32_t barometer_filter_data_count_get(barometer_filter_t* p_filter)
{
    return p_filter->count;
}

// @brief 打印所有数据
void barometer_filter_print(barometer_filter_t* p_filter)
{
    uint16_t i;
    ASSERT(p_filter);
    print_debug("median filter: count=%d, size=%d, last_val=%d", p_filter->count, p_filter->size, p_filter->last_val);
    for (i=0; i<p_filter->count; ++i) {
        print_debug("i=%d, val=%d", i, p_filter->p_val[i]);
    }
}

