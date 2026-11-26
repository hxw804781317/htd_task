// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL   4
#define DEBUG_TAG              "flash.common"
#include "htd_flash_common.h"



//////////////////////////////////////////////////////////////////
HTD_SECTION_DEF(HTD_FLASH_HANDLE_SECTION_NAME, flash_handle_t);
typedef void (*ftl_ctrl_t)(void);
static ftl_ctrl_t m_flash_lock_handle = NULL;
static ftl_ctrl_t m_flash_unlock_handle = NULL;
#define FLASH_LOCK()      do { if (m_flash_lock_handle) { m_flash_lock_handle();}} while(0)
#define FLASH_UNLOCK()    do { if (m_flash_unlock_handle) { m_flash_unlock_handle();}} while (0)




// @brief 查找 flash_handle_t
const flash_handle_t* htd_flash_handle_find(const char* p_name)
{
    ASSERT(p_name);
    uint32_t name_len = htd_strlen(p_name) + 1;
    const uint32_t* start = (uint32_t*)HTD_SECTION_START_ADDR(HTD_FLASH_HANDLE_SECTION_NAME);
    const uint32_t* end = (uint32_t*)HTD_SECTION_END_ADDR(HTD_FLASH_HANDLE_SECTION_NAME);
    flash_handle_t* p_dev;
    const uint32_t* p_item = htd_section_first_item_get(start, end);
    while (p_item) {
        p_dev = (flash_handle_t*)p_item;
        if (p_dev->magic == HTD_FLASH_HANDLE_MAGIC) {
            if (htd_memcmp(p_name, p_dev->p_name, name_len) == 0) {
                return p_dev;
            }
        }
        p_item = htd_section_next_item_get(p_item, end, sizeof(htd_dev_t));
    }
    return NULL;
}

// @brief 写入flash 数据
static int32_t htd_flash_write_without_erase(const flash_handle_t* p_flash, uint32_t addr, const void* p_src, uint32_t len)
{
    uint32_t offset = 0;
    uint32_t page_remained;
    uint32_t w_len;
    
    // 地址和长度是否按照4字节对齐
    if ((addr & 3) || (len & 3)) {
        print_warning("ftl write failed. addr=%08x, len=%d, p_src=%p", addr, len, p_src);
        return ERR_FLASH_INVALID_ARG;
    }
    
    page_remained = p_flash->page_size - (addr & (p_flash->page_size - 1));
    if (len <= page_remained) {
        p_flash->flash_write(addr, p_src, len);
    } else {
        p_flash->flash_write(addr+offset, (uint8_t*)p_src+offset, page_remained);
        offset += page_remained;
        len -= page_remained;

        w_len = p_flash->page_size;
        while (len) {
            if (len < p_flash->page_size) {
                w_len = len;
            }
            p_flash->flash_write(addr+offset, (uint8_t*)p_src+offset, w_len);
            offset += w_len;
            len -= w_len;
        }
    }
    return ERR_FLASH_SUCCESS;
}

// @brief 通过数据擦除 flash 
bool flash_erase_by_data(const flash_handle_t* p_flash, uint32_t addr, uint16_t data_len)
{
    uint32_t offset;
    
    ASSERT(p_flash);
    if (ERASE_ADDR(addr + data_len - 1, p_flash->erase_unit) > ERASE_ADDR(addr, p_flash->erase_unit)) {
        // 从 addr 页一直擦除到 addr + dataLen - 1 这一页
        offset = addr + p_flash->erase_unit;
        do {
            p_flash->flash_erase(offset, p_flash->erase_unit);
            offset += p_flash->erase_unit;
        } while (ERASE_ADDR(offset, p_flash->erase_unit) <= ERASE_ADDR(addr + data_len - 1, p_flash->erase_unit));
    } else if (IS_ERASE_ADDR(addr, p_flash->erase_unit)) {
        p_flash->flash_erase(addr, p_flash->erase_unit);
    } else {
        return false;
    }
    return true;
}

// @brief 擦除内部 flash 数据 
int32_t htd_flash_erase(const flash_handle_t* p_flash, uint32_t page_addr)
{
    ASSERT(p_flash);

    print_warning("erase: addr=%08x, erase_unit=%d", page_addr, p_flash->erase_unit);
    FLASH_LOCK();
    p_flash->flash_erase(page_addr, p_flash->erase_unit);
    FLASH_UNLOCK();
    return ERR_FLASH_SUCCESS;
}

// @brief 读取 flash 数据，支持跨页读取
int32_t htd_flash_read(const flash_handle_t* p_flash, uint32_t addr, void* p_dst, uint32_t len)
{
    uint32_t offset = 0;
    uint32_t page_remained;
    uint32_t rlen;

    ASSERT(p_flash);
    ASSERT(p_dst);

    // 地址和长度是否按照4字节对齐
    if ((addr & 3) || (len & 3) || NULL == p_dst) {
        print_warning("ftl read failed. addr=%08x, len=%08x, p_dst=%p", addr, len, p_dst);
        return ERR_FLASH_INVALID_ARG;
    }
    
    page_remained = p_flash->page_size - (addr & (p_flash->page_size - 1));
    if (len <= page_remained) {
        FLASH_LOCK();
        p_flash->flash_read(addr, p_dst, len);
        FLASH_UNLOCK();
        return ERR_FLASH_SUCCESS;
    } else {
        FLASH_LOCK();
        p_flash->flash_read(addr+offset, ((uint8_t*)p_dst)+offset, page_remained);
        offset += page_remained;
        len -= page_remained;

        rlen = p_flash->page_size;
        while (len > 0) {
            if (len < rlen) {
                rlen = len;
            }
            p_flash->flash_read(addr+offset, ((uint8_t*)p_dst)+offset, rlen);
            offset += rlen;
            len -= rlen;
        }
        FLASH_UNLOCK();
    }
    return ERR_FLASH_SUCCESS;
}

