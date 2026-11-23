// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
//#define DEBUG_LVL   4
#define DEBUG_TAG              "fatfs.nand"
#include <htd_vfs_process.h>


#if defined(HTD_VFS_USING_LIB_DHARA) && defined(HTD_VFS_USING_ELM_FATFS) && defined(HTD_VFS_USING_NANDFLASH_DEV) && defined(HTD_USING_NANDFLASH)
#include <ff.h>
#include <diskio.h>
#include "dhara_port.h"
#include "map.h"

// @brief 读数据
static int32_t fatfs_nandflash_dhara_read(const elm_fatfs_dev_t* p_dev, uint8_t *buff, uint32_t sector, uint32_t count)
{
    dhara_error_t err_code = DHARA_E_NONE;
    while (count--) {
        print_debug("dhara read. sector=%d, sector_size=%d", sector, p_dev->sector_size);
        if (0 != dhara_map_read((struct dhara_map*)p_dev->p_user_data, sector, buff, &err_code)) {
            print_error("dhara read failed. %s", dhara_strerror(err_code));
            return -1;
        }
        buff += p_dev->sector_size;
        sector++;
    }
    return 0;
}

// @brief 写数据
static int32_t fatfs_nandflash_dhara_write(const elm_fatfs_dev_t* p_dev, const uint8_t *buff, uint32_t sector, uint32_t count)
{
    dhara_error_t err_code = DHARA_E_NONE;
    while (count--) {
        print_debug("dhara write. sector=%d, sector_size=%d", sector, p_dev->sector_size);
        if (0 != dhara_map_write((struct dhara_map*)p_dev->p_user_data, sector, buff, &err_code)) {
            print_error("dhara write failed. %s", dhara_strerror(err_code));
            return -1;
        }
        buff += p_dev->sector_size;
        sector++;
    }
    return 0;
}

// @brief 同步
static int32_t fatfs_nandflash_dhara_sync(const elm_fatfs_dev_t* p_dev)
{
    dhara_error_t err_code = DHARA_E_NONE;
    if (0 != dhara_map_sync((struct dhara_map*)p_dev->p_user_data, &err_code)) {
        print_error("dhara sync failed. %s", dhara_strerror(err_code));
        return -1;
    }
    return 0;
}

// @brief 销毁 nandflash_dhara 文件系统设备
static void fatfs_nandflash_dhara_destroy(const elm_fatfs_dev_t* p_fatfs_dev)
{
    if (p_fatfs_dev) {
        htd_free(p_fatfs_dev);
    }
}

// @brief 创建 nandflash_dhara 文件系统设备
// @param start_block_id 起始块id，block_count 块数量， gc_ratio dhara垃圾回收参数
elm_fatfs_dev_t* vfs_fatfs_nandflash_dhara_create(uint8_t disk_idx, uint32_t start_block_id, uint32_t block_count, uint8_t gc_ratio)
{
    uint32_t pages_per_block = vfs_nandflash_pages_per_block_get();
    uint32_t page_size = vfs_nandflash_page_data_size_get();
    uint32_t user_data_size = page_size + sizeof(struct dhara_nand) + sizeof(struct dhara_map);

    // fatfs 文件系统读写接口对接
    elm_fatfs_dev_t* p_fatfs_dev = htd_malloc(sizeof(elm_fatfs_dev_t) + user_data_size);
    ASSERT(p_fatfs_dev);
    htd_memset(p_fatfs_dev, 0, sizeof(elm_fatfs_dev_t));
    p_fatfs_dev->disk_idx = disk_idx;
    p_fatfs_dev->sector_size = page_size;
    p_fatfs_dev->erase_sector_count = pages_per_block;
    p_fatfs_dev->write_handle = fatfs_nandflash_dhara_write;
    p_fatfs_dev->read_handle = fatfs_nandflash_dhara_read;
    p_fatfs_dev->destroy_handle = fatfs_nandflash_dhara_destroy;
    p_fatfs_dev->sync_handle = fatfs_nandflash_dhara_sync;
    p_fatfs_dev->user_data_size = user_data_size;

    // 硬件驱动接口和 dhara 库对接
    struct dhara_map* p_dhara = (struct dhara_map*)(p_fatfs_dev + 1);
    htd_memset(p_dhara, 0, sizeof(struct dhara_map));
    
    struct dhara_nand* p_dhara_dev = (struct dhara_nand*)(p_dhara + 1);
    htd_memset(p_dhara_dev, 0, sizeof(struct dhara_nand));
    p_dhara_dev->start_block_id = start_block_id;
    p_dhara_dev->num_blocks = block_count;
    p_dhara_dev->log2_page_size = dhara_log2_off_data(page_size);
    print_warning("page_sie=%d, log2_page_size=%d, block_count=%d", page_size, p_dhara_dev->log2_page_size, block_count);
    p_dhara_dev->log2_ppb = dhara_log2_off_data(pages_per_block);
    print_warning("pages_per_block=%d, log2_ppb=%d", pages_per_block, p_dhara_dev->log2_ppb);
    p_dhara_dev->block_erase_handle = vfs_nandflash_block_erase;
    p_dhara_dev->page_write_handle = vfs_nandflash_page_write; 
    p_dhara_dev->page_read_handle = vfs_nandflash_page_read;

    uint8_t* p_page_buff = (uint8_t*)(p_dhara_dev + 1);
    htd_memset(p_page_buff, 0, page_size);
    dhara_map_init(p_dhara, p_dhara_dev, p_page_buff, gc_ratio);
    
    dhara_error_t err_code = DHARA_E_NONE;
    if (0 != dhara_map_resume(p_dhara, &err_code)) {
        print_error("resume failed. ", dhara_strerror(err_code));
    }
    
    // gc=1; sector_count=14578(32768)
    // gc=2; sector_count=19608(32768)
    // gc=3; sector_count=22123(32768)
    // gc=4; sector_count=23632(32768)
    p_fatfs_dev->sector_count = dhara_map_capacity(p_dhara);
    p_fatfs_dev->p_user_data = p_dhara;
    htd_print("sector_count=%d, err_code=%d\n", p_fatfs_dev->sector_count, err_code);
    return p_fatfs_dev;
}

// @brief 销毁文件系统设备
int32_t vfs_fatfs_nandflash_dhara_destroy(elm_fatfs_dev_t* p_fatfs_dev)
{
    if (p_fatfs_dev) {
        p_fatfs_dev->destroy_handle(p_fatfs_dev);
    }
    return RES_OK;
}


#endif /*defined(HTD_VFS_USING_LIB_DHARA) && defined(HTD_VFS_USING_ELM_FATFS) && defined(HTD_VFS_USING_NANDFLASH_DEV) && defined(HTD_USING_NANDFLASH)*/

