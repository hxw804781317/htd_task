// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#include "htd_device.h"
#include "app_filesystem.h"


static int32_t spiffs_write_handle(uint32_t addr, uint32_t buff_len, uint8_t* p_buff)
{
    //print_warning("spiffs write. addr=0x%08x, %d", addr, buff_len);
    return (buff_len == htd_dev_norflash_write(addr, p_buff, buff_len)) ? 0 : -1;
}
static int32_t spiffs_read_handle(uint32_t addr, uint32_t buff_len, uint8_t* p_buff)
{
    //print_warning("spiffs read. addr=0x%08x, %d", addr, buff_len);
    return (buff_len == htd_dev_norflash_read(addr, p_buff, buff_len)) ? 0 : -1;
}
static int32_t spiffs_erase_handle(uint32_t addr, uint32_t size)
{
    return htd_dev_norflash_erase(addr, size);
}

// @brief 初始化文件系统
int app_filesystem_init(void)
{
    int32_t ret = 0;
    htd_vfs_init();

#if defined(_WIN32)
    ret = htd_vfs_mount(HTD_VFS_TYPE_WIN32, "/", NULL, false);
    if (ret != 0) {
        htd_print("mount win32 filesystem failed. ret=%d\r\n", ret);
        ASSERT(0);
    } else {
        htd_print("mount win32 filesystem success.\r\n");
    }
#endif
    return ret;
}

// @brief 挂载文件系统
int32_t app_filesystem_mount(void)
{
    uint32_t offset = 0;
    int32_t ret = 0;

    // 挂载ram fs
#if defined(HTD_VFS_USING_RAMFS)
    uint32_t mem_size = 1024*1024*1;
    uint8_t *p_mem = malloc(mem_size);
    ASSERT(p_mem);
    void* p_ramfs = htd_vfs_ramfs_create(p_mem, mem_size);
    ret = htd_vfs_mount(HTD_VFS_TYPE_RAMFS, "/ram", p_ramfs, false);
    if (ret != 0) {
        htd_print("mount ramfs failed. ret=%d\r\n", ret);
        ASSERT(0);
    } else {
        htd_print("mount ramfs success.\r\n");
    }
#endif

    // 挂载 fatfs
#if defined(HTD_VFS_USING_ELM_FATFS)
    elm_fatfs_dev_t *p_fatfs_dev = NULL;
    #if defined(HTD_VFS_USING_NORFLASH_DEV) 
        #if defined(HTD_VFS_USING_LIB_DHARA)
            p_fatfs_dev = vfs_fatfs_norflash_dhara_create(0, offset, 4*1024*1024, 4);
        #else
            p_fatfs_dev = vfs_fatfs_norflash_create(0, offset, 4*1024*1024);
        #endif
    #elif defined(HTD_VFS_USING_NANDFLASH_DEV) && defined(HTD_VFS_USING_LIB_DHARA)
        p_fatfs_dev = vfs_fatfs_nandflash_dhara_create(0, 0, 256, 4);
    #endif
    if (p_fatfs_dev) {
        ret = htd_vfs_mount(HTD_VFS_TYPE_ELM_FATFS, "/fatfs", p_fatfs_dev, true);
        if (ret != 0) {
            htd_print("mount fatfs failed. ret=%d\r\n", ret);
            ASSERT(0);
        } else {
            htd_print("mount fatfs success.\r\n");
        }
        offset += 4*1024*1024;
    }
#endif

    // 挂载 spiffs
#if defined(HTD_VFS_USING_SPIFFS)
    spiffs_dev_t spiffs_dev;
    spiffs_dev.write_handle = spiffs_write_handle;
    spiffs_dev.read_handle = spiffs_read_handle;
    spiffs_dev.erase_handle = spiffs_erase_handle;
    spiffs_dev.block_size = htd_dev_norflash_erase_unit_get();
    spiffs_dev.block_count = (4*1024*1024) / spiffs_dev.block_size;
    spiffs_dev.phy_addr = htd_dev_norflash_phy_saddr_get() + offset;
    offset += spiffs_dev.block_size * spiffs_dev.block_count;
    ret = htd_vfs_mount(HTD_VFS_TYPE_SPIFFS, "/spifs", &spiffs_dev, 0);
    if (ret != 0) {
        htd_print("mount spiffs failed. ret=%d\r\n", ret);
        ASSERT(0);
    } else {
        htd_print("mount spiffs success.\r\n");
    }
#endif

    // 挂载 little-fs
#if defined(HTD_VFS_USING_LITTLE_FS)
    lfs_dev_t lfs_dev;
    lfs_dev.write_handle = htd_dev_norflash_write;
    lfs_dev.read_handle = htd_dev_norflash_read;
    lfs_dev.erase_handle = htd_dev_norflash_erase;
    lfs_dev.block_size = htd_dev_norflash_erase_unit_get();
    lfs_dev.block_count = (4*1024*1024) / lfs_dev.block_size;
    lfs_dev.phy_addr = htd_dev_norflash_phy_saddr_get() + offset;
    offset += lfs_dev.block_size * lfs_dev.block_count;
    ret = htd_vfs_mount(HTD_VFS_TYPE_LITTLE_FS, "/lfs", &lfs_dev, 1);
    if (ret != 0) {
        htd_print("mount lfs failed. ret=%d\r\n", ret);
        ASSERT(0);
    } else {
        htd_print("mount lfs success.\r\n");
    }
#endif
    return ret;
}


// 根据块管理列表进行挂载





