// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL 4
#define DEBUG_TAG              "bitmap"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "htd_task.h"
#include "htd_bitmap.h"


// @brief 标记bitmap表第pos位
void htd_bitmap_pos_set(uint32_t* p_bitmap_table, uint32_t count, uint16_t pos, bool is_enable)
{
    uint32_t all_bits;
    uint32_t index;
    uint32_t val;

    all_bits = count * 32;
    if (pos >= all_bits) {
        print_error("bitmap set failed. pos=%d, count=%d, all_bits=%d", pos, count, all_bits);
        return;
    }

    index = pos >> 5;
    pos &= 31;
    val = p_bitmap_table[index];
    if (is_enable) {
        val |= (1UL << pos);
    } else {
        val &= ~(1UL << pos);
    }
    p_bitmap_table[index] = val;
}

// @brief 获取bitmap表第pos位，位长为len(不能超过32bit)
uint32_t htd_bitmap_pos_get(uint32_t* p_bitmap_table, uint32_t count, uint32_t pos, uint8_t len)
{
    uint32_t index, i = 0;
    uint32_t val;

    if (pos >= count * 32 || len > 32) {
        return 0;
    }
    if (count * 32 < pos + len) {
        len = count * 32 - pos;
    }
    
    val = 0;
    while (len--) {
        index = pos >> 5;
        if (p_bitmap_table[index] & (1<<(pos & 31))){
            val |= (1 << i);
        }
        i++;
        pos++;
    }
    return val;
}  

// @brief 判断bitmap表第pos是否被标记
bool htd_bitmap_pos_is_set(const uint32_t *p_bitmap_table, uint32_t count, uint16_t pos)
{
    uint32_t index;
    if (pos >= count * 32) {
        return false;
    }
    index = pos >> 5;
    pos &= 31;
    return !!(p_bitmap_table[index] & (1UL << pos));
}

// @brief 查找第一个未标记的位置
uint16_t htd_bitmap_unused_pos_get(uint32_t* p_bitmap_table, uint32_t count)
{
    uint32_t i, j;
    uint32_t val;
    for (i=0; i<count; ++i) {
        val = p_bitmap_table[i];
        for (j=0; j<32; ++j) {
            if (!(val & (1UL << j))) {
                return ((i * 32) + j);
            }
        }
    }
    return 0xffff;
}

// @brief 计算一个数值中二进制1的个数
int htd_popcount(uint32_t v)
{
#ifdef __GNUC__ 
    return __builtin_popcount(v);
#else
    v = (v & 0x55555555) + ((v & 0xaaaaaaaa) >> 1);
    v = (v & 0x33333333) + ((v & 0xcccccccc) >> 2);
    v = (v & 0x0f0f0f0f) + ((v & 0xf0f0f0f0) >> 4);
    v = (v & 0x00ff00ff) + ((v & 0xff00ff00) >> 8);
    v = (v & 0x0000ffff) + ((v & 0xffff0000) >> 16);
    return v;
#endif
}

// @brief 总共多少个bit
uint32_t htd_bitmap_popcount(uint32_t* p_bitmap_table, uint32_t count)
{
    uint32_t i = 0;
    while (count--) {
        i += htd_popcount(p_bitmap_table[0]);
        p_bitmap_table++;
    }
    return i;
}


