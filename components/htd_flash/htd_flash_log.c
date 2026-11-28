// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL   4
#define DEBUG_TAG              "flash.log"
#include "htd_flash_common.h"
#include "htd_flash_log.h"


#if defined(HTD_FLASH_LOG_ENABLE)
//////////////////////////////////////////////////////////////////////
// 允许写入的数据量为 4g * 4096 byte
typedef struct {
    uint32_t pidx;          // 页序号标志 page index
    uint16_t magic;
    uint16_t crc;           // 前 6 字节的 crc16
} flashlog_page_head_t;
const static uint16_t m_flashlog_default_magic = 0xf3f3;
const static uint16_t m_flashlog_init_flag = 0x1155;
const static uint32_t m_flashlog_pidx_max = 0xfffffff8;


//////////////////////////////////////////////////////////////////////
// @brief 参数检测
static int32_t flashlog_param_assert(const flash_config_t* p_cfg, const flash_handle_t* p_flash)
{
    ASSERT(p_cfg);
    ASSERT(p_flash);
    ASSERT(p_flash->flash_erase);
    ASSERT(p_flash->flash_read);
    ASSERT(p_flash->flash_write);
    
    if (! IS_POWER_OF_TWO(p_flash->erase_unit)) {
        print_error("error: erase_unit=%d", p_flash->erase_unit);
        return ERR_FLASH_INVALID_ARG;
    }
    if (! IS_ERASE_ADDR(p_cfg->start, p_flash->erase_unit)) {
        print_error("error: start=0x%08x, %d", p_cfg->start, p_flash->erase_unit);
        return ERR_FLASH_INVALID_ARG;
    }
    if (! IS_ERASE_ADDR(p_cfg->end, p_flash->erase_unit)) {
        print_error("error: end=0x%08x, %d", p_cfg->end, p_flash->erase_unit);
        return ERR_FLASH_INVALID_ARG;
    }
    // 至少两个擦除单位
    if (p_cfg->start + p_flash->erase_unit * 2 > p_cfg->end) {
        print_error("error: end=%08x, start=%08x", p_cfg->end, p_cfg->start);
        return ERR_FLASH_INVALID_ARG;
    }
    return ERR_FLASH_SUCCESS;
}

// @brief 页头校验值
static uint16_t flashlog_head_hash(const flashlog_page_head_t* p_head)
{
    uint32_t buff_len = OFFSETOF(flashlog_page_head_t, crc);
    const uint8_t* p_buff = (const uint8_t*)p_head;
    uint32_t hash = 0;
    while (buff_len--) {
        hash = (*p_buff++) + (hash << 6) + (hash << 16) - hash;
    }
    return (hash & 0xffff);
}

// @brief 读取并校验页头
static bool flashlog_head_read(flashlog_t* p_flog, uint32_t addr, flashlog_page_head_t* p_head)
{
    htd_memset(p_head, 0, sizeof(flashlog_page_head_t));
    htd_flash_read(p_flog->p_flash, addr, p_head, sizeof(flashlog_page_head_t));
    if (p_head->magic != m_flashlog_default_magic) {
        return false;
    }
    if (p_head->crc != flashlog_head_hash(p_head)) {
        return false;
    }
    if (p_head->pidx > m_flashlog_pidx_max) {
        return false;
    }
    return true;
}

// @brief 写入头部数据
static void flashlog_head_write(flashlog_t* p_flog, uint32_t addr, uint32_t index)
{
    flashlog_page_head_t hdr;
    hdr.magic = m_flashlog_default_magic;
    hdr.pidx = index;
    hdr.crc = flashlog_head_hash(&hdr);
    ASSERT(htd_flash_is_erase_addr(p_flog->p_flash, addr));
    htd_flash_write(p_flog->p_flash, addr, &hdr, sizeof(flashlog_page_head_t));
    
    if (htd_flash_erase_addr(p_flog->p_flash, p_flog->rpos) == htd_flash_erase_addr(p_flog->p_flash, addr)) {
        p_flog->rpos = htd_flash_next_addr_get(p_flog->p_cfg, addr, p_flog->p_flash->erase_unit);
        p_flog->rpos += sizeof(flashlog_page_head_t);
    }
    p_flog->pidx = index;
}

// @brief 写入无效页面标志
static void flashlog_head_write_invalid(flashlog_t* p_flog, uint32_t addr)
{
    flashlog_page_head_t hdr;
    htd_memset(&hdr, 0, sizeof(flashlog_page_head_t));
    htd_flash_write(p_flog->p_flash, addr, &hdr, sizeof(flashlog_page_head_t));
}


