/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2025        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/
#define DEBUG_LVL 4
#define DEBUG_TAG              "disk"
#include "htd_vfs_fatfs.h"
#include "ff.h" 
#include "diskio.h"


// @brief 返回设备状态
// @param pdrv 设备id
DSTATUS disk_status(BYTE pdrv)
{
    return 0;
}

// @brief 初始化
// @param pdrv 设备id
DSTATUS disk_initialize(BYTE pdrv)
{
    return 0;
}

// @brief 读扇区数据
// @param sector 第几个扇区，count 多少个扇区
DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count)
{
    //vfs_print("[%02d]read. sector=%d, count=%d\n", pdrv, sector, count);
    return (DRESULT)vfs_fatfs_flash_read((uint8_t)pdrv, (uint8_t*)buff, (uint32_t)sector, (uint32_t)count);
}

#if FF_FS_READONLY == 0
// @brief 写扇区数据
DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count)
{
    //vfs_print("[%02d]write. sector=%d, count=%d\n", pdrv, sector, count);
    return (DRESULT)vfs_fatfs_flash_write((uint8_t)pdrv, (uint8_t*)buff, (uint32_t)sector, (uint32_t)count);
}
#endif

// @brief IO 控制
DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
    const elm_fatfs_dev_t* p_dev = vfs_fatfs_dev_get(pdrv);
    if (p_dev == NULL) {
        print_error("disk get device failed. pdrv=%d", pdrv);
        return RES_ERROR;
    }
    
    switch (cmd) {
        case CTRL_SYNC: {
            vfs_fatfs_flash_sync(pdrv);
            break;
        }
        case GET_SECTOR_COUNT: {
            *(DWORD*)buff = p_dev->sector_count;
            break;
        }
        case GET_SECTOR_SIZE: {
            *(WORD*)buff = p_dev->sector_size;
            break;
        }
        case GET_BLOCK_SIZE: {
            *(DWORD*)buff = p_dev->erase_sector_count;
            break;
        }
        case CTRL_TRIM: {           // 将这个 sector 标记为坏块
            LBA_t sector = *(LBA_t*)buff;
            uint32_t count = *(uint32_t*)((uint8_t*)buff+sizeof(uint32_t));
            print_warning("trim. sector=%d, count=%d", sector, count);
            break;
        }
        default: {
            print_error("unknown cmd: %d", cmd);
            return RES_ERROR;
        }
    }
    return RES_OK;
}


