// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL       4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG       "norflash"
#include "htd_device.h"
#include "htd_components.h"
#include "htd_dev_norflash.h"

static const dev_norflash_t *m_norflash_dev = NULL;
static htd_mutex_lock_t *m_norflash_lock = NULL;
#define norflash_dev_lock_init()           do { if (m_norflash_lock == NULL) {m_norflash_lock = htd_mutex_lock_create("norflash"); ASSERT(m_norflash_lock);} }while(0)
#define norflash_dev_lock()                do { htd_mutex_lock(m_norflash_lock);} while(0)
#define norflash_dev_unlock()              do { htd_mutex_unlock(m_norflash_lock);} while(0)


// @brief 开始
static void norflash_enable(bool is_enable)
{
    if (is_enable) {
        norflash_dev_lock();
    } else {
        norflash_dev_unlock();
    }
}

// @brief 注册(只能调用一次)
int32_t htd_dev_norflash_register(const dev_norflash_t* p_norflash)
{
    ASSERT(m_norflash_dev == NULL);
    ASSERT(p_norflash);
    ASSERT((NORFLASH_MAP_START_ADDR & (p_norflash->erase_unit - 1)) == 0);
    ASSERT(p_norflash->read_handle);
    ASSERT(p_norflash->write_handle);
    ASSERT(p_norflash->ctrl_handle);
    ASSERT(p_norflash->init_handle);
    ASSERT((p_norflash->erase_unit & 0x03) == 0);
    ASSERT((p_norflash->page_size & 0x03) == 0);
    m_norflash_dev = p_norflash;
    return 0;
}

// @brief 展示已经挂载的norflash信息
void htd_dev_norflash_show(void)
{
#if defined(DEBUG_LVL)
    if (m_norflash_dev == NULL) {
        print_warning("norflash unregistered.");
        return;
    }
    norflash_enable(true);
    print_log("norflash dev: %s", m_norflash_dev->p_name);
    print_log("\tphy_saddr: 0x%08x", NORFLASH_MAP_START_ADDR);
    print_log("\terase_unit=%d, page_size=%d", m_norflash_dev->erase_unit, m_norflash_dev->page_size);
    m_norflash_dev->ctrl_handle(NORFLASH_CMD_SHOW_INFO, NULL, 0);
    norflash_enable(false);
#endif
}

// @brief 读数据(支持addr不用按4字节对齐，data_len不用按4字节对齐，读取长度可以超过一页)
int32_t htd_dev_norflash_read(uint32_t addr, uint8_t* p_data, uint32_t data_len)
{
    const uint8_t* p_start = p_data;
    uint32_t rlen;
    uint8_t cache[4];
    uint32_t front = addr & (sizeof(cache)-1);
    
    ASSERT(m_norflash_dev);
    
    norflash_enable(true);
    // 先读地址未对齐部分
    if (front) {
        rlen = sizeof(cache) - front;
        if (rlen > data_len) {
            rlen = data_len;
        }
        addr -= front;
        htd_memset(cache, 0xff, sizeof(cache));
        m_norflash_dev->read_handle(addr, cache, sizeof(cache));
        addr += sizeof(cache);
        htd_memcpy(p_data, &cache[front], rlen);
        p_data += rlen;
        data_len -= rlen;
    }

    uint32_t remain_len = data_len & (sizeof(cache)-1);
    data_len -= remain_len;

    // 再读中间部分
    if (data_len) {
        uint32_t page_remained = m_norflash_dev->page_size - (addr & (m_norflash_dev->page_size - 1));
        if (data_len <= page_remained) {
            m_norflash_dev->read_handle(addr, p_data, data_len);
            addr += data_len;
            p_data += data_len;
        } else {
            m_norflash_dev->read_handle(addr, p_data, page_remained);
            addr += page_remained;
            p_data += page_remained;
            data_len -= page_remained;

            rlen = m_norflash_dev->page_size;
            while (data_len) {
                if (data_len < rlen) {
                    rlen = data_len;
                }
                m_norflash_dev->read_handle(addr, p_data, rlen);
                data_len -= rlen;
                addr += rlen;
                p_data += rlen;
            }
        }
    }

    // 最后读取长度剩余部分
    if (remain_len) {
        m_norflash_dev->read_handle(addr, cache, sizeof(cache));
        htd_memcpy(p_data, cache, remain_len);
        p_data += remain_len;
    }
    norflash_enable(false);
    return p_data - p_start;
}