// @brief 调整写的位置, 从后往前查看
static uint32_t flashlog_write_pos_adjust(const flash_handle_t * p_flash, uint32_t start)
{
    uint8_t buff[64];
    uint32_t read_len;

    uint32_t end = htd_flash_next_page_addr(p_flash, start);
    while (end > start) {
        read_len = end - start;
        if (read_len > sizeof(buff)) {
            read_len = sizeof(buff);
        } 
        end -= read_len;
        
        htd_memset(buff, 0, read_len);
        htd_flash_read(p_flash, end, buff, read_len);
        while (buff[read_len-1] == 0xff) {        // 不是 0xff 说明有数据了
            if (read_len == 1) {
                read_len = 0;
                break;
            }
            read_len--;
        }
        if (read_len) {
            print_warning("end=%08x, read_len=%d, %08x", end, read_len, end+read_len);
            return end + read_len;
        }
    } 
    return end;
}

// @brief 写入数据按4字节对齐
static void flashlog_write_align_4byte(const flash_handle_t* p_flash, uint32_t addr, const uint8_t* p_buff, uint32_t buff_len)
{
    uint32_t front, len;
    uint8_t cache[4];

    ASSERT(htd_flash_erase_addr(p_flash, addr) == htd_flash_erase_addr(p_flash, addr+buff_len-1));
    front = addr & 0x03;
    if (front) {
        len = sizeof(cache) - front;
        if (len > buff_len) {
            len = buff_len;
        }
        //print_debug("front=%d, len=%d, addr=%08x, buff_len=%d", front, len, addr, buff_len);
        addr = addr - front;
        *(uint32_t*)cache = 0xffffffff;
        htd_flash_read(p_flash, addr, cache, sizeof(cache));
        htd_memcpy(&cache[front], p_buff, len);
        htd_flash_write(p_flash, addr, cache, sizeof(cache));
        buff_len -= len;
        if (buff_len == 0) {
            return ;
        }
        addr += sizeof(cache);
        p_buff += len; 
    }

    // 写入中间部分
    len = buff_len & 0x03;
    buff_len -= len;
    if (buff_len) {
        htd_flash_write(p_flash, addr, p_buff, buff_len);
    }

    // 写入后面部分
    if (len) {
        p_buff += buff_len;
        addr += buff_len;
        *(uint32_t*)cache = 0xffffffff;
        htd_memcpy(cache, p_buff, len);
        htd_flash_write(p_flash, addr, cache, sizeof(cache));
    }
}

// @brief 读数据，按4个字节对齐
static void flashlog_read_align_4byte(const flash_handle_t* p_flash, uint32_t addr, uint8_t* p_buff, uint32_t buff_len)
{
    uint32_t len;
    uint8_t cache[4];
    
    // 先读前面的数据
    uint32_t front = addr & 0x03;
    if (front) {
        len = 4 - front;
        if (len > buff_len) {
            len = buff_len;
        }
        addr = addr - front;
        *(uint32_t*)cache = 0xffffffff;
        htd_flash_read(p_flash, addr, cache, 4);
        htd_memcpy(p_buff, &cache[front], len);
        buff_len -= len;
        if (buff_len == 0) {
            return ;
        }
        addr += 4;
        p_buff += len; 
    }

    // 读中间部分
    front = buff_len - (buff_len & 0x03);
    htd_flash_read(p_flash, addr, p_buff, front);
    buff_len -= front;

    // 读后面剩下的
    if (buff_len) {
        addr += front;
        p_buff += front;
        htd_flash_read(p_flash, addr, cache, sizeof(cache));
        htd_memcpy(p_buff, cache, buff_len);
    }
}

