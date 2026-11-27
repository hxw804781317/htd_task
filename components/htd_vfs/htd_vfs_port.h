// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_VFS_PORT_H_
#define _HTD_VFS_PORT_H_

// 外部头文件
#include "htd_task.h"

// 本地头文件
#include "htd_vfs_common.h"
#include "htd_vfs_section.h"
#include "htd_vfs_slist.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/



//////////////////////////////////////////////////////////////////////
#ifndef ASSERT
    #define ASSERT(a)                   ASSERT(a)
#endif
#ifndef htd_snprintf
    #define htd_snprintf(buf, buff_size, fmt, ...)  snprintf(buf, buff_size, fmt, ##__VA_ARGS__)
#endif
#ifndef htd_memset
    #define htd_memset(ptr, v, n)       memset(ptr, v, n)
#endif
#ifndef htd_memcpy
    #define htd_memcpy(dst, src, n)     memcpy(dst, src, n)
#endif
#ifndef htd_memmove
    #define htd_memmove(dst, src, n)    memmove(dst, src, n)
#endif
#ifndef htd_memcmp
    #define htd_memcmp(v1, v2, n)       memcmp(v1, v2, n)
#endif
#ifndef htd_strlen
    #define htd_strlen(str)             strlen(str)
#endif
#ifndef htd_strcmp
    #define htd_strcmp(v1, v2, n)       strcmp(v1, v2, n)
#endif
#ifndef htd_strncpy
    #define htd_strncpy(dst, src, n)    strncpy(dst, src, n)
#endif
#ifndef htd_malloc
    #define htd_malloc(n)               malloc(n)
#endif 
#ifndef htd_free
    #define htd_free(p)                 free(p)
#endif 

// @brief 对接外部打印接口
extern int32_t htd_print(const char *fmt, ...);

#ifndef print_debug
    #define print_debug(...)            
#endif
#ifndef print_warning
    #define print_warning(...)          
#endif
#ifndef print_error
    #define print_error(...)            
#endif




//////////////////////////////////////////////////////////////////////
typedef htd_mutex_lock_t vfs_mutex_lock_t;

// @brief 创建互斥锁
__STATIC_INLINE vfs_mutex_lock_t* vfs_mutex_lock_create(const char* p_name)
{
    return htd_mutex_lock_create(p_name);
}
// @brief 销毁锁
__STATIC_INLINE void vfs_mutex_lock_destroy(vfs_mutex_lock_t* p_lock)
{
    htd_mutex_lock_destroy(p_lock);
}
// @brief 加锁
__STATIC_INLINE void vfs_mutex_lock(vfs_mutex_lock_t* p_lock)
{
    htd_mutex_lock(p_lock);
}
// @brief 解锁
__STATIC_INLINE void vfs_mutex_unlock(vfs_mutex_lock_t* p_lock)
{
    htd_mutex_unlock(p_lock);
}



//////////////////////////////////////////////////////////////////////
#if defined(HTD_VFS_USING_NANDFLASH_DEV) && defined(HTD_USING_NANDFLASH)
#include "htd_dev_nandflash.h"
// @brief 读页数据(不读oob数据), page_id 为相对页序号
// 注意：该接口需要做好坏块管理和ECC校正
__STATIC_INLINE int32_t vfs_nandflash_page_read(uint32_t page_id, uint32_t offset, uint8_t* p_data, uint32_t data_len)
{
    return htd_dev_nandflash_page_read(page_id, offset, p_data, data_len);
}
// @brief 写页数据(不写oob数据), page_id 为相对页序号
// 注意：该接口需要做好坏块管理和ECC校正
__STATIC_INLINE int32_t vfs_nandflash_page_write(uint32_t page_id, const uint8_t* p_data, uint32_t data_len)
{
    return htd_dev_nandflash_page_write(page_id, p_data, data_len);
}
// @brief 擦除一块数据, block_id 为相对块序号
// 注意：该接口需要做好坏块管理和ECC校正
__STATIC_INLINE int32_t vfs_nandflash_block_erase(uint32_t block_id)
{
    return htd_dev_nandflash_block_erase(block_id);
}
// @brief 获取页数据大小
__STATIC_INLINE uint32_t vfs_nandflash_page_data_size_get(void)
{
    return htd_dev_nandflash_page_data_size_get();
}
// @brief 获取 oob 大小
__STATIC_INLINE uint32_t vfs_nandflash_page_spare_size_get(void)
{
    return htd_dev_nandflash_page_spare_size_get();
}
// @brief 获取一个块多少页
__STATIC_INLINE uint32_t vfs_nandflash_pages_per_block_get(void)
{
    return htd_dev_nandflash_pages_per_block_get();
}
#endif /*defined(HTD_VFS_USING_NANDFLASH_DEV) && defined(HTD_USING_NANDFLASH)*/


