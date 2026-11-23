// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_VFS_SPIFFS_H_
#define _HTD_VFS_SPIFFS_H_

#include "htd_vfs_port.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

typedef struct {
    uint16_t block_size;                // 擦除大小
    uint32_t block_count;
    uint32_t phy_addr;                  // 起始地址
    int32_t (*write_handle)(uint32_t addr, uint32_t buff_len, uint8_t* p_buff);
    int32_t (*read_handle)(uint32_t addr, uint32_t buff_len, uint8_t* p_buff);
    int32_t (*erase_handle)(uint32_t addr, uint32_t size);
} spiffs_dev_t;


// @brief 加锁
void vfs_spiffs_lock(void* p_fs);
// @brief 解锁
void vfs_spiffs_unlock(void* p_fs);

#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_VFS_SPIFFS_H_*/

