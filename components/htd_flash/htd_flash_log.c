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
    uint32_t magic;
    uint32_t pidx;          // 页序号标志 page index
} flashlog_page_head_t;
const static uint32_t m_flashlog_default_magic = 0x4a5af3f3;
const static uint16_t m_flashlog_init_flag = 0x1155;


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

// @brief 写入头部数据
static void flashlog_head_write(flashlog_t* p_flog, uint32_t addr, uint32_t index)
{
    flashlog_page_head_t hdr;
    hdr.magic = m_flashlog_default_magic;
    hdr.pidx = index;
    ASSERT(htd_flash_is_erase_addr(p_flog->p_flash, addr));
    htd_flash_write(p_flog->p_flash, addr, &hdr, sizeof(flashlog_page_head_t));
    p_flog->pidx = index;
    if (p_flog->rpos == 0xffffffff) {
        return;
    }
    if (htd_flash_erase_addr(p_flog->p_flash, p_flog->rpos) == htd_flash_erase_addr(p_flog->p_flash, addr)) {
        p_flog->rpos = htd_flash_next_addr_get(p_flog->p_cfg, addr, p_flog->p_flash->erase_unit);
        p_flog->rpos += sizeof(flashlog_page_head_t);
    }
}

// @brief 调整写的位置, 从后往前查看
static uint32_t flashlog_write_pos_adjust(const flash_handle_t * p_flash, uint32_t start)
{
    uint8_t buff[64];
    uint32_t end;
    uint32_t read_len;

    end = htd_flash_next_page_addr(p_flash, start);
    do {
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
    } while (end > start);
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
    uint32_t front, len;
    uint8_t cache[4];
    
    front = addr & 0x03;
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

    front = buff_len - (buff_len & 0x03);
    htd_flash_read(p_flash, addr, p_buff, front);
    buff_len -= front;

    if (buff_len) {
        addr += front;
        p_buff += front;
        htd_flash_read(p_flash, addr, cache, sizeof(cache));
        htd_memcpy(p_buff, cache, buff_len);
    }
}