// @brief 写数据(支持addr不用按4字节对齐，data_len不用按4字节对齐，写入长度可以超过一页)
int32_t htd_dev_norflash_write(uint32_t addr, const uint8_t* p_data, uint32_t data_len)
{
    uint32_t w_len;
    const uint8_t* p_start = p_data;
    uint8_t cache[4];
    uint32_t front = addr & (sizeof(cache)-1);

    ASSERT(m_norflash_dev);

    norflash_enable(true);
    // 先写前面地址未对齐部分
    if (front) {
        addr -= front;
        htd_memset(cache, 0xff, sizeof(cache));
        m_norflash_dev->read_handle(addr, cache, sizeof(cache));
        w_len = sizeof(cache) - front;
        if (w_len > data_len) {
            w_len = data_len;
        }
        htd_memcpy(&cache[front], p_data, w_len);
        p_data += w_len;
        data_len -= w_len;
        m_norflash_dev->write_handle(addr, cache, sizeof(cache));
        addr += sizeof(cache);
    }

    uint32_t remain_len = data_len & (sizeof(cache)-1);
    data_len -= remain_len;

    // 再写入中间数据
    if (data_len) {
        uint32_t page_remained = m_norflash_dev->page_size - (addr & (m_norflash_dev->page_size - 1));
        if (data_len <= page_remained) {
            m_norflash_dev->write_handle(addr, p_data, data_len);
            addr += data_len;
            p_data += data_len;
        } else {
            m_norflash_dev->write_handle(addr, p_data, page_remained);
            data_len -= page_remained;
            addr += page_remained;
            p_data += page_remained;

            w_len = m_norflash_dev->page_size;
            while (data_len) {
                if (data_len < m_norflash_dev->page_size) {
                    w_len = data_len;
                }
                m_norflash_dev->write_handle(addr, p_data, w_len);
                data_len -= w_len;
                addr += w_len;
                p_data += w_len;
            }
        }
    }

    // 再写入未对齐部分
    if (remain_len) {
        htd_memset(cache, 0xff, sizeof(cache));
        m_norflash_dev->read_handle(addr, cache, sizeof(cache));
        htd_memcpy(cache, p_data, remain_len);
        p_data += remain_len;
        m_norflash_dev->write_handle(addr, cache, sizeof(cache));
    }
    norflash_enable(false);
    return p_data - p_start;
}

// @brief 擦除某一块数据
int32_t htd_dev_norflash_erase(uint32_t addr, uint32_t size)
{
    norflash_erase_param_t param;
    ASSERT(m_norflash_dev);
    
    norflash_enable(true);
    param.addr = ALIGN_DOWN(addr, m_norflash_dev->erase_unit);
    param.size = ALIGN_UP(size, m_norflash_dev->erase_unit);
    m_norflash_dev->ctrl_handle(NORFLASH_CMD_SECTOR_ERASE, &param, sizeof(norflash_erase_param_t));
    norflash_enable(false);
    return 0;
}

// @brief 按页读数据(page_idx 为页序号)
// @node 这里的页是一个擦除单位的大小
uint32_t htd_dev_norflash_page_read(uint32_t page_idx, uint8_t* p_data, uint32_t data_len)
{
    ASSERT(m_norflash_dev);

    norflash_enable(true);
    uint32_t read_len = m_norflash_dev->erase_unit;
    uint32_t addr = NORFLASH_MAP_START_ADDR + page_idx * m_norflash_dev->erase_unit;

    uint8_t* p_start = p_data;
    while (data_len) {
        if (read_len > data_len) {
            read_len = data_len;
        }
        m_norflash_dev->read_handle(addr, p_data, read_len);
        p_data += read_len;
        addr += read_len;
        data_len -= read_len;
    }
    norflash_enable(false);
    return (p_data - p_start);
}

// @brief 按页写数据, 该接口先擦后写，外部不用调用擦除函数(page_idx 为页序号)
// @node 这里的页是一个擦除单位的大小
uint32_t htd_dev_norflash_page_write(uint32_t page_idx, const uint8_t* p_data, uint32_t data_len)
{
    ASSERT(m_norflash_dev);
    
    norflash_enable(true);
    uint32_t write_len = m_norflash_dev->erase_unit;
    uint32_t addr = NORFLASH_MAP_START_ADDR + page_idx * m_norflash_dev->erase_unit;
    
    norflash_erase_param_t param;
    const uint8_t* p_start = p_data;
    while (data_len) {
        if (write_len > data_len) {
            write_len = data_len;
        }
        param.addr = addr;
        param.size = m_norflash_dev->erase_unit;
        m_norflash_dev->ctrl_handle(NORFLASH_CMD_SECTOR_ERASE, &param, sizeof(norflash_erase_param_t));
        m_norflash_dev->write_handle(addr, p_data, write_len);
        p_data += write_len;
        addr += write_len;
        data_len -= write_len;
    }
    norflash_enable(false);
    return (p_data - p_start);
}

