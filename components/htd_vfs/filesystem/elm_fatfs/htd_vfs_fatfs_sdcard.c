// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
//#define DEBUG_LVL   4
#define DEBUG_TAG              "fatfs.sdcard"
#include <htd_vfs_process.h>

#if defined(HTD_VFS_USING_SDCARD_DEV) && defined(HTD_VFS_USING_ELM_FATFS) && defined(HTD_USING_SDCARD)
#include <ff.h>
#include <diskio.h>


// @brief 读扇区数据
// @param sector 第几个扇区，count 多少个扇区
static int32_t fatfs_sdcard_read(const elm_fatfs_dev_t* p_dev, uint8_t *buff, uint32_t sector, uint32_t count)
{
    sector += p_dev->phy_addr;
    if (vfs_sdcard_read(p_dev->p_user_data, buff, sector, count) > 0) {
        return RES_OK;
    }
    return RES_ERROR;
}

// @brief 写扇区数据
static int32_t fatfs_sdcard_write(const elm_fatfs_dev_t* p_dev, const uint8_t *buff, uint32_t sector, uint32_t count)
{
    sector += p_dev->phy_addr;
    if (vfs_sdcard_write(p_dev->p_user_data, buff, sector, count) > 0) {
        return RES_OK;
    }
    return RES_ERROR;
}

// @brief 同步
static int32_t fatfs_sdcard_sync(const elm_fatfs_dev_t* p_dev)
{
    return RES_OK;
}

// @brief 销毁 sdcard 文件系统设备
static void fatfs_sdcard_destroy(const elm_fatfs_dev_t* p_fatfs_dev)
{
    if (p_fatfs_dev) {
        if (p_fatfs_dev->p_user_data) {
            vfs_sdcard_close(p_fatfs_dev->p_user_data);
        }
        htd_free(p_fatfs_dev);
    }
}

// @brief 创建 sdcard 文件系统设备
elm_fatfs_dev_t* vfs_fatfs_sdcard_create(uint8_t disk_idx, uint32_t block_idx, uint32_t block_count)
{
    void* sdcard_handle = vfs_sdcard_open();
    if (sdcard_handle == NULL) {
        return NULL;
    }

    uint32_t sdcard_block_count, sdcard_block_size;
    if (0 != vfs_sdcard_block_info_get(sdcard_handle, &sdcard_block_count, &sdcard_block_size)) {
        return NULL;
    }
    if ((block_idx + block_count) > sdcard_block_count) {
        block_count = sdcard_block_count - block_idx;
    }
    
    elm_fatfs_dev_t* p_fatfs_dev = (elm_fatfs_dev_t*)htd_malloc(sizeof(elm_fatfs_dev_t));
    ASSERT(p_fatfs_dev);
    htd_memset(p_fatfs_dev, 0, sizeof(elm_fatfs_dev_t));
    p_fatfs_dev->disk_idx = disk_idx;
    p_fatfs_dev->sector_size = sdcard_block_size;
    p_fatfs_dev->sector_count = block_count;
    p_fatfs_dev->erase_sector_count = 1;
    p_fatfs_dev->phy_addr = block_idx;
    p_fatfs_dev->write_handle = fatfs_sdcard_write;
    p_fatfs_dev->read_handle = fatfs_sdcard_read;
    p_fatfs_dev->destroy_handle = fatfs_sdcard_destroy;
    p_fatfs_dev->sync_handle = fatfs_sdcard_sync;
    p_fatfs_dev->p_user_data = sdcard_handle;
    return p_fatfs_dev;
}

// @brief 销毁文件系统设备
int32_t vfs_fatfs_sdcard_destroy(elm_fatfs_dev_t* p_fatfs_dev)
{
    if (p_fatfs_dev) {
        p_fatfs_dev->destroy_handle(p_fatfs_dev);
    }
    return RES_OK;
}
#endif /*defined(HTD_VFS_USING_SDCARD_DEV) && defined(HTD_VFS_USING_ELM_FATFS) && defined(HTD_USING_SDCARD)*/