// @brief 写入flash
static uint32_t flashlog_write_flash(flashlog_t* p_flog, uint32_t addr, const uint8_t* p_buff, uint32_t buff_len)
{
    uint32_t front;
    uint32_t erase_addr;

    if (buff_len == 0) {
        return 0;
    }

    const flash_handle_t* p_flash = p_flog->p_flash;
    if (htd_flash_is_erase_addr(p_flash, addr)) {
        htd_flash_erase(p_flash, addr);
        // 写入头部信息
        flashlog_head_write(p_flog, addr, p_flog->pidx + 1);
        addr += sizeof(flashlog_page_head_t);
        // 写入数据
        flashlog_write_align_4byte(p_flash, addr, p_buff, buff_len);
    } else {
        erase_addr = htd_flash_next_addr_get(p_flog->p_cfg, addr, buff_len - 1);
        erase_addr = htd_flash_erase_addr(p_flash, erase_addr);
        if (erase_addr == htd_flash_erase_addr(p_flash, addr)) {
            flashlog_write_align_4byte(p_flash, addr, p_buff, buff_len);
            if (htd_flash_is_erase_addr(p_flash, addr + buff_len)) {
                erase_addr = htd_flash_next_addr_get(p_flog->p_cfg, addr, buff_len);
                buff_len = 0;
            } else {
                return 0;
            }
        }
        // 擦除数据
        htd_flash_erase(p_flash, erase_addr);
        // 写入头部信息
        flashlog_head_write(p_flog, erase_addr, p_flog->pidx + 1);
        if (buff_len) {
            front = erase_addr - addr;
            // 写入前半段数据
            flashlog_write_align_4byte(p_flash, addr, p_buff, front);
            p_buff += front;
            // 写入后半段数据
            flashlog_write_align_4byte(p_flash, erase_addr + sizeof(flashlog_page_head_t), p_buff, buff_len - front);
        }
    }
    return sizeof(flashlog_page_head_t);
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
    int32_t ret;
    uint32_t addr;
    uint32_t prev_pidx;
    
    ASSERT(p_flog);
    p_flog->p_flash = htd_flash_handle_find(p_cfg->p_name);
    if (p_flog->p_flash == NULL) {
        return ERR_FLASH_NOT_FOUND;
    }
    
    ret = flashlog_param_assert(p_cfg, p_flog->p_flash); 
    if (ret != ERR_FLASH_SUCCESS) {
        return ret;
    }
    p_flog->p_cfg = p_cfg;

    // 查找最后一页
    flashlog_page_head_t hdr;
    p_flog->rpos = p_flog->wpos = 0xffffffff;
    addr = p_cfg->start;
    prev_pidx = 1;
    while (addr < p_cfg->end) {
        htd_memset(&hdr, 0, sizeof(flashlog_page_head_t));
        htd_flash_read(p_flog->p_flash, addr, &hdr, sizeof(flashlog_page_head_t));
        if (hdr.magic != m_flashlog_default_magic) {
            break;
        }
        if (hdr.pidx < prev_pidx) {
            p_flog->rpos = addr + sizeof(flashlog_page_head_t);
            break;
        }
        if (p_flog->rpos == 0xffffffff) {
            p_flog->rpos = addr + sizeof(flashlog_page_head_t);
        }
        p_flog->wpos = addr + sizeof(flashlog_page_head_t);
        addr += p_flog->p_flash->erase_unit;
        prev_pidx = hdr.pidx;
    }
    p_flog->pidx = prev_pidx;
    
    print_debug("==111==wpos=%08x, pidx=%u", p_flog->wpos, p_flog->pidx);
    if (p_flog->wpos == 0xffffffff) {
        // 首次初始化
        p_flog->rpos = p_flog->wpos;
        p_flog->wpos = p_cfg->start + sizeof(flashlog_page_head_t);
        htd_flash_erase(p_flog->p_flash, p_cfg->start);
        flashlog_head_write(p_flog, p_cfg->start, p_flog->pidx);
        p_flog->rpos = p_cfg->start + sizeof(flashlog_page_head_t);
        htd_flash_sync(p_flog->p_flash);
    } else {
        // 调整写位置
        p_flog->wpos = flashlog_write_pos_adjust(p_flog->p_flash, p_flog->wpos);
    }
    print_debug("==222==wpos=%08x, rpos=%08x, pidx=%d", p_flog->wpos, p_flog->rpos, p_flog->pidx);
    p_flog->init_flag = m_flashlog_init_flag;
    p_flog->busy_flag = 0;
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
    uint32_t valid_len;
    uint32_t offset = 0;
    uint32_t raddr, waddr;

    ASSERT(p_flog);
    if (p_flog->init_flag != m_flashlog_init_flag) {
        return 0;
    }

    raddr = htd_flash_erase_addr(p_flog->p_flash, p_flog->rpos);
    waddr = htd_flash_erase_addr(p_flog->p_flash, p_flog->wpos);
    if (p_flog->rpos > p_flog->wpos) {
        offset = (p_flog->p_cfg->end - p_flog->p_cfg->start  + waddr - raddr) / p_flog->p_flash->erase_unit;
        valid_len = p_flog->p_cfg->end - p_flog->p_cfg->start + p_flog->wpos - p_flog->rpos;
    } else {
        offset = ((waddr - raddr) / p_flog->p_flash->erase_unit);
        valid_len = p_flog->wpos - p_flog->rpos;
    }
    offset = offset * sizeof(flashlog_page_head_t);
    if (valid_len >= offset) { 
        valid_len -= offset;
    } else {
        valid_len = 0;
    }
    return valid_len;
}

// @brief 清除所有数据
void htd_flashlog_clear(flashlog_t* p_flog)
{
    uint32_t addr;
    uint32_t val = 0;
    
    ASSERT(p_flog);
    if (p_flog->init_flag != m_flashlog_init_flag) {
        return ;
    }
    
    HTD_FLASH_MUTEX_LOCK(p_flog->p_cfg);
    addr = p_flog->p_cfg->start;
    p_flog->rpos = 0xffffffff;
    p_flog->wpos = p_flog->p_cfg->start + sizeof(flashlog_page_head_t);
    p_flog->pidx = 1;
    // 第一页重新写入头部信息
    htd_flash_erase(p_flog->p_flash, addr);
    flashlog_head_write(p_flog, addr, p_flog->pidx);
    p_flog->rpos = p_flog->wpos;
    // 清除后面页面
    while (1) {
        addr += p_flog->p_flash->erase_unit;
        if (addr >= p_flog->p_cfg->end) {
            break;
        }
        htd_flash_write(p_flog->p_flash, addr, &val, sizeof(val));
    }
    htd_flash_sync(p_flog->p_flash);
    HTD_FLASH_MUTEX_UNLOCK(p_flog->p_cfg);
}