// @brief 写入flash (buff_len <= erase_unit, 最多跨一页; 返回本次额外占用的页头长度)
static uint32_t flashlog_write_flash(flashlog_t* p_flog, uint32_t addr, const uint8_t* p_buff, uint32_t buff_len)
{
    if (buff_len == 0) {
        return 0;
    }

    // 页首: 先擦页写头, 数据从页头之后开始
    uint32_t spread = 0;
    const flash_handle_t* p_flash = p_flog->p_flash;
    if (htd_flash_is_erase_addr(p_flash, addr)) {
        htd_flash_erase(p_flash, addr);
        flashlog_head_write(p_flog, addr, p_flog->pidx + 1);
        addr += sizeof(flashlog_page_head_t);
        spread += sizeof(flashlog_page_head_t);
    }

    uint32_t page_left = htd_flash_page_remain_len(p_flash, addr);
    if (buff_len <= page_left) {
        // 本页内写完
        flashlog_write_align_4byte(p_flash, addr, p_buff, buff_len);
        addr += buff_len;
        if (addr >= p_flog->p_cfg->end) {
            addr = p_flog->p_cfg->start;
        }
        if (htd_flash_is_erase_addr(p_flash, addr)) {
            // 正好写到页尾: 提前擦好下一页并写页头, 下次写入从页头之后开始
            htd_flash_erase(p_flash, addr);
            flashlog_head_write(p_flog, addr, p_flog->pidx + 1);
            spread += sizeof(flashlog_page_head_t);
        }
        return spread;
    }

    // 跨页: 先把当前页剩下的数据写完, 再擦新页写页头, 最后写剩余数据
    // 顺序不能反, 先擦页写头的话掉电会在当前页尾部留下没写过的空白, 重启后会被当成有效数据读出来
    flashlog_write_align_4byte(p_flash, addr, p_buff, page_left);
    addr += page_left;
    if (addr >= p_flog->p_cfg->end) {
        addr = p_flog->p_cfg->start;
    }
    p_buff += page_left;
    buff_len -= page_left;

    htd_flash_erase(p_flash, addr);
    flashlog_head_write(p_flog, addr, p_flog->pidx + 1);
    addr += sizeof(flashlog_page_head_t);
    flashlog_write_align_4byte(p_flash, addr, p_buff, buff_len);
    return spread + sizeof(flashlog_page_head_t);
}

// @brief 读取flash数据
static void flashlog_read_flash(flashlog_t* p_flog, uint32_t addr, uint8_t* p_buff, uint32_t buff_len)
{
    uint32_t len;
    while (buff_len) {
        len = htd_flash_next_page_addr(p_flog->p_flash, addr) - addr;
        if (len > buff_len) {
            len = buff_len;
        }
        
        flashlog_read_align_4byte(p_flog->p_flash, addr, p_buff, len);
        p_buff += len;
        addr += len;
        if (addr >= p_flog->p_cfg->end) {
            addr = p_flog->p_cfg->start;
        }
        addr += sizeof(flashlog_page_head_t);
        buff_len -= len;
    }
}


//////////////////////////////////////////////////////////////////////
// @brief 初始化
int32_t htd_flashlog_init(flashlog_t* p_flog, const flash_config_t* p_cfg)
{
    ASSERT(p_flog);
    p_flog->p_flash = htd_flash_handle_find(p_cfg->p_name);
    if (p_flog->p_flash == NULL) {
        return ERR_FLASH_NOT_FOUND;
    }
    int32_t ret = flashlog_param_assert(p_cfg, p_flog->p_flash); 
    if (ret != ERR_FLASH_SUCCESS) {
        return ret;
    }
    p_flog->p_cfg = p_cfg;
    p_flog->rpos = p_flog->wpos = 0xffffffff;

    // 扫描全部页收集有效页头, 再按页序号挑出最新/最旧页
    uint32_t newest_addr = 0xffffffff;
    uint32_t newest_pidx = 0;
    uint32_t oldest_addr = 0xffffffff;
    uint32_t oldest_pidx = 0xffffffff;
    flashlog_page_head_t hdr;
    uint32_t addr = p_cfg->start;
    while (addr < p_cfg->end) {
        if (flashlog_head_read(p_flog, addr, &hdr)) {
            if (hdr.pidx >= newest_pidx) {
                newest_pidx = hdr.pidx;
                newest_addr = addr;
            }
            if (hdr.pidx < oldest_pidx) {
                oldest_pidx = hdr.pidx;
                oldest_addr = addr;
            }
        }
        addr += p_flog->p_flash->erase_unit;
    }

    if (newest_addr == 0xffffffff) {
        // 首次初始化
        p_flog->pidx = 1;
        p_flog->wpos = p_cfg->start + sizeof(flashlog_page_head_t);
        htd_flash_erase(p_flog->p_flash, p_cfg->start);
        flashlog_head_write(p_flog, p_cfg->start, p_flog->pidx);
        p_flog->rpos = p_cfg->start + sizeof(flashlog_page_head_t);
        htd_flash_sync(p_flog->p_flash);
    } else {
        p_flog->pidx = newest_pidx;
        p_flog->rpos = oldest_addr + sizeof(flashlog_page_head_t);
        p_flog->wpos = newest_addr + sizeof(flashlog_page_head_t);
        // 调整写位置
        print_debug("==111==wpos=%08x, rpos=%08x, pidx=%u", p_flog->wpos, p_flog->rpos, p_flog->pidx);
        p_flog->wpos = flashlog_write_pos_adjust(p_flog->p_flash, p_flog->wpos);
        if (p_flog->wpos >= p_cfg->end) {
            p_flog->wpos = p_cfg->start;
        }
        print_debug("==222==wpos=%08x, rpos=%08x, pidx=%d", p_flog->wpos, p_flog->rpos, p_flog->pidx);
    }
    p_flog->init_flag = m_flashlog_init_flag;
    return ret;
}

