// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_FLASH_KV_H_
#define _HTD_FLASH_KV_H_

#include "htd_flash_common.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


//////////////////////////////////////////////////////////////////////
// 可存储数据最大长度
#define FLASHKV_VAL_MAX_LEN                     (2048-16)

typedef uint16_t key_table_t;
typedef struct {
    uint32_t init_flag;                         // 初始化标志
    volatile uint32_t addr;
    volatile uint32_t swap_addr;
    key_table_t* p_key_table;
    uint16_t key_count;
    const flash_config_t* p_cfg;
    const flash_handle_t* p_flash;
}flashkv_t;



//////////////////////////////////////////////////////////////////////
// @brief 初始化
int32_t htd_flashkv_init(flashkv_t* p_fkv, const flash_config_t* p_cfg, key_table_t* p_key_table, uint16_t key_count);
// @brief 去初始化
int32_t htd_flashkv_uninit(flashkv_t* p_fkv);
// @brief 读取数值
int32_t htd_flashkv_read(flashkv_t* p_fkv, uint16_t key, void* p_data, uint16_t dat_len);
// @brief 读取字段存储数据长度
int32_t htd_flashkv_data_len_read(flashkv_t* p_fkv, uint16_t key);
// @brief 写入数值
int32_t htd_flashkv_write(flashkv_t* p_fkv, uint16_t key, const void* p_data, uint16_t data_len);
// @brief 清除所有数据
void htd_flashkv_clear(flashkv_t* p_fkv);
// @brief 删除某条数据
void htd_flashkv_delete(flashkv_t* p_fkv, uint16_t key);


// @brief 读取前面N个字节
int32_t htd_flashkv_read_direct(flashkv_t* p_fkv, uint16_t key, void* p_data, uint16_t data_len);
// @brief 获取存储的数据地址
void* htd_flashkv_data_addr_get(flashkv_t* p_fkv, uint16_t key);



#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_FLASH_KV_H_*/