// @brief 写入数据
int32_t htd_flashlog_write(flashlog_t* p_flog, const uint8_t* p_buff, uint32_t buff_len)
{
    uint32_t spread_page = 0;
    
    ASSERT(p_flog);
    ASSERT(p_buff);
    ASSERT(buff_len);
    if (p_flog->init_flag != m_flashlog_init_flag) {
        return ERR_FLASH_INVALID_ARG;
    }

    const flash_handle_t* p_flash = p_flog->p_flash;
    if (buff_len > p_flash->erase_unit) {
        print_error("invalid paramter. buff_len=%d, erase_unit=%d", buff_len, p_flash->erase_unit);
        return ERR_FLASH_INVALID_ARG;
    }
    
    print_debug("start write: wpos=%08x, rpos=%08x, buff_len=%d, end=%08x", p_flog->wpos, p_flog->rpos, buff_len, p_flog->p_cfg->end);
    // 写入数据，偏移写位置
    HTD_FLASH_MUTEX_LOCK(p_flog->p_cfg);
    if (p_flog->wpos + buff_len <= p_flog->p_cfg->end) {
        spread_page = flashlog_write_flash(p_flog, p_flog->wpos, p_buff, buff_len);
        p_flog->wpos += buff_len + spread_page;
        if (p_flog->wpos >= p_flog->p_cfg->end) {
            p_flog->wpos = p_flog->p_cfg->start + p_flog->wpos - p_flog->p_cfg->end;
        }
    } else {
        uint32_t len = p_flog->p_cfg->end - p_flog->wpos;
        flashlog_write_flash(p_flog, p_flog->wpos, p_buff, len);
        buff_len -= len;
        spread_page = flashlog_write_flash(p_flog, p_flog->p_cfg->start, p_buff + len, buff_len);
        p_flog->wpos = p_flog->p_cfg->start + buff_len + spread_page;
    }
    htd_flash_sync(p_flash);
    HTD_FLASH_MUTEX_UNLOCK(p_flog->p_cfg);
    print_debug("write finish: wpos=%08x, rpos=%08x", p_flog->wpos, p_flog->rpos);
    return ERR_FLASH_SUCCESS;
}

// @brief 读取数据
int32_t htd_flashlog_read(flashlog_t* p_flog, uint32_t offset, uint8_t* p_buff, uint32_t* buff_len)
{
    uint32_t rlen;
    uint32_t start;
    uint32_t valid_len;

    ASSERT(p_flog);
    ASSERT(p_buff);
    if (p_flog->init_flag != m_flashlog_init_flag) {
        return ERR_FLASH_INVALID_ARG;
    }
    
    rlen = *buff_len;
    if (rlen == 0) {
        print_error("invalid paramter. buff_len=%d", rlen);
        return ERR_FLASH_INVALID_ARG;
    }
    
    // 偏移大于有效长度
    HTD_FLASH_MUTEX_LOCK(p_flog->p_cfg);
    valid_len = flashlog_data_len(p_flog);
    if (offset >= valid_len) {
        HTD_FLASH_MUTEX_UNLOCK(p_flog->p_cfg);
        print_error("offset=%d, valid_len=%d", offset, valid_len);
        return ERR_FLASH_FAILED;
    }
    // 计算读取的长度
    valid_len -= offset;
    if (rlen > valid_len) {
        rlen = valid_len;
    }
    // 计算读取位置
    start = htd_flash_next_addr_get(p_flog->p_cfg, p_flog->rpos, offset);
    flashlog_read_flash(p_flog, start, p_buff, rlen);
    HTD_FLASH_MUTEX_UNLOCK(p_flog->p_cfg);
    *buff_len = rlen;
    return ERR_FLASH_SUCCESS;
}