// @brief 去初始化
int32_t htd_flashlog_uninit(flashlog_t* p_flog)
{
    if (p_flog->init_flag != m_flashlog_init_flag) {
        return 0;
    }
    p_flog->init_flag = 0;
    return 0;
}

// @brief 获取有效长度
static uint32_t flashlog_data_len(flashlog_t* p_flog)
{
    ASSERT(p_flog);
    if (p_flog->init_flag != m_flashlog_init_flag) {
        return 0;
    }

    uint32_t range = p_flog->p_cfg->end - p_flog->p_cfg->start;
    uint32_t page_count = range / p_flog->p_flash->erase_unit;
    uint32_t raddr = p_flog->rpos - p_flog->p_cfg->start;
    uint32_t waddr = p_flog->wpos - p_flog->p_cfg->start;   // 写指针可能正好等于 end
    uint32_t valid_len = (waddr >= raddr) ? (waddr - raddr) : (range + waddr - raddr);
    if (valid_len == 0) {
        return 0;
    }

    // 占用多少页，waddr/raddr 重定义为写读的页序号
    uint32_t pages;
    waddr = ((waddr + range - 1) % range) / p_flog->p_flash->erase_unit;
    raddr = raddr / p_flog->p_flash->erase_unit;
    pages = (waddr + page_count - raddr) % page_count;
    valid_len -= pages * sizeof(flashlog_page_head_t);
    return valid_len;
}

// @brief 清除所有数据
void htd_flashlog_clear(flashlog_t* p_flog)
{
    ASSERT(p_flog);
    if (p_flog->init_flag != m_flashlog_init_flag) {
        return ;
    }
    
    HTD_FLASH_MUTEX_LOCK(p_flog->p_cfg);
    p_flog->rpos = 0xffffffff;
    p_flog->wpos = p_flog->p_cfg->start + sizeof(flashlog_page_head_t);
    p_flog->pidx = 1;
    // 所有页头写入无效
    uint32_t addr = p_flog->p_cfg->start;
    while (1) {
        flashlog_head_write_invalid(p_flog, addr);
        addr += p_flog->p_flash->erase_unit;
        if (addr >= p_flog->p_cfg->end) {
            break;
        }
    }

    // 第一页重新写入头部信息
    addr = p_flog->p_cfg->start;
    htd_flash_erase(p_flog->p_flash, addr);
    flashlog_head_write(p_flog, addr, p_flog->pidx);
    p_flog->rpos = p_flog->wpos;
    htd_flash_sync(p_flog->p_flash);
    HTD_FLASH_MUTEX_UNLOCK(p_flog->p_cfg);
}

// @brief 写入数据
int32_t htd_flashlog_write(flashlog_t* p_flog, const uint8_t* p_buff, uint32_t buff_len)
{
    ASSERT(p_flog);
    ASSERT(p_buff);
    if (buff_len == 0 || p_flog->init_flag != m_flashlog_init_flag) {
        return ERR_FLASH_INVALID_ARG;
    }
    
    uint32_t spread_page;
    uint32_t write_len = p_flog->p_flash->erase_unit - sizeof(flashlog_page_head_t);
    print_debug("start write: wpos=%08x, rpos=%08x, buff_len=%d, end=%08x", p_flog->wpos, p_flog->rpos, buff_len, p_flog->p_cfg->end);
    HTD_FLASH_MUTEX_LOCK(p_flog->p_cfg);
    while (buff_len) {
        if (write_len > buff_len) {
            write_len = buff_len;
        }
        // flashlog_write_flash 最多只能写入一页
        spread_page = flashlog_write_flash(p_flog, p_flog->wpos, p_buff, write_len);
        p_flog->wpos += write_len + spread_page;
        if (p_flog->wpos >= p_flog->p_cfg->end) {
            p_flog->wpos = p_flog->p_cfg->start + p_flog->wpos - p_flog->p_cfg->end;
        }
        p_buff += write_len;
        buff_len -= write_len;
    }
    
    htd_flash_sync(p_flog->p_flash);
    HTD_FLASH_MUTEX_UNLOCK(p_flog->p_cfg);
    print_debug("write finish: wpos=%08x, rpos=%08x", p_flog->wpos, p_flog->rpos);
    return ERR_FLASH_SUCCESS;
}