// @brief 获取起始地址, 失败返回0xffffffff
uint32_t htd_dev_norflash_phy_saddr_get(void)
{
    return NORFLASH_MAP_START_ADDR;
}

// @brief 获取结束地址, 失败返回0xffffffff
uint32_t htd_dev_norflash_phy_eaddr_get(void)
{
    uint32_t capacity = htd_dev_norflash_size_get();
    return NORFLASH_MAP_START_ADDR + capacity;
}

// @brief 获取大小
uint32_t htd_dev_norflash_size_get(void)
{
    ASSERT(m_norflash_dev);
    
    uint32_t param;
    norflash_enable(true);
    if (sizeof(param) != m_norflash_dev->ctrl_handle(NORFLASH_CMD_SIZE_GET, &param, sizeof(param))) {
        param = 0;
    } 
    norflash_enable(false);
    return param;
}

// @brief 获取擦除的大小，单位字节;
uint32_t htd_dev_norflash_erase_unit_get(void)
{
    ASSERT(m_norflash_dev);
    return m_norflash_dev->erase_unit;
}

// @brief 获取页大小
uint32_t htd_dev_norflash_page_size_get(void)
{
    ASSERT(m_norflash_dev);
    return m_norflash_dev->page_size;
}

// @brief 初始化
int32_t htd_dev_norflash_init(void)
{
    ASSERT(m_norflash_dev);     // 必须先运行 htd_dev_norflash_register 注册设备
    norflash_dev_lock_init();
    return m_norflash_dev->init_handle();
}

// @brief 去初始化
int32_t htd_dev_norflash_uninit(void)
{
    ASSERT(m_norflash_dev);
    
    return 0;
}

//////////////////////////////////////////////////////////////////////
#if defined(HTD_CONSOLE_ENABLE) && (DEBUG_LVL > 0) && 0
#include "htd_console_process.h"

// @brief 
static void norflash_test(void)
{
    uint8_t buff[256];
    uint32_t buff_len = sizeof(buff);
    htd_memset(buff, 'a', sizeof(buff));

    // 同页读写测试
    uint32_t addr = htd_dev_norflash_phy_saddr_get();
    uint32_t page_size = htd_dev_norflash_page_size_get();
    htd_dev_norflash_erase(addr, page_size*8);
    htd_dev_norflash_write(addr, buff, buff_len);
    htd_memset(buff, 0, buff_len);
    htd_dev_norflash_read(addr, buff, buff_len);
    if (false == htd_mem_allof(buff, 'a', buff_len)) {
        print_error("norflash write/read failed. addr=0x%08x, buff_len=%d", addr, buff_len);
        print_hex("data", buff, buff_len);
        return;
    }

    // 跨页读写测试, 同时地址和读写长度都不对齐
    buff_len = 254;
    htd_memset(buff, 'a', sizeof(buff));
    addr += page_size - 63;
    htd_dev_norflash_write(addr, buff, buff_len);
    htd_memset(buff, 0, buff_len);
    htd_dev_norflash_read(addr, buff, buff_len);
    if (false == htd_mem_allof(buff, 'a', buff_len)) {
        print_error("norflash write/read failed. buff_len=%d, addr=0x%08x", buff_len, addr);
        print_hex("data", buff, buff_len);
        return;
    }
    print_warning("norflash read/write test success.");
}

// @brief 调试命令
static int cmd_norflash(int argc, char **argv)
{
    if (argc != 2) {
        print_error("Usage: %s show/test", argv[0]);
        return 0;
    }
    
    if (htd_memcmp(argv[1], "show", 5) == 0) {
        htd_dev_norflash_show();
    } else if (htd_memcmp(argv[1], "test", 5) == 0) {
        norflash_test();
    } else {
        print_error("Usage: %s show/test", argv[0]);
    }
    return 0;
}
CONSOLE_CMD_EXPORT(cmd_norflash, norflash);

#endif /**/