// @brief 获取有效长度
uint32_t htd_flashlog_data_len(flashlog_t* p_flog)
{
    uint32_t valid_len;
    
    ASSERT(p_flog);
    if (p_flog->init_flag != m_flashlog_init_flag) {
        return ERR_FLASH_INVALID_ARG;
    }
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
    return sprintf(p_data, "[%02d/%02d %02d:%02d:%02d]", cdr.month, cdr.day, cdr.hour, cdr.min, cdr.second);
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
    
    htd_flashlog_write(p_flog, (uint8_t*)data, data_offset);
    print_debug("rpos=%08x, wpos=%08x, valid_len=%d", p_flog->rpos, p_flog->wpos, htd_flashlog_data_len(p_flog));
    return data_offset;
}









/////////////////////////////////////////////////////////////////////////////
#if (0 && defined(DEBUG_LVL) && (DEBUG_LVL >= 4) && defined(HTD_CONSOLE_ENABLE) && defined(HTD_USING_NORFLASH))
#include "htd_dev_norflash.h"
#include "htd_console_process.h"

#define FLASH_TEST_LEN      4096*4
static flashlog_t m_flashlog_object;
const static flash_config_t m_flashlog_config = {
    .p_name = "norflash_4k",
    .start = NORFLASH_MAP_START_ADDR,
    .end = NORFLASH_MAP_START_ADDR + FLASH_TEST_LEN,
    .lock_handle = NULL,
    .unlock_handle = NULL,
};

typedef struct {
    uint32_t rpos;
    uint32_t wpos;
    uint32_t mask;
    uint8_t* p_buff;
}fifo_buff_t;
static fifo_buff_t flash_fifo;
static uint8_t flash_fifo_buff[FLASH_TEST_LEN*2];

// @brief 往队列添加数据
static void fifo_data_init(fifo_buff_t* p_fifo, uint8_t *p_buff, uint32_t buff_size)
{
    ASSERT(p_buff);
    ASSERT(IS_POWER_OF_TWO(buff_size) && buff_size > 4096);
    p_fifo->p_buff = p_buff;
    p_fifo->mask = buff_size - 1;
    p_fifo->rpos = p_fifo->wpos = 0;
}
// @brief 获取 fifo 数据大小
static uint32_t fifo_data_length(fifo_buff_t* p_fifo)
{
    //print_debug("wpos=%d, rpos=%d", p_fifo->wpos, p_fifo->rpos);
    return ((p_fifo->wpos - p_fifo->rpos) & 0xffffffff);
}
// @brief fifo 剩余空间大小
static uint32_t fifo_data_remained(fifo_buff_t* p_fifo)
{
    return (p_fifo->mask - fifo_data_length(p_fifo));
}
// @brief 添加数据
static void fifo_data_put(fifo_buff_t* p_fifo, const uint8_t* p_data, uint32_t data_len)
{
    uint32_t front;
    uint32_t data_size = p_fifo->mask + 1;
    uint32_t pos = p_fifo->wpos & p_fifo->mask;
    uint32_t remain_len = fifo_data_remained(p_fifo);
    if (remain_len < data_len) {
        data_len = remain_len;
    }
    if (pos + data_len > data_size) {
        front = data_size - pos;
        htd_memcpy(&p_fifo->p_buff[pos], p_data, front);
        p_data += front;
        htd_memcpy(&p_fifo->p_buff[0], p_data, data_len - front);
    } else {
        htd_memcpy(&p_fifo->p_buff[pos], p_data, data_len);
    }
    p_fifo->wpos += data_len;

    remain_len = fifo_data_remained(p_fifo);
    if (remain_len < 4096) {
        p_fifo->rpos += 4096;
    }
}
// @brief 往队列提取数据
static uint32_t fifo_data_get(fifo_buff_t* p_fifo, uint8_t* p_data, uint32_t data_len)
{
    uint32_t front;
    uint32_t data_size = p_fifo->mask + 1;
    uint32_t pos = p_fifo->rpos & p_fifo->mask;
    uint32_t length = fifo_data_length(p_fifo);
    if (length < data_len) {
        data_len = length;
    }
    if (pos + data_len > data_size) {
        front = data_size - pos;
        htd_memcpy(p_data, &p_fifo->p_buff[pos], front);
        p_data += front;
        htd_memcpy(p_data, &p_fifo->p_buff[0], data_len - front);
    } else {
        htd_memcpy(p_data, &p_fifo->p_buff[pos], data_len);
    }
    //p_fifo->rpos += data_len;
    return data_len;
}
// @brief 删除队列多少个字节
static void fifo_data_pop(fifo_buff_t* p_fifo, uint32_t data_len)
{
    p_fifo->rpos += data_len;
}

