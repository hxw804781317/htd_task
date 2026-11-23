// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_VFS_LFS_H_
#define _HTD_VFS_LFS_H_

#include "htd_vfs_port.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


typedef struct {
    uint32_t block_size;        // 最小的擦除单位大小，单位字节
    uint32_t block_count;       // 单元数量
    uint32_t phy_addr;          // 读写起始地址
    int32_t (*write_handle)(uint32_t addr, const uint8_t* p_buff, uint32_t buff_len);
    int32_t (*read_handle)(uint32_t addr, uint8_t* p_buff, uint32_t buff_len);
    int32_t (*erase_handle)(uint32_t addr, uint32_t size);
} lfs_dev_t;


#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_VFS_LFS_H_*/

