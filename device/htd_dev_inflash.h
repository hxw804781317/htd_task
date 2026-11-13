// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_DEV_INFLASH_H_
#define _HTD_DEV_INFLASH_H_

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/



typedef struct {
    uint32_t addr;
    uint32_t size;
    uint32_t erase_unit;
} htd_flash_partition_t;


// @brief 获取flash信息，返回总共多少个内存堆
// @param idx 第几块flash，partition_count 获取多少块内存信息
uint32_t htd_inflash_partition_get(htd_flash_partition_t* p_info, uint32_t partition_count, uint32_t idx);
// @brief 判断是否为内部flash块
bool htd_inflash_bank_is(uint32_t start_addr, uint32_t end_addr);
// @brief 读取片内flash数据
// @return 返回读取到的数据长度
int32_t htd_inflash_read(uint32_t addr, uint8_t *p_buff, size_t buff_len);
// @brief 写入片内flash数据
// @return 返回写入的数据长度
int32_t htd_inflash_write(uint32_t addr, const uint8_t *p_buff, size_t buff_len);
// @brief 擦除片内flash
int32_t htd_inflash_erase(uint32_t addr, size_t size);
// @brief 获取擦除单位大小
uint32_t htd_inflash_erase_unit_get(uint32_t addr);
// @brief 信息显示
void htd_inflash_info_show(void);


#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_DEV_INFLASH_H_*/

