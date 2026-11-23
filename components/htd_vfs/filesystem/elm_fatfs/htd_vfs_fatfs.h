// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_VFS_FATFS_H_
#define _HTD_VFS_FATFS_H_

#include "htd_vfs_port.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


typedef struct elm_fatfs_dev_s elm_fatfs_dev_t;
struct elm_fatfs_dev_s {
    uint32_t disk_idx:6;                // 第几个文件系统，起始值为0
    uint32_t erase_sector_count:10;     // 一次擦除多少个扇区
    uint32_t sector_size:16;            // 扇区大小
    uint32_t sector_count;              // 总共多少个扇区
    int32_t (*write_handle)(const elm_fatfs_dev_t* p_dev, const uint8_t *buff, uint32_t sector, uint32_t count);
    int32_t (*read_handle)(const elm_fatfs_dev_t* p_dev, uint8_t *buff, uint32_t sector, uint32_t count);
    int32_t (*sync_handle)(const elm_fatfs_dev_t* p_dev);
    void (*destroy_handle)(const elm_fatfs_dev_t* p_dev);
    uint32_t user_data_size;
    void* p_user_data;
    uint32_t phy_addr;              // 起始地址
};


// @brief 获取设备
const elm_fatfs_dev_t *vfs_fatfs_dev_get(uint8_t disk_idx);
// @brief 读扇区数据
// @param sector 第几个扇区，count 多少个扇区
int32_t vfs_fatfs_flash_read(uint8_t disk_idx, uint8_t *buff, uint32_t sector, uint32_t count);
// @brief 写扇区数据
int32_t vfs_fatfs_flash_write(uint8_t disk_idx, const uint8_t *buff, uint32_t sector, uint32_t count);
// @brief 同步
int32_t vfs_fatfs_flash_sync(uint8_t disk_idx);



// @brief 创建 norflash 文件系统设备
// @param start_addr 相对地址，size 大小
elm_fatfs_dev_t* vfs_fatfs_norflash_create(uint8_t disk_idx, uint32_t start_addr, uint32_t size);
// @brief 销毁文件系统设备
int32_t vfs_fatfs_norflash_destroy(elm_fatfs_dev_t* p_fatfs_dev);

// @brief 创建 norflash_dhara 文件系统设备
elm_fatfs_dev_t* vfs_fatfs_norflash_dhara_create(uint8_t disk_idx, uint32_t start_addr, uint32_t size, uint8_t gc_ratio);
// @brief 销毁文件系统设备
int32_t vfs_fatfs_norflash_dhara_destroy(elm_fatfs_dev_t* p_fatfs_dev);

// @brief 创建 nandflash_dhara 文件系统设备
// @param start_block_id 起始块id，block_count 块数量， gc_ratio dhara垃圾回收参数
elm_fatfs_dev_t* vfs_fatfs_nandflash_dhara_create(uint8_t disk_idx, uint32_t start_block_id, uint32_t block_count, uint8_t gc_ratio);
// @brief 销毁文件系统设备
int32_t vfs_fatfs_nandflash_dhara_destroy(elm_fatfs_dev_t* p_fatfs_dev);

// @brief 创建 sdcard 文件系统设备
elm_fatfs_dev_t* vfs_fatfs_sdcard_create(uint8_t disk_idx, uint32_t block_idx, uint32_t block_count);
// @brief 销毁文件系统设备
int32_t vfs_fatfs_sdcard_destroy(elm_fatfs_dev_t* p_fatfs_dev);


#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_VFS_FATFS_H_*/

