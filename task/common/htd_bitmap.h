// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_BITMAP_H_
#define _HTD_BITMAP_H_

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif


// @brief 标记 bitmap表第pos位
void htd_bitmap_pos_set(uint32_t* p_bitmap_table, uint32_t count, uint16_t pos, bool is_enable);
// @brief 获取bitmap表第pos位，位长为len(不能超过32bit)
uint32_t htd_bitmap_pos_get(uint32_t* p_bitmap_table, uint32_t count, uint32_t pos, uint8_t len);
// @brief 判断bitmap表第pos是否被标记
bool htd_bitmap_pos_is_set(const uint32_t *p_bitmap_table, uint32_t count, uint16_t pos);
// @brief 查找第一个未标记的位置
uint16_t htd_bitmap_unused_pos_get(uint32_t* p_bitmap_table, uint32_t count);


// @brief 计算一个数值中二进制1的个数
int htd_popcount(uint32_t v);
// @brief 总共多少个bit
uint32_t htd_bitmap_popcount(uint32_t* p_bitmap_table, uint32_t count);


#ifdef __cplusplus
}
#endif
#endif /*_HTD_BITMAP_H_*/

