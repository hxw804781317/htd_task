// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL   4
#define DEBUG_TAG              "fatfs.nor"
#include <htd_vfs_process.h>

#if defined(HTD_VFS_USING_NORFLASH_DEV) && defined(HTD_VFS_USING_ELM_FATFS) && defined(HTD_USING_NORFLASH)
#include <ff.h>
#include <diskio.h>

// @brief 读扇区数据
// @param sector 第几个扇区，count 多少个扇区
static int32_t fatfs_norflash_read(const elm_fatfs_dev_t* p_dev, uint8_t *buff, uint32_t sector, uint32_t count)
{
    uint32_t addr = p_dev->phy_addr + sector * p_dev->sector_size;
    int32_t buff_len = p_dev->sector_size * count;
    int32_t ret = vfs_norflash_read(addr, buff, buff_len);      // norflash 驱动接口
    if (buff_len != ret) {
        print_error("disk read failed. ret=%d, buff_len=%d", ret, buff_len);
        return RES_ERROR;
    }
    return 0;
}

// @brief 写扇区数据
static int32_t fatfs_norflash_write(const elm_fatfs_dev_t* p_dev, const uint8_t *buff, uint32_t sector, uint32_t count)
{
    uint32_t addr = p_dev->phy_addr + sector * p_dev->sector_size;
    int32_t buff_len = p_dev->sector_size * count;
    
    vfs_norflash_erase(addr, p_dev->sector_size * p_dev->erase_sector_count);
    int32_t ret = vfs_norflash_write(addr, buff, buff_len);     // norflash 驱动接口
    if (buff_len != ret) {
        print_error("disk write failed. ret=%d, buff_len=%d", ret, buff_len);
        return RES_ERROR;
    }
    return 0;
}

// @brief 同步
static int32_t fatfs_norflash_sync(const elm_fatfs_dev_t* p_dev)
{
    return 0;
}

// @brief 销毁 norflash 文件系统设备
static void fatfs_norflash_destroy(const elm_fatfs_dev_t* p_fatfs_dev)
{
    if (p_fatfs_dev) {
        htd_free(p_fatfs_dev);
    }
}

// @brief 创建 norflash 文件系统设备
// @param start_addr 相对地址，size 大小
elm_fatfs_dev_t* vfs_fatfs_norflash_create(uint8_t disk_idx, uint32_t start_addr, uint32_t size)
{
    elm_fatfs_dev_t* p_fatfs_dev = (elm_fatfs_dev_t*)htd_malloc(sizeof(elm_fatfs_dev_t));
    ASSERT(p_fatfs_dev);
    uint32_t sector_size = vfs_norflash_erase_unit_get();
    htd_memset(p_fatfs_dev, 0, sizeof(elm_fatfs_dev_t));
    p_fatfs_dev->disk_idx = disk_idx;
    p_fatfs_dev->sector_size = sector_size;
    p_fatfs_dev->sector_count = size / p_fatfs_dev->sector_size;
    p_fatfs_dev->erase_sector_count = 1;
    p_fatfs_dev->phy_addr = vfs_norflash_phy_saddr_get() + start_addr;
    p_fatfs_dev->write_handle = fatfs_norflash_write;
    p_fatfs_dev->read_handle = fatfs_norflash_read;
    p_fatfs_dev->destroy_handle = fatfs_norflash_destroy;
    p_fatfs_dev->sync_handle = fatfs_norflash_sync;
    return p_fatfs_dev;
}

// @brief 销毁文件系统设备
int32_t vfs_fatfs_norflash_destroy(elm_fatfs_dev_t* p_fatfs_dev)
{
    if (p_fatfs_dev) {
        p_fatfs_dev->destroy_handle(p_fatfs_dev);
    }
    return RES_OK;
}
#endif /*defined(VFS_USING_NORFLASH_DEV) && defined(VFS_USING_ELM_FATFS)*/