// @brief 写入 flash 数据，支持跨页写入，并支持跨页擦写
int32_t htd_flash_write(const flash_handle_t* p_flash, uint32_t addr, const void* p_data, uint32_t data_len)
{
    ASSERT(p_flash);
    ASSERT(p_data);
    ASSERT(data_len > 0);
    
    FLASH_LOCK();
    htd_flash_write_without_erase(p_flash, addr, p_data, data_len);
    FLASH_UNLOCK();
    return ERR_FLASH_SUCCESS;
}

// @brief 同步数据
void htd_flash_sync(const flash_handle_t* p_flash)
{
    if (p_flash->flash_sync) {
        FLASH_LOCK();
        p_flash->flash_sync();
        FLASH_UNLOCK();
    }
}

// @brief 在指定范围内擦除
int32_t htd_flash_range_erase(const flash_config_t* p_fconfig, const flash_handle_t* p_flash, uint32_t page_addr, uint32_t page_count)
{
    ASSERT(p_fconfig);
    ASSERT(page_addr < p_fconfig->end);

    print_warning("erase: addr=%08x, page_count=%d", page_addr, page_count);
    FLASH_LOCK();
    while (page_count--) {
        p_flash->flash_erase(page_addr, p_flash->erase_unit);
        page_addr += p_flash->erase_unit;
        if (page_addr >= p_fconfig->end) {
            page_addr = p_fconfig->start;
        }
    }
    FLASH_UNLOCK();
    return ERR_FLASH_SUCCESS;
}

// @brief 在配置的范围内读取数据
void htd_flash_range_read(const flash_config_t* p_fconfig, const flash_handle_t* p_flash, uint32_t addr, void* p_data, uint16_t data_len)
{
    uint16_t front;

    ASSERT(p_fconfig);
    ASSERT(p_data);
    ASSERT(addr < p_fconfig->end);
    
    if (addr + data_len > p_fconfig->end) {
        front = p_fconfig->end - addr;
        htd_flash_read(p_flash, addr, p_data, front);
        p_data = (void*)((uint8_t*)p_data + front);
        data_len -= front;
        htd_flash_read(p_flash, p_fconfig->start, p_data, data_len);
    } else {
        htd_flash_read(p_flash, addr, p_data, data_len);
    }
}

// @brief 在配置的范围内写入数据
void htd_flash_range_write(const flash_config_t* p_fconfig, const flash_handle_t* p_flash, uint32_t addr, const void* p_data, uint32_t data_len)
{
    uint32_t front;
    
    ASSERT(p_fconfig);
    ASSERT(p_data);
    ASSERT(addr < p_fconfig->end);
    if (addr + data_len > p_fconfig->end) {
        front = p_fconfig->end - addr;
        htd_flash_write(p_flash, addr, p_data, front);
        p_data = (void*)((uint8_t*)p_data + front);
        data_len -= front;
        htd_flash_write(p_flash, p_fconfig->start, p_data, data_len);
    } else {
        htd_flash_write(p_flash, addr, p_data, data_len);
    }
}

// @brief flash 初始化
void htd_flash_init(void (*lock_handle)(void), void (*unlock_handle)(void))
{
    if (NULL == m_flash_lock_handle && lock_handle) {
        m_flash_lock_handle = lock_handle;
    }
    if (NULL == m_flash_unlock_handle && unlock_handle) {
        m_flash_unlock_handle = unlock_handle;
    }
}

// @brief 获取上一页地址
uint32_t htd_flash_prev_page_addr_get(const flash_config_t* p_fconfig, const flash_handle_t* p_flash, uint32_t addr)
{
    addr = ERASE_ADDR(addr, p_flash->erase_unit);
    if (addr <= p_fconfig->start) {
        addr = p_fconfig->end;
    }
    return addr - p_flash->erase_unit;
}

// @brief 获取下一页地址
uint32_t htd_flash_next_page_addr_get(const flash_config_t* p_fconfig, const flash_handle_t* p_flash, uint32_t addr)
{
    addr = ERASE_ADDR(addr, p_flash->erase_unit) + p_flash->erase_unit;
    if (addr >= p_fconfig->end) {
        addr = p_fconfig->start;
    }
    return addr;
}

// @brief 获取两点的距离
uint32_t htd_flash_addr_distance(const flash_config_t* p_fconfig, uint32_t pos1, uint32_t pos2)
{
    if (pos1 <= pos2) {
        return pos2 - pos1;
    } else {
        return (p_fconfig->end - pos1) + (pos2 - p_fconfig->start);
    }
}

// @brief 判断地址是否在某个范围内
bool htd_flash_addr_in_range(const flash_config_t* p_fconfig, uint32_t start, uint32_t end, uint32_t addr)
{
    if (start < end) {
        return (addr >= start && addr < start);
    }
    return ((addr >= start && addr < p_fconfig->end) || (addr >= p_fconfig->start && addr < end));
}

// @brief 获取下一个地址
uint32_t htd_flash_next_addr_get(const flash_config_t* p_fconfig, uint32_t addr, uint32_t distance)
{
    addr += distance;
    if (addr >= p_fconfig->end) {
        return addr - p_fconfig->end + p_fconfig->start;
    }
    return addr;
}

// @brief 判断内存每个字节是否都是 val
bool htd_flash_mem_allof(const uint8_t *dst, uint8_t val, uint16_t n)
{
    while (n--) {
        if (*dst++ != val) {
            return false;
        }
    }
    return true;
}