// @brief 逻辑偏移换算成 flash 地址(每页数据前面有一个页头, 要跳过)
static uint32_t flashlog_read_addr_get(flashlog_t* p_flog, uint32_t offset)
{
    uint32_t addr = p_flog->rpos;
    uint32_t page_left = htd_flash_next_page_addr(p_flog->p_flash, addr) - addr;
    while (offset >= page_left) {
        offset -= page_left;
        addr = htd_flash_next_page_addr(p_flog->p_flash, addr);
        if (addr >= p_flog->p_cfg->end) {
            addr = p_flog->p_cfg->start;
        }
        addr += sizeof(flashlog_page_head_t);
        page_left = p_flog->p_flash->erase_unit - sizeof(flashlog_page_head_t);
    }
    return addr + offset;
}

// @brief 读取数据, 成功返回读取到的数据长度，失败返回错误码(小于0)
int32_t htd_flashlog_read(flashlog_t* p_flog, uint32_t offset, uint8_t* p_buff, uint32_t buff_len)
{
    ASSERT(p_flog);
    ASSERT(p_buff);
    if (buff_len == 0 || p_flog->init_flag != m_flashlog_init_flag) {
        return ERR_FLASH_INVALID_ARG;
    }
    
    // 偏移大于有效长度
    uint32_t valid_len;
    HTD_FLASH_MUTEX_LOCK(p_flog->p_cfg);
    valid_len = flashlog_data_len(p_flog);
    if (offset >= valid_len) {
        HTD_FLASH_MUTEX_UNLOCK(p_flog->p_cfg);
        print_error("offset=%d, valid_len=%d", offset, valid_len);
        return ERR_FLASH_FAILED;
    }
    // 计算读取的长度
    valid_len -= offset;
    if (buff_len > valid_len) {
        buff_len = valid_len;
    }
    // 计算读取位置
    uint32_t raddr = flashlog_read_addr_get(p_flog, offset);
    flashlog_read_flash(p_flog, raddr, p_buff, buff_len);
    HTD_FLASH_MUTEX_UNLOCK(p_flog->p_cfg);
    return (int32_t)buff_len; 
}

// @brief 获取有效长度
uint32_t htd_flashlog_data_len(flashlog_t* p_flog)
{
    ASSERT(p_flog);
    ASSERT(p_flog->init_flag == m_flashlog_init_flag);

    uint32_t valid_len;
    HTD_FLASH_MUTEX_LOCK(p_flog->p_cfg);
    valid_len = flashlog_data_len(p_flog);
    HTD_FLASH_MUTEX_UNLOCK(p_flog->p_cfg);
    return valid_len;
}

// @brief 增加日期前缀
static uint32_t add_calendar_prefix(char* p_data)
{
#if 0
    htd_time_t cdr = htd_rtc_time_get();
    return htd_sprintf(p_data, "[%02d/%02d %02d:%02d:%02d]", cdr.month, cdr.day, cdr.hour, cdr.min, cdr.second);
#else
    return 0;
#endif
}

// @brief 打印接口
int32_t htd_flashlog_line_print(flashlog_t* p_flog, const char *fmt, ...)
{
    char data[256+64];
    int32_t data_offset;

    data_offset = add_calendar_prefix(data);
    va_list args;
    va_start(args, fmt);
    data_offset += htd_vsnprintf(&data[data_offset], sizeof(data)-data_offset, fmt, args);
    va_end(args);
    // vsnprintf 返回的是"想要写的长度", 超长时会超出缓冲区, 这里截断
    if (data_offset > (int32_t)sizeof(data) - 1) {
        data_offset = (int32_t)sizeof(data) - 1;
    }
    if (data_offset > 0) {
        htd_flashlog_write(p_flog, (uint8_t*)data, data_offset);
        print_debug("rpos=%08x, wpos=%08x, valid_len=%d", p_flog->rpos, p_flog->wpos, htd_flashlog_data_len(p_flog));
    }
    return data_offset;
}

#endif /*defined(HTD_FLASH_LOG_ENABLE)*/