// @brief 比较写入的数据
bool compare_write_data(void)
{
    bool ret = false;
    uint32_t buff_len;
    uint8_t* p_data;

    uint32_t data_len = htd_flashlog_data_len(&m_flashlog_object);
    if (data_len == 0) {
        print_error("compare failed. rpos=%08x, wpos=%08x", m_flashlog_object.rpos, m_flashlog_object.wpos);
        return ret;
    }

    p_data = htd_malloc(data_len);
    ASSERT(p_data);
    htd_memset(p_data, 0, data_len);
    buff_len = data_len;
    htd_flashlog_read(&m_flashlog_object, 0, p_data, &data_len);
    if (buff_len != data_len) {
        htd_free(p_data);
        print_error("rngbuf read failed. buff_len=%d, data_len=%d", buff_len, data_len);
        return ret;
    }
    
    buff_len = fifo_data_length(&flash_fifo);
    if (buff_len > data_len) {
        buff_len -= data_len;
        fifo_data_pop(&flash_fifo, buff_len);
    }

    uint8_t* p_buff = htd_malloc(data_len);
    ASSERT(p_buff);
    htd_memset(p_buff, 0, data_len);
    buff_len = fifo_data_get(&flash_fifo, p_buff, data_len);
    if (buff_len != data_len) {
        print_error("rngbuff test failed. data_len=%d, buff_len=%d", data_len, buff_len);
    } else {
        if (htd_memcmp(p_buff, p_data, data_len) == 0) {
            ret = true;
        } else {
            print_error("rngbuff test failed. data_len=%d, wpos=%08x, rpos=%08x", data_len, flash_fifo.wpos, flash_fifo.rpos);
            print_hex("data", p_data, data_len);
            print_hex("buff", p_buff, data_len);
        }
    }

    htd_free(p_buff);
    htd_free(p_data);
    return ret;
}

int flashlog_test_cmd(void)
{
    srand(3);
    htd_flashlog_init(&m_flashlog_object, &m_flashlog_config);
    htd_flashlog_clear(&m_flashlog_object);
    fifo_data_init(&flash_fifo, flash_fifo_buff, sizeof(flash_fifo_buff));

    uint8_t buff[4096];
    uint32_t buff_len, i;
    uint8_t byte = 0;
    for (i=0; i<1000000; ++i) {
        // 写入数据进行比较
        buff_len = 512 + htd_rng_val_get() % 512; 
        for (uint32_t j=0; j<buff_len; ++j) {
            buff[j] = byte++;
            if (byte == 0xff) {
                byte = 0;
            }
        }
        
        htd_flashlog_write(&m_flashlog_object, buff, buff_len);
        fifo_data_put(&flash_fifo, buff, buff_len);
        if (compare_write_data() == false) {
            print_error("---111---");
            break;
        }

    #if 1
        // 重新初始化再进行比较
        //print_debug("before: wpos=%d, rpos=%d", m_flashlog_object.wpos, m_flashlog_object.rpos);
        htd_flashlog_init(&m_flashlog_object, &m_flashlog_config);
        //print_debug("after: wpos=%d, rpos=%d", m_flashlog_object.wpos, m_flashlog_object.rpos);
        if (compare_write_data() == false) {
            print_error("---222---");
            break;
        }
        print_debug("\r\n\r\n");
    #endif
    }
    print_debug("byte=%02x, i=%d", byte, i);
    return 0;
}
CONSOLE_CMD_EXPORT(flashlog_test_cmd, flashlog_test);
#endif
#endif /*defined(HTD_FLASH_LOG_ENABLE)*/

