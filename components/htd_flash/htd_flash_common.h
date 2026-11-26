// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_FLASH_COMMON_H_
#define _HTD_FLASH_COMMON_H_

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>
#include "htd_flash_section.h"
#include "htd_flash_port.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


//////////////////////////////////////////////////////////////////////
#ifndef ARRAY_COUNT
    #define ARRAY_COUNT(X)              (sizeof(X) / sizeof(X[0]))
#endif
#ifndef ALIGN_DOWN
    #define ALIGN_DOWN(n, align)        ((n) & ~((align) - 1))
#endif 
#ifndef ALIGN_UP
    #define ALIGN_UP(n, align)          (((n) + (align) - 1) & ~((align) - 1))
#endif 
// 判断是否为2的指数倍数
#ifndef IS_POWER_OF_TWO
    #define IS_POWER_OF_TWO(A)          (((A) != 0) && ((((A) - 1) & (A)) == 0))
#endif
#ifndef OFFSETOF
    #if defined(__SES_ARM) && defined(__GNUC__)
        #define OFFSETOF(T, x)          __builtin_offsetof(T, x)
    #else
        #define OFFSETOF(T, x)          ((size_t)&((T*)0)->x)
    #endif
#endif


// flash 操作句柄
typedef struct {
    uint32_t magic;
    const char* p_name;
    uint32_t page_size;                 // 页大小
    uint32_t erase_unit;                // 擦除单位大小
    void (*flash_write)(uint32_t addr, const uint8_t* p_buff, uint32_t buff_len);
    void (*flash_read)(uint32_t addr, uint8_t* p_buff, uint32_t buff_len); 
    void (*flash_erase)(uint32_t addr, uint32_t size);
    void (*flash_sync)(void);
} flash_handle_t;
#define HTD_FLASH_HANDLE_MAGIC               0x99ad66aa
#define HTD_FLASH_HANDLE_SECTION_NAME        htd_flash_handle
#define HTD_FLASH_HANDLE_REGISTER(name, _page_size, _erase_unit, _read_handle, _write_handle, _erase_handle, _sync_handle)   \
    HTD_SECTION_ITEM_REGISTER(HTD_FLASH_HANDLE_SECTION_NAME, static const flash_handle_t CONCAT_2(m_flash_, name)) = {\
        .magic = HTD_FLASH_HANDLE_MAGIC,    \
        .p_name = STR(name),                \
        .page_size = _page_size,            \
        .erase_unit = _erase_unit,          \
        .flash_write = _write_handle,       \
        .flash_read = _read_handle,         \
        .flash_erase = _erase_handle,       \
        .flash_sync = _sync_handle,         \
    };

// flash 配置选项
typedef struct {
    const char* p_name;                 // HTD_FLASH_HANDLE_REGISTER 注册的名字
    uint32_t start;                     // 起始地址
    uint32_t end;                       // 结束地址
    void (*lock_handle)(void);          // 锁接口
    void (*unlock_handle)(void);        // 解锁接口
} flash_config_t;
#define HTD_FLASH_MUTEX_LOCK(p_fconfig)         if ((p_fconfig)->lock_handle) (p_fconfig)->lock_handle()
#define HTD_FLASH_MUTEX_UNLOCK(p_fconfig)       if ((p_fconfig)->unlock_handle) (p_fconfig)->unlock_handle()





//////////////////////////////////////////////////////////////////////
// @brief flash 初始化
void htd_flash_init(void (*lock_handle)(void), void (*unlock_handle)(void));
// @brief 查找 flash_handle_t
const flash_handle_t* htd_flash_handle_find(const char* p_name);
// @brief 擦除内部 flash 数据
int32_t htd_flash_erase(const flash_handle_t* p_flash, uint32_t page_addr);
// @brief 读取 flash 数据，支持跨页读取
int32_t htd_flash_read(const flash_handle_t* p_flash, uint32_t addr, void* p_dst, uint32_t len);
// @brief 写入 flash 数据，支持跨页写入，并支持跨页擦写
int32_t htd_flash_write(const flash_handle_t* p_flash, uint32_t addr, const void* p_data, uint32_t data_len);
// @brief 同步数据
void htd_flash_sync(const flash_handle_t* p_flash);


