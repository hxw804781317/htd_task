// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_FLASH_BOX_H_
#define _HTD_FLASH_BOX_H_

#include "htd_flash_common.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

//////////////////////////////////////////////////////////////////////
#define FTL_DATA_VAL_MAX_LEN        64

// 每个块的信息
typedef struct {
    uint32_t serials;           // 该块序列号
    uint16_t addr;              // 地址
    uint16_t tag;               // 标签
    uint16_t eidx;
    uint16_t sidx; 
} block_info_t;

typedef struct {
    const flash_handle_t* p_flash;
    const flash_config_t* p_cfg;
    uint32_t block_len;
    uint32_t last_serials;      // 最大值0xffffffff
    block_info_t* p_block;
    
    uint16_t init_flag;
    uint16_t data_count;        // 总共多少条数据
    uint8_t block_count;
    uint8_t fixed_size_flag:1;  // 片区大小是否固定标志
    uint8_t data_len:7;         // 每条数据长度
    uint16_t user_data_len;     // 用户数据长度
} flash_box_t;


//////////////////////////////////////////////////////////////////////
// @brief 时间戳转标签
__STATIC_INLINE uint32_t timestamps2tag(uint32_t timestamps)
{
    return timestamps / (24 * 3600);
}
// @brief 时间戳转序号
__STATIC_INLINE uint32_t timestamps2index(uint32_t timestamps)
{
    // 这里是一分钟一条
    return (timestamps % (24 * 3600)) / 60;
}
// @brief 时间戳递增
__STATIC_INLINE uint32_t timestamps_increase(uint32_t timestamps) 
{
    return (timestamps + 60);
}
// @brief 时间戳递减
__STATIC_INLINE uint32_t timestamps_decrease(uint32_t timestamps) 
{
    return (timestamps - 60);
}



//////////////////////////////////////////////////////////////////////
#define FLASH_BOX_DEFAULT(p_ftl, _block_len, _data_len, _fixed_size, _user_data_len)  do {\
    (p_ftl)->block_len  = (_block_len);         \
    (p_ftl)->data_len   = (_data_len);          \
    (p_ftl)->fixed_size_flag = (_fixed_size);   \
    (p_ftl)->user_data_len = (_user_data_len);  \
} while (0)


// @brief 数据初始化
int32_t htd_flashbox_init(flash_box_t* p_ftlb, const flash_config_t* p_cfg, block_info_t* p_block, uint8_t block_count);
// @brief 数据初始化
int32_t htd_flashbox_uninit(flash_box_t* p_ftlb);
// @brief 判断是否初始化
bool htd_flashbox_is_inited(flash_box_t* p_ftlb);
// @brief 指定序号添加数据
int32_t htd_flashbox_add(flash_box_t* p_ftlb, uint16_t tag, uint16_t index, const void* p_data);
// @brief 按序添加数据
int32_t htd_flashbox_append(flash_box_t* p_ftlb, uint16_t tag, const void* p_data);
// @brief 新增一个块
int32_t htd_flashbox_block_add(flash_box_t* p_ftlb, uint16_t tag);
// @brief 添加多个日志
bool htd_flashbox_append_lots(flash_box_t* p_ftlb, uint16_t tag, const void* p_data, uint16_t unit_len, uint8_t count);
// @brief 将一个块截短
int32_t htd_flashbox_block_trunc(flash_box_t* p_ftlb, uint16_t tag, uint32_t index);


// @brief 指定序号读取数据
int32_t htd_flashbox_get(flash_box_t* p_ftlb, uint16_t tag, uint16_t index, void* p_data);
// @brief 获取片区最大序列号
uint16_t htd_flashbox_max_serials_get(flash_box_t* p_ftlb, uint16_t tag);
// @brief 轮训获取标签
uint16_t htd_flashbox_tag_get(const flash_box_t* p_ftlb, uint16_t tag, bool is_forward);
// @brief 获取最后的标签
uint16_t htd_flashbox_last_tag_get(const flash_box_t* p_ftlb);
// @brief 获取最后一个数值
int32_t htd_flashbox_last_get(flash_box_t* p_ftlb, uint16_t tag, void* p_data);

// @brief 是否存在某个片区
bool htd_flashbox_block_check(flash_box_t* p_ftlb, uint16_t tag);
// @brief 获取某个片区的起始序列号和结束序列号
bool htd_flashbox_block_serials_get(flash_box_t* p_ftlb, uint16_t tag, uint16_t* p_sidx, uint16_t* p_eidx);
// @brief 擦除整块数据
int32_t htd_flashbox_clear(flash_box_t* p_ftlb);
// @brief 擦除一片数据
void htd_flashbox_block_clear(flash_box_t* p_ftlb, uint16_t tag);

// @brief 是否存在某个片区的某类数据
bool flash_box_block_data_check(flash_box_t* p_ftlb, uint16_t tag, uint32_t data_bit_flag);
// @brief 擦除某一块的某类数据
void flash_box_block_data_clear(flash_box_t* p_ftlb, uint16_t tag, uint32_t data_bit_flag);

// @brief 获取用户数据
int32_t htd_flashbox_user_data_read(flash_box_t* p_ftlb, uint16_t tag, uint32_t offset, void* p_data, uint16_t data_size);
// @brief 写入用户数据
int32_t htd_flashbox_user_data_write(flash_box_t* p_ftlb, uint16_t tag, uint32_t offset, const void* p_data, uint16_t data_len);

// @brief 连续读取多个数据
// @param pos 偏移几个字节, len 获取几个字节
int32_t htd_flashbox_get_lots_continuity(flash_box_t* p_ftlb, uint16_t tag, uint32_t index, uint8_t* p_data, uint8_t pos, uint8_t len, uint32_t* p_count, bool* p_is_end);
// @brief 计算一个块有多大
uint32_t htd_flashbox_block_size_get(uint32_t data_size, uint32_t user_size, uint32_t erase_unit);
// @brief 打印所有片区信息
void htd_flashbox_block_info_print(flash_box_t* p_ftlb);



#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_FLASH_BOX_H_*/

