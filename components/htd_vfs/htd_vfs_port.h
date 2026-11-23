// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_VFS_PORT_H_
#define _HTD_VFS_PORT_H_

#include "htd_task.h"
#include "htd_vfs_section.h"
#include "htd_vfs_slist.h"
#include "htd_vfs_common.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/



//////////////////////////////////////////////////////////////////////
#ifndef ASSERT
    #define ASSERT(a)                   ASSERT(a)
#endif
#ifndef htd_print
    #define htd_print(fmt, ...)         do {printf(fmt, ##__VA_ARGS__);}while(0)
#endif /*vfs_print*/
#ifndef htd_snprintf
    #define htd_snprintf(buf, buff_size, fmt, ...)  snprintf(buf, buff_size, fmt, ##__VA_ARGS__)
#endif
#ifndef htd_memset
    #define htd_memset                  memset
#endif
#ifndef htd_memcpy
    #define htd_memcpy                  memcpy
#endif
#ifndef htd_memmove
    #define htd_memmove                 memmove
#endif
#ifndef htd_memcmp
    #define htd_memcmp                  memcmp
#endif
#ifndef htd_strlen
    #define htd_strlen                  strlen
#endif
#ifndef htd_strnlen
    #define htd_strnlen                 strnlen
#endif
#ifndef htd_strcpy
    #define htd_strcpy                  strcpy
#endif
#ifndef htd_strncpy
    #define htd_strncpy                 strncpy
#endif  
#ifndef htd_strcmp
    #define htd_strcmp                  strcmp
#endif
#ifndef htd_strcasecmp
    #define htd_strcasecmp              strcasecmp
#endif
#ifndef htd_strstr
    #define htd_strstr                  strstr
#endif
#ifndef htd_malloc
    #define htd_malloc(n)               malloc(n)
#endif /*vfs_malloc*/
#ifndef htd_free
    #define htd_free(p)                 free(p)
#endif /*vfs_free*/



//////////////////////////////////////////////////////////////////////
typedef htd_mutex_lock_t vfs_mutex_lock_t;

// @brief 创建互斥锁
static inline vfs_mutex_lock_t* vfs_mutex_lock_create(const char* p_name)
{
    return htd_mutex_lock_create(p_name);
}
// @brief 销毁锁
static inline void vfs_mutex_lock_destroy(vfs_mutex_lock_t* p_lock)
{
    htd_mutex_lock_destroy(p_lock);
}
// @brief 加锁
static inline void vfs_mutex_lock(vfs_mutex_lock_t* p_lock)
{
    htd_mutex_lock(p_lock);
}
// @brief 解锁
static inline void vfs_mutex_unlock(vfs_mutex_lock_t* p_lock)
{
    htd_mutex_unlock(p_lock);
}




//////////////////////////////////////////////////////////////////////
#if defined(HTD_VFS_USING_NANDFLASH_DEV) && defined(HTD_USING_NANDFLASH)
#include "htd_dev_nandflash.h"
// @brief 读页数据(不读oob数据), page_id 为相对页序号
// 注意：该接口需要做好坏块管理和ECC校正
static inline int32_t vfs_nandflash_page_read(uint32_t page_id, uint32_t offset, uint8_t* p_data, uint32_t data_len)
{
    return htd_dev_nandflash_page_read(page_id, offset, p_data, data_len);
}
// @brief 写页数据(不写oob数据), page_id 为相对页序号
// 注意：该接口需要做好坏块管理和ECC校正
static inline int32_t vfs_nandflash_page_write(uint32_t page_id, const uint8_t* p_data, uint32_t data_len)
{
    return htd_dev_nandflash_page_write(page_id, p_data, data_len);
}
// @brief 擦除一块数据, block_id 为相对块序号
// 注意：该接口需要做好坏块管理和ECC校正
static inline int32_t vfs_nandflash_block_erase(uint32_t block_id)
{
    return htd_dev_nandflash_block_erase(block_id);
}
// @brief 获取页数据大小
static inline uint32_t vfs_nandflash_page_data_size_get(void)
{
    return htd_dev_nandflash_page_data_size_get();
}
// @brief 获取 oob 大小
static inline uint32_t vfs_nandflash_page_spare_size_get(void)
{
    return htd_dev_nandflash_page_spare_size_get();
}
// @brief 获取一个块多少页
static inline uint32_t vfs_nandflash_pages_per_block_get(void)
{
    return htd_dev_nandflash_pages_per_block_get();
}
#endif /*defined(HTD_VFS_USING_NANDFLASH_DEV) && defined(HTD_USING_NANDFLASH)*/


//////////////////////////////////////////////////////////////////////
#if defined(HTD_VFS_USING_NORFLASH_DEV) && defined(HTD_USING_NORFLASH)
#include "htd_dev_norflash.h"
// @brief 读数据, addr 为绝对地址
static inline int32_t vfs_norflash_read(uint32_t addr, uint8_t* p_data, uint32_t data_len)
{
    return htd_dev_norflash_read(addr, p_data, data_len);
}
// @brief 写数据, addr 为绝对地址
static inline int32_t vfs_norflash_write(uint32_t addr, const uint8_t* p_data, uint32_t data_len)
{
    return htd_dev_norflash_write(addr, p_data, data_len);
}
// @brief 擦除某一块数据, addr 为绝对地址
static inline int32_t vfs_norflash_erase(uint32_t addr, uint32_t size)
{
    return htd_dev_norflash_erase(addr, size);
}
// @brief 获取起始地址, 失败返回0xffffffff
static inline uint32_t vfs_norflash_phy_saddr_get(void)
{
    return htd_dev_norflash_phy_saddr_get();
}
// @brief 获取擦除的大小，单位字节;
static inline uint32_t vfs_norflash_erase_unit_get(void)
{
    return htd_dev_norflash_erase_unit_get();
}
#endif /*defined(HTD_VFS_USING_NORFLASH_DEV) && defined(HTD_USING_NORFLASH)*/


#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_VFS_PORT_H_*/