// @brief 在指定范围内擦除
int32_t htd_flash_range_erase(const flash_config_t* p_fconfig, const flash_handle_t* p_flash, uint32_t page_addr, uint32_t page_count);
// @brief 在p_cfg的范围内读取数据
void htd_flash_range_read(const flash_config_t* p_fconfig, const flash_handle_t* p_flash, uint32_t addr, void* p_data, uint16_t data_len);
// @brief 在配置的范围内写入数据
void htd_flash_range_write(const flash_config_t* p_fconfig, const flash_handle_t* p_flash, uint32_t addr, const void* p_data, uint32_t data_len);


// @brief 获取上一页地址
uint32_t htd_flash_prev_page_addr_get(const flash_config_t* p_fconfig, const flash_handle_t* p_flash, uint32_t addr);
// @brief 获取下一页地址
uint32_t htd_flash_next_page_addr_get(const flash_config_t* p_fconfig, const flash_handle_t* p_flash, uint32_t addr);
// @brief 获取两点的距离
uint32_t htd_flash_addr_distance(const flash_config_t* p_fconfig, uint32_t pos1, uint32_t pos2);
// @brief 判断地址是否在某个范围内
bool htd_flash_addr_in_range(const flash_config_t* p_fconfig, uint32_t start, uint32_t end, uint32_t addr);
// @brief 根据地址和距离获取下一个地址
uint32_t htd_flash_next_addr_get(const flash_config_t* p_fconfig, uint32_t addr, uint32_t distance);
// @brief 判断内存每个字节是否都是 val
bool htd_flash_mem_allof(const uint8_t *dst, uint8_t val, uint16_t n);




//////////////////////////////////////////////////////////////////
// 是否为擦除地址
#define IS_ERASE_ADDR(addr, erase_unit)     (0 == ((addr) & ((erase_unit) - 1)))
// 获取擦除地址
#define ERASE_ADDR(addr, erase_unit)        ((addr) & (~((erase_unit) - 1)))

// @brief 是否为擦除地址
__STATIC_INLINE bool htd_flash_is_erase_addr(const flash_handle_t* p_flash, uint32_t addr)
{
    return IS_ERASE_ADDR(addr, p_flash->erase_unit);
}
// @brief 获取 addr 的擦除地址
__STATIC_INLINE uint32_t htd_flash_erase_addr(const flash_handle_t* p_flash, uint32_t addr)
{
    return ERASE_ADDR(addr, p_flash->erase_unit);
}
// @brief 获取 addr 的下一页地址
__STATIC_INLINE uint32_t htd_flash_next_page_addr(const flash_handle_t* p_flash, uint32_t addr)
{
    return (ERASE_ADDR(addr, p_flash->erase_unit) + p_flash->erase_unit);
}
// @brief 获取当前页剩余长度
__STATIC_INLINE uint32_t htd_flash_page_remain_len(const flash_handle_t* p_flash, uint32_t addr)
{
    return (p_flash->erase_unit - (addr & (p_flash->erase_unit - 1)));
}
// @brief 
__STATIC_INLINE uint32_t htd_flash_page_position_get(const flash_handle_t* p_flash, uint32_t addr)
{
    return (addr & (p_flash->erase_unit - 1));
}
// @brief 判断是否为同一页
__STATIC_INLINE bool htd_flash_is_same_page(const flash_handle_t* p_flash, uint32_t addr1, uint32_t addr2)
{
    return ERASE_ADDR(addr1, p_flash->erase_unit) == ERASE_ADDR(addr2, p_flash->erase_unit);
}


#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_FLASH_COMMON_H_*/