//////////////////////////////////////////////////////////////////////
#if defined(HTD_VFS_USING_NORFLASH_DEV) && defined(HTD_USING_NORFLASH)
#include "htd_dev_norflash.h"
// @brief 读数据, addr 为绝对地址
__STATIC_INLINE int32_t vfs_norflash_read(uint32_t addr, uint8_t* p_data, uint32_t data_len)
{
    return htd_dev_norflash_read(addr, p_data, data_len);
}
// @brief 写数据, addr 为绝对地址
__STATIC_INLINE int32_t vfs_norflash_write(uint32_t addr, const uint8_t* p_data, uint32_t data_len)
{
    return htd_dev_norflash_write(addr, p_data, data_len);
}
// @brief 擦除某一块数据, addr 为绝对地址
__STATIC_INLINE int32_t vfs_norflash_erase(uint32_t addr, uint32_t size)
{
    return htd_dev_norflash_erase(addr, size);
}
// @brief 获取起始地址, 失败返回0xffffffff
__STATIC_INLINE uint32_t vfs_norflash_phy_saddr_get(void)
{
    return htd_dev_norflash_phy_saddr_get();
}
// @brief 获取擦除的大小，单位字节;
__STATIC_INLINE uint32_t vfs_norflash_erase_unit_get(void)
{
    return htd_dev_norflash_erase_unit_get();
}
#endif /*defined(HTD_VFS_USING_NORFLASH_DEV) && defined(HTD_USING_NORFLASH)*/


#if defined(HTD_VFS_USING_NORFLASH_DEV) && defined(HTD_USING_SDCARD)
//////////////////////////////////////////////////////////////////////
#include "htd_dev_sdcard.h"

// @brief 打开 sd-card
__STATIC_INLINE void* vfs_sdcard_open(void)
{
    return htd_dev_sdcard_open();
}
// @brief 关闭 sd-card
__STATIC_INLINE void vfs_sdcard_close(void* handle)
{
    htd_dev_sdcard_close((sdcard_handle_t)handle);
}
// @brief 读扇区
__STATIC_INLINE int32_t vfs_sdcard_read(void* handle, uint8_t *buff, uint32_t sector, uint32_t count)
{
    return htd_dev_sdcard_block_read((sdcard_handle_t)handle, sector, buff, count);
}
// @brief 写扇区数据
__STATIC_INLINE int32_t vfs_sdcard_write(void* handle, const uint8_t *buff, uint32_t sector, uint32_t count)
{
    return htd_dev_sdcard_block_write((sdcard_handle_t)handle, sector, buff, count);
}
// @brief 获取块数量
__STATIC_INLINE uint32_t vfs_sdcard_block_info_get(void* handle, uint32_t* block_count, uint32_t *block_size)
{
    dev_sdcard_info_t info;
    if (ERROR_CODE_SUCCESS != htd_dev_sdcard_info_get((sdcard_handle_t)handle, &info)) {
        return -1;
    }
    if (block_count) {
        block_count = info.block_count;
    }
    if (block_size) {
        block_size = info.block_size;
    }
    return 0;
}

#endif /*defined(HTD_VFS_USING_NORFLASH_DEV) && defined(HTD_USING_SDCARD)*/








#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_VFS_PORT_H_*/

