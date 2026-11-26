// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL   4
#define DEBUG_TAG              "flash.kv"
#include "htd_flash_common.h"
#include "htd_flash_kv.h"


#if defined(HTD_FLASH_KV_ENABLE)
//////////////////////////////////////////////////////////////////////
typedef struct {
    uint32_t checksum:8;
    uint32_t key:12;            // 可以保存最大值为0~4095
    uint32_t len:12;            // 
    uint32_t index;             // 计数值，越大越新
} flashkv_head_t;
static const uint32_t m_flashkv_page_swap_tag = 0xb5a6a7a1;
static const uint32_t m_flashkv_init_flag = 0x9f7e53c6;
static const uint16_t m_flashkv_invalid_key = 0xfff;
static const uint16_t m_flashkv_invalid_addr = 0xffff;


//////////////////////////////////////////////////////////////////////
// @brief 计算校验和
__STATIC_INLINE uint8_t flashkv_checksum_get(const uint8_t* p_data, uint16_t data_len)
{
    return htd_flash_crc8(p_data, data_len, 0);
}
// @brief 地址编码
__STATIC_INLINE uint16_t flashkv_addr_encode(uint32_t start, uint32_t addr)
{
    return (addr - start) >> 2;
}
// @brief 地址解码
__STATIC_INLINE uint32_t flashkv_addr_decode(uint32_t start, uint32_t addr)
{
    return ((addr << 2) + start);
}
// @brief 计算数据页数
__STATIC_INLINE uint32_t flashkv_data_page_count_get(const flash_config_t* p_cfg, const flash_handle_t* p_flash)
{
    return ((p_cfg->end - p_cfg->start) / p_flash->erase_unit);
}
// @brief 计算当前页是否有足够的空间写入数据
__STATIC_INLINE uint8_t flashkv_page_has_enough_space_to_write(const flash_handle_t* p_flash, uint32_t addr, uint16_t write_len)
{
    return (ERASE_ADDR(addr, p_flash->erase_unit) == 
            ERASE_ADDR(addr + write_len + sizeof(flashkv_head_t) - 1, p_flash->erase_unit));
}
// @brief 获取当前页剩余大小
__STATIC_INLINE uint32_t flashkv_page_remain_len_get(const flash_handle_t* p_flash, uint32_t addr)
{
    return (p_flash->erase_unit - (addr & (p_flash->erase_unit - 1)));
}

// @brief 查找某个key
static uint32_t flashkv_addr_find(flashkv_t* p_fkv, uint16_t key, flashkv_head_t* p_hdr)
{
    uint32_t addr;
    int32_t ret = ERR_FLASH_SUCCESS;
    
    if (key >= p_fkv->key_count) {
        return 0xffffffff;
    }
    if (p_fkv->p_key_table[key] == m_flashkv_invalid_addr) {
        return 0xffffffff;
    }
    addr = flashkv_addr_decode(p_fkv->p_cfg->start, p_fkv->p_key_table[key]);
    if (p_hdr) {
        ret = htd_flash_read(p_fkv->p_flash, addr, p_hdr, sizeof(flashkv_head_t));
        if (ret != ERR_FLASH_SUCCESS) {
            print_error("flash read failed. ret=%d, addr=%08x, key=%d", ret, addr, key);
        }
    }
    return addr;
}

// @brief 写入无效数据
static uint32_t flashkv_write_invalid_data(flashkv_t* p_fkv, uint32_t addr, uint16_t data_len)
{
    flashkv_head_t hdr;
    hdr.index = 0;
    hdr.key = m_flashkv_invalid_key;
    hdr.len = data_len;
    hdr.checksum = 0xff;
    return htd_flash_write(p_fkv->p_flash, addr, &hdr, sizeof(flashkv_head_t));
}

// @brief 获取原有数据的序列号
uint32_t flashkv_index_get(flashkv_t* p_fkv, uint16_t key)
{
    uint32_t addr;
    flashkv_head_t head;
    
    // 查找该 key，如果找到，将其设置为废弃
    addr = flashkv_addr_find(p_fkv, key, NULL);
    if (addr != 0xffffffff) {
        htd_flash_read(p_fkv->p_flash, addr, &head, sizeof(flashkv_head_t));
        return head.index;
    }
    return 0;
}

// @brief 写入数据
static int32_t htd_flashkv_write_data(flashkv_t* p_fkv, uint32_t addr, uint16_t key, const uint8_t* p_data, uint16_t data_len)
{
    flashkv_head_t* p_hdr;
    int32_t ret;
    uint8_t* p_buff = NULL;
    int32_t buff_len;

    p_buff = htd_malloc(ALIGN_UP(data_len, 4) + sizeof(flashkv_head_t));
    ASSERT(p_buff);
    
    p_hdr = (flashkv_head_t*)p_buff;
    p_hdr->key = key;
    p_hdr->len = data_len;
    p_hdr->checksum = 0;
    p_hdr->index = flashkv_index_get(p_fkv, key) + 1;
    buff_len = sizeof(flashkv_head_t);
    print_debug("write addr: %08x, key=%d, checksum=%02x, index=%d", addr, p_hdr->key, p_hdr->checksum, p_hdr->index);
    if (p_data && data_len > 0) {
        p_hdr->checksum = flashkv_checksum_get(p_data, data_len);
        htd_memcpy(&p_buff[buff_len], p_data, data_len);
        buff_len += ALIGN_UP(data_len, 4);
    }
    // 写入数据
    ret = htd_flash_write(p_fkv->p_flash, addr, p_buff, buff_len);
    if (ret == ERR_FLASH_SUCCESS) {
        // 记录新的地址
        p_fkv->p_key_table[key] = flashkv_addr_encode(p_fkv->p_cfg->start, addr);
    }
    htd_free(p_buff);
    return ret;
}

// @brief 写入交换页标志
int32_t flashkv_write_swap_page_flag(const flash_handle_t* p_flash, uint32_t swap_page_index, uint32_t addr)
{
    uint8_t buff[16];
    flashkv_head_t* p_hdr = (flashkv_head_t*)buff;
    p_hdr->key = m_flashkv_invalid_key;
    p_hdr->checksum = 0xff; //flashkv_checksum_get((uint8_t*)&m_flashkv_page_swap_tag, sizeof(uint32_t));
    p_hdr->index = swap_page_index;
    p_hdr->len = sizeof(uint32_t);
    htd_memcpy(&buff[sizeof(flashkv_head_t)], &m_flashkv_page_swap_tag, sizeof(uint32_t));
    return htd_flash_write(p_flash, addr, buff, sizeof(flashkv_head_t) + sizeof(uint32_t));
}

// @brief 写入垃圾页标志
int32_t flashkv_write_invalid_page_flag(const flash_handle_t* p_flash, uint32_t addr)
{
    uint32_t invalid_flag = ~m_flashkv_page_swap_tag;
    uint8_t buff[sizeof(flashkv_head_t)];
    flashkv_head_t* p_hdr = (flashkv_head_t*)buff;
    htd_flash_read(p_flash, addr, p_hdr, sizeof(flashkv_head_t));
    if (p_hdr->key == m_flashkv_invalid_key && p_hdr->len == sizeof(uint32_t)) {
        return htd_flash_write(p_flash, addr + sizeof(flashkv_head_t), &invalid_flag, sizeof(uint32_t));
    }
    return 0;
}

// @brief 获取有效地址个数
uint16_t flashkv_addr_count_get(flashkv_t* p_fkv)
{
    uint16_t count = 0;
    uint16_t i;
    for (i=0; i<p_fkv->key_count; ++i) {
        if (p_fkv->p_key_table[i] == m_flashkv_invalid_addr) {
            continue;
        }
        count++;
    }
    return count;
}

// @brief 查找相同页的key
uint16_t flashkv_same_page_key_find(flashkv_t* p_fkv, uint32_t addr, uint32_t i)
{
    uint32_t kaddr;
    for (; i<p_fkv->key_count; ++i) {
        if (p_fkv->p_key_table[i] == m_flashkv_invalid_addr) {
            continue;
        }
        kaddr = flashkv_addr_decode(p_fkv->p_cfg->start, p_fkv->p_key_table[i]);
        if (ERASE_ADDR(kaddr, p_fkv->p_flash->erase_unit) == ERASE_ADDR(addr, p_fkv->p_flash->erase_unit)) {
            return i;
        }
    }
    return m_flashkv_invalid_key;
}

// @brief 部分碎片整理，只整理交换页的下一页
void flashkv_gc_section(flashkv_t* p_fkv)
{
    uint32_t addr, new_swap_addr;
    uint32_t dst_addr, swap_index;
    uint32_t i;
    flashkv_head_t *p_hdr;
    uint8_t* p_info;
    uint16_t align_len;

    p_info = htd_malloc(sizeof(flashkv_head_t) + FLASHKV_VAL_MAX_LEN); 
    ASSERT(p_info);
    p_hdr = (flashkv_head_t*)p_info;

    const flash_handle_t* p_flash = p_fkv->p_flash;
    dst_addr = p_fkv->swap_addr;
    dst_addr += (sizeof(flashkv_head_t) + 4);
    htd_flash_read(p_flash, p_fkv->swap_addr, p_hdr, sizeof(flashkv_head_t));
    swap_index = p_hdr->index;

    i = 0;
    new_swap_addr = htd_flash_next_page_addr_get(p_fkv->p_cfg, p_flash, p_fkv->swap_addr);
    while (1) {
        // 部分整理，只整理交换页的下一页
        i = flashkv_same_page_key_find(p_fkv, new_swap_addr, i);
        if (i == m_flashkv_invalid_key) {
            break;
        }
        
        addr = flashkv_addr_decode(p_fkv->p_cfg->start, p_fkv->p_key_table[i]);
        if (addr < p_fkv->p_cfg->start || addr > p_fkv->p_cfg->end) {
            print_error("gc: addr=%08x, %08x->%08x", addr, p_fkv->p_cfg->start, p_fkv->p_cfg->end);
            p_fkv->p_key_table[i] = m_flashkv_invalid_addr;            // 作废该条信息
            continue;
        }
        
        // 读取头部信息和数据
        htd_memset(p_hdr, 0xff, sizeof(flashkv_head_t));
        htd_flash_read(p_flash, addr, p_hdr, sizeof(flashkv_head_t));
        align_len = ALIGN_UP(p_hdr->len, 4);
        if (p_hdr->key != i) {
            print_error("gc: key=%d->%d", p_hdr->key, i);
            p_fkv->p_key_table[i] = m_flashkv_invalid_addr;            // 作废该条信息
            continue;
        }
        if (align_len > FLASHKV_VAL_MAX_LEN || 
            align_len + sizeof(flashkv_head_t) > flashkv_page_remain_len_get(p_flash, dst_addr)) {
            print_error("gc: align_len=%d->%d, remain=%d", align_len, FLASHKV_VAL_MAX_LEN, flashkv_page_remain_len_get(p_flash, dst_addr));
            p_fkv->p_key_table[i] = m_flashkv_invalid_addr;            // 作废该条信息
            continue;
        }

        if (align_len) {
            htd_flash_read(p_flash, addr+sizeof(flashkv_head_t), &p_info[sizeof(flashkv_head_t)], align_len);
        }
        
        // 写入数据到新地址
        print_debug("gc: key=%d, dst_addr=%08x, len=%d", i, dst_addr, align_len);
        p_fkv->p_key_table[i] = flashkv_addr_encode(p_fkv->p_cfg->start, dst_addr);
        htd_flash_write(p_flash, dst_addr, p_info, sizeof(flashkv_head_t) + align_len);
        dst_addr += (sizeof(flashkv_head_t) + align_len);
    }
    htd_free(p_info);

    p_fkv->addr = dst_addr;
    // 设置新的交换页
    print_warning("swap_addr=%08x->%08x, swap_index=%d", p_fkv->swap_addr, new_swap_addr, swap_index);
    p_fkv->swap_addr = new_swap_addr;
    htd_flash_erase(p_flash, p_fkv->swap_addr);
    flashkv_write_swap_page_flag(p_flash, swap_index+1, p_fkv->swap_addr);
}

// @brief 参数检测
int32_t flashkv_param_assert(const flash_config_t* p_cfg, const flash_handle_t* p_flash)
{
    uint32_t size;
    
    ASSERT(p_cfg);
    ASSERT(p_flash);
    ASSERT(p_flash->flash_erase);
    ASSERT(p_flash->flash_read);
    ASSERT(p_flash->flash_write);
    ASSERT(p_cfg->start < p_cfg->end);
    
    if (! IS_POWER_OF_TWO(p_flash->erase_unit)) {
        print_error("ftl variable init failed. erase_unit=%d", p_flash->erase_unit);
        return ERR_FLASH_INVALID_ARG;
    }
    if (! IS_ERASE_ADDR(p_cfg->start, p_flash->erase_unit)) {
        print_error("start=0x%08x, %d", p_cfg->start, p_flash->erase_unit);
        return ERR_FLASH_INVALID_ARG;
    }
    if (! IS_ERASE_ADDR(p_cfg->end, p_flash->erase_unit)) {
        print_error("end=0x%08x, %d", p_cfg->end, p_flash->erase_unit);
        return ERR_FLASH_INVALID_ARG;
    }
    size = p_cfg->end - p_cfg->start;
    if (size < p_flash->erase_unit * 3) {
        print_error("end=%08x, start=%08x, size=%d", p_cfg->end, p_cfg->start, size);
        return ERR_FLASH_INVALID_ARG;
    }
    return ERR_FLASH_SUCCESS;
}

// @brief 数据起始页地址
uint32_t flashkv_data_start_page_addr_find(flashkv_t* p_fkv)
{
    uint32_t addr;
    uint8_t buff[sizeof(flashkv_head_t) + sizeof(uint32_t)];
    flashkv_head_t* p_hdr = (flashkv_head_t*)buff;
    uint32_t swap_flag, min_index;

    // 查找有效交换页地址
    const flash_handle_t* p_flash = p_fkv->p_flash;
    min_index = 0xffffffff;
    p_fkv->addr = p_fkv->p_cfg->start;
    for (addr=p_fkv->addr; addr < p_fkv->p_cfg->end; addr += p_flash->erase_unit) {
        htd_memset(buff, 0xff, sizeof(flashkv_head_t));
        htd_flash_read(p_flash, addr, buff, sizeof(flashkv_head_t) + sizeof(uint32_t));
        htd_memcpy(&swap_flag, &buff[sizeof(flashkv_head_t)], sizeof(uint32_t));
        
        print_debug("addr=%08x, swap_flag=%08x, key=%02x, index=%d, min_index=%d", addr, swap_flag, p_hdr->key, p_hdr->index, min_index);
        if (p_hdr->key == m_flashkv_invalid_key && p_hdr->checksum == 0xff && swap_flag == m_flashkv_page_swap_tag) {
            if (p_hdr->index < min_index) {
                min_index = p_hdr->index;
                p_fkv->addr = addr;
            }
        }
    }
    return p_fkv->addr;
}

// @brief 交换页地址
uint32_t flashkv_swap_page_addr_find(flashkv_t* p_fkv, uint32_t data_start_addr)
{
    uint32_t page_count;
    uint8_t buff[sizeof(flashkv_head_t) + sizeof(uint32_t)];
    flashkv_head_t* p_hdr = (flashkv_head_t*)buff;
    uint32_t swap_flag, prev_index;

    // 获取第一页的序列号
    const flash_handle_t* p_flash = p_fkv->p_flash;
    prev_index = 0xffffffff;
    htd_flash_read(p_flash, data_start_addr, buff, sizeof(flashkv_head_t) + sizeof(uint32_t));
    htd_memcpy(&swap_flag, &buff[sizeof(flashkv_head_t)], sizeof(uint32_t));
    if (p_hdr->key == m_flashkv_invalid_key && p_hdr->checksum == 0xff && swap_flag == m_flashkv_page_swap_tag) {
        prev_index = p_hdr->index;
    }
    
    if (prev_index != 0xffffffff) {
        // 查找最后一页，最后一页就是交换页
        p_fkv->swap_addr = data_start_addr;
        page_count = flashkv_data_page_count_get(p_fkv->p_cfg, p_flash);
        while (page_count--) {
            data_start_addr = htd_flash_next_page_addr_get(p_fkv->p_cfg, p_flash, data_start_addr);
            htd_flash_read(p_flash, data_start_addr, buff, sizeof(flashkv_head_t) + sizeof(uint32_t));
            htd_memcpy(&swap_flag, &buff[sizeof(flashkv_head_t)], sizeof(uint32_t));
            if (p_hdr->key != m_flashkv_invalid_key ||
                p_hdr->checksum != 0xff ||
                swap_flag != m_flashkv_page_swap_tag ||
                prev_index + 1 != p_hdr->index) {
                break;
            }
            prev_index = p_hdr->index;
            p_fkv->swap_addr = data_start_addr;
        }
    } else {
        // 擦除数据起始页
        htd_flash_range_erase(p_fkv->p_cfg, p_flash, p_fkv->p_cfg->start, 2);
        
        p_fkv->addr = p_fkv->p_cfg->start;
        p_fkv->swap_addr = p_fkv->p_cfg->start;
        flashkv_write_swap_page_flag(p_flash, 0, p_fkv->swap_addr);
        p_fkv->addr = p_fkv->swap_addr + sizeof(flashkv_head_t) + 4;
        
        p_fkv->swap_addr = htd_flash_next_page_addr_get(p_fkv->p_cfg, p_flash, p_fkv->swap_addr);
        flashkv_write_swap_page_flag(p_flash, 1, p_fkv->swap_addr);
    }
    return p_fkv->swap_addr;
}


//////////////////////////////////////////////////////////////////////
// @brief 初始化
int32_t htd_flashkv_init(flashkv_t* p_fkv, const flash_config_t* p_cfg, key_table_t* p_key_table, uint16_t key_count)
{
    int32_t ret;
    flashkv_head_t hdr;
    uint8_t* p_info = NULL;
    uint8_t checksum;
    uint16_t align_len;
    uint32_t start, end;

    ASSERT(p_fkv);
    ASSERT(p_key_table);
    ASSERT(key_count > 0 && key_count < m_flashkv_invalid_key);

    print_debug("flash info: start=%08x, end=%08x", p_cfg->start, p_cfg->end);
    p_fkv->p_flash = htd_flash_handle_find(p_cfg->p_name);
    if (p_fkv->p_flash == NULL) {
        return ERR_FLASH_NOT_FOUND;
    }
    ret = flashkv_param_assert(p_cfg, p_fkv->p_flash);
    if (ERR_FLASH_SUCCESS != ret) {
        return ret;
    }
    
    // 保存基本信息
    p_fkv->p_cfg = p_cfg;
    p_fkv->p_key_table = p_key_table;
    p_fkv->key_count = key_count;
    htd_memset(p_fkv->p_key_table, 0xff, sizeof(key_table_t) * p_fkv->key_count);

    // 查看起始页地址
    flashkv_data_start_page_addr_find(p_fkv);
    // 重置交换地址
    flashkv_swap_page_addr_find(p_fkv, p_fkv->addr);
    print_debug("swap_addr=%08x, data_addr=%08x, start=%08x", p_fkv->swap_addr, p_fkv->addr, p_cfg->start);
    //htd_sleep_ms(2);

    // 序列号列表
    uint32_t *index_table = htd_malloc(sizeof(uint32_t) * key_count);
    ASSERT(index_table);
    htd_memset(index_table, 0, sizeof(uint32_t) * key_count);
    
    // 读取所有数据
    p_info = htd_malloc(FLASHKV_VAL_MAX_LEN);
    ASSERT(p_info);

    p_fkv->init_flag = m_flashkv_init_flag;
    start = ERASE_ADDR(p_fkv->addr, p_fkv->p_flash->erase_unit);
    while (start != p_fkv->swap_addr) {
        //print_debug("data_addr=%08x", start);
        end = start + p_fkv->p_flash->erase_unit;
        while (start + sizeof(flashkv_head_t) < end) {
            htd_memset(&hdr, 0xff, sizeof(flashkv_head_t));
            htd_flash_read(p_fkv->p_flash, start, &hdr, sizeof(flashkv_head_t));
            //print_debug("addr=%08x, len=%04d, key=%04d, index=%d, checksum=%d", start, hdr.len, hdr.key, hdr.index, hdr.checksum);
            // 如果都是 0xff 说明已经结束
            if (htd_flash_mem_allof((uint8_t*)&hdr, 0xff, sizeof(flashkv_head_t))) {
                print_warning("read finish. addr=%08x", start);
                break;
            }
            
            //ASSERT(hdr.len <= FTL_KEY_VAL_MAX_LEN);     // 长度不能超过范围
            if (hdr.len > FLASHKV_VAL_MAX_LEN) {
                print_debug("len=%d->%d", hdr.len, FLASHKV_VAL_MAX_LEN);
                start += sizeof(uint32_t);
                p_fkv->addr = start;
                continue;
            }
            
            // 跳过无效数据
            align_len = ALIGN_UP(hdr.len, 4);
            if (hdr.key == m_flashkv_invalid_key) {
                start += sizeof(flashkv_head_t);
                p_fkv->addr = start;
                print_warning("invalid data. align_len=%d", align_len);
                continue;
            }
            
            // 检验数据
            print_debug("align_len=%d, checksum=%d", align_len, hdr.checksum);
            if (align_len) {
                htd_flash_read(p_fkv->p_flash, start + sizeof(flashkv_head_t), p_info, align_len);
                checksum = flashkv_checksum_get(p_info, hdr.len);
                if (hdr.checksum != checksum) {
                    print_warning("checksum failed. addr=%08x, key=%d, len=%d, checksum=%02x->%02x", start, hdr.key, hdr.len, hdr.checksum, checksum);
                    start += 4;
                    p_fkv->addr = start;
                    continue;
                }
            }
            // 记录内存地址
            if (hdr.key < p_fkv->key_count) {
                print_debug("key=%03d, len=%3d, addr=%08x, index=%d->%d", hdr.key, hdr.len, start, hdr.index, index_table[hdr.key]);
                if (index_table[hdr.key] < hdr.index) {
                    p_fkv->p_key_table[hdr.key] = flashkv_addr_encode(p_cfg->start, start);
                    index_table[hdr.key] = hdr.index;
                }
            }
            //delay_ms(2);
            start += align_len + sizeof(flashkv_head_t);
            p_fkv->addr = start;
        }

        if (start >= end) {
            start = end - sizeof(flashkv_head_t);
        }
        start = htd_flash_next_page_addr_get(p_fkv->p_cfg, p_fkv->p_flash, start);
        print_debug("next page addr=%08x, swap_addr=%08x", start, p_fkv->swap_addr);
    }
    htd_free(index_table);
    htd_free(p_info);

    // 调整数据
    if (p_fkv->addr == p_fkv->swap_addr) {
        flashkv_gc_section(p_fkv);
        htd_flash_sync(p_fkv->p_flash);
    }
    print_debug("init finish: addr=%08x, swap_addr=%08x", p_fkv->addr, p_fkv->swap_addr);
    //delay_ms(100);
    ASSERT(htd_flash_next_page_addr_get(p_fkv->p_cfg, p_fkv->p_flash, p_fkv->addr) == p_fkv->swap_addr);
    return ret;
}

// @brief 去初始化
int32_t htd_flashkv_uninit(flashkv_t* p_fkv)
{
    if (p_fkv->init_flag != m_flashkv_init_flag) {
        return 0;
    }
    p_fkv->init_flag = 0;
    return 0;
}

// @brief 读取数值
int32_t htd_flashkv_read(flashkv_t* p_fkv, uint16_t key, void* p_data, uint16_t data_len)
{
    uint32_t addr;
    flashkv_head_t hdr;
    uint8_t* p_info = NULL;
    uint8_t* p_buff;
    uint8_t checksum;
    int32_t ret;
    uint16_t align_len;

    if (p_fkv->init_flag != m_flashkv_init_flag) {
        print_error("ftl dict isnot init.");
        return 0;
    }
    ASSERT(p_data != NULL);
    ASSERT(p_fkv != NULL);
    ASSERT(key < p_fkv->key_count);
    
    // 查找数据
    HTD_FLASH_MUTEX_LOCK(p_fkv->p_cfg);
    addr = flashkv_addr_find(p_fkv, key, &hdr);
    if (addr == 0xffffffff || hdr.len == 0) {
        ret = ERR_FLASH_NOT_FOUND;
        print_warning("Can't find key: %d", key);
        goto Return;
    }
    if (hdr.key != key) {
        ret = ERR_FLASH_INVALID_DATA;
        print_error("it is a invalid data. rkey=%d->%d, addr=%08x", key, hdr.key, addr);
        goto Return;
    }
    
    align_len = ALIGN_UP(hdr.len, 4);
    if (align_len > FLASHKV_VAL_MAX_LEN) {
        ret = ERR_FLASH_INVALID_DATA;
        print_error("invalid data. align_len=%d->%d", align_len, FLASHKV_VAL_MAX_LEN);
        goto Return;
    }
    print_debug("dict read: key=%d, addr=%08x, align_len=%d->%d", key, addr, align_len, hdr.len);
    addr += sizeof(flashkv_head_t);
    
    if (align_len <= data_len) {
        p_buff = p_data;
    } else {
        p_info = htd_malloc(align_len);
        ASSERT(p_info);
        p_buff = p_info;
    }

    // 读取数据
    ret = htd_flash_read(p_fkv->p_flash, addr, p_buff, align_len);
    if (ret != ERR_FLASH_SUCCESS) {
        print_error("flash read failed. ret=%d, addr=0x%08x, rkey=%d, len=%d", ret, addr, hdr.key, hdr.len);
        goto Return;
    }

    // 校验数据
    checksum = flashkv_checksum_get(p_buff, hdr.len);
    if (hdr.checksum == checksum) {
        data_len = (hdr.len > data_len) ? data_len : hdr.len;
        if (data_len && p_buff != p_data) {
            htd_memcpy(p_data, p_buff, data_len);
        } 
        ret = data_len;
    } else {
        print_error("data check failed. checksum=%d->%d, rkey=%d->%d, len=%d", hdr.checksum, checksum, hdr.key, key, hdr.len);
        ret = ERR_FLASH_INVALID_DATA;
    }

Return:
    HTD_FLASH_MUTEX_UNLOCK(p_fkv->p_cfg);
    if (p_info) {
        htd_free(p_info);
    }
    return ret;
}

// @brief 读取字段存储数据长度
int32_t htd_flashkv_data_len_read(flashkv_t* p_fkv, uint16_t key)
{
    uint32_t addr;
    flashkv_head_t hdr;
    int32_t ret;

    if (p_fkv->init_flag != m_flashkv_init_flag) {
        print_error("ftl dict isnot init.");
        return 0;
    }
    ASSERT(p_fkv != NULL);
    ASSERT(key < p_fkv->key_count);
    
    // 查找数据
    HTD_FLASH_MUTEX_LOCK(p_fkv->p_cfg);
    addr = flashkv_addr_find(p_fkv, key, &hdr);
    if (addr == 0xffffffff || hdr.len == 0) {
        ret = ERR_FLASH_NOT_FOUND;
        print_warning("Can't find key: %d", key);
        goto Return;
    }
    if (hdr.key != key) {
        ret = ERR_FLASH_INVALID_DATA;
        print_error("it is a invalid data. rkey=%d->%d, addr=%08x", key, hdr.key, addr);
        goto Return;
    }
    ret = hdr.len;
Return:
    HTD_FLASH_MUTEX_UNLOCK(p_fkv->p_cfg);
    return ret;
}

// @brief 写入数值
int32_t htd_flashkv_write(flashkv_t* p_fkv, uint16_t key, const void* p_data, uint16_t data_len)
{
    ASSERT(p_fkv);
    ASSERT(data_len <= FLASHKV_VAL_MAX_LEN);

    uint32_t remain_len = 0;
    uint16_t align_len;
    uint8_t write_flag = false;

    if (p_fkv->init_flag != m_flashkv_init_flag) {
        print_error("ftl dict isnot init.");
        return 0;
    }
    ASSERT(key < p_fkv->key_count);

    HTD_FLASH_MUTEX_LOCK(p_fkv->p_cfg);
    // 如果空间不够，进行碎片整理
    align_len = ALIGN_UP(data_len, 4);
    int16_t page_count = flashkv_data_page_count_get(p_fkv->p_cfg, p_fkv->p_flash) - 1;
    while (page_count--) {
        if (flashkv_page_has_enough_space_to_write(p_fkv->p_flash, p_fkv->addr, align_len)) {
            write_flag = true;
            break;
        }
        
        remain_len = flashkv_page_remain_len_get(p_fkv->p_flash, p_fkv->addr);
        if (remain_len >= sizeof(flashkv_head_t)) {
            flashkv_write_invalid_data(p_fkv, p_fkv->addr, remain_len - sizeof(flashkv_head_t));
        }
        
        p_fkv->addr = p_fkv->swap_addr; 
        flashkv_gc_section(p_fkv);
        print_debug("after gc: swap_addr=%08x", p_fkv->swap_addr);
    }
    ASSERT(write_flag);
    
    // 写入新的数据
    htd_flashkv_write_data(p_fkv, p_fkv->addr, key, p_data, data_len);

    print_debug("key=%d, remain_len=%d, addr=%08x, %d, data_len=%d", key, remain_len, p_fkv->addr, p_fkv->addr&4095, data_len);
    p_fkv->addr += (align_len + sizeof(flashkv_head_t));
    if (p_fkv->addr >= p_fkv->p_cfg->end) {
        p_fkv->addr = p_fkv->p_cfg->start;
    }
    if (p_fkv->swap_addr == p_fkv->addr) {
        flashkv_gc_section(p_fkv);
    }
    htd_flash_sync(p_fkv->p_flash);
    HTD_FLASH_MUTEX_UNLOCK(p_fkv->p_cfg);
    return data_len;
}

// @brief 查找和addr距离最近的key
uint16_t htd_flashkv_nearest_key_find(flashkv_t* p_fkv, uint32_t addr, uint32_t* p_distance) 
{
    uint16_t i; 
    uint32_t kaddr;
    uint32_t inter;
    uint32_t distance;
    uint16_t index = m_flashkv_invalid_key;
    uint32_t min_inter = 0xffffffff;
    uint32_t max_distance;

    max_distance = p_fkv->p_cfg->end - p_fkv->p_cfg->start;
    distance = *p_distance;
    for (i=0; i<p_fkv->key_count; ++i) {
        if (p_fkv->p_key_table[i] == m_flashkv_invalid_addr) {
            continue;
        }
        
        kaddr = flashkv_addr_decode(p_fkv->p_cfg->start, p_fkv->p_key_table[i]);
        if (kaddr < addr) {
            inter = kaddr + max_distance - addr;
        } else {
            inter = kaddr - addr;
        }
        
        if (inter < min_inter 
            && inter > distance 
            && (inter+p_fkv->p_flash->erase_unit) < max_distance) {
            min_inter = inter;
            index = i;
        }
    }
    if (index != m_flashkv_invalid_key) {
        *p_distance = min_inter;
        print_debug("min_inter=%08x", min_inter);
    }
    return index;
}

// @brief 碎片整理，将数据页的数据整理到交换页，再将数据转移回来
void htd_flashkv_gc(flashkv_t* p_fkv)
{
    uint32_t remain_len;
    uint32_t addr;
    uint32_t dst_addr;
    uint32_t i;
    uint32_t swap_index;
    uint32_t distance = 0;
    flashkv_head_t *p_hdr;
    uint8_t* p_info;
    uint16_t align_len;

    p_info = htd_malloc(sizeof(flashkv_head_t) + FLASHKV_VAL_MAX_LEN); 
    ASSERT(p_info);
    p_hdr = (flashkv_head_t*)p_info;

    HTD_FLASH_MUTEX_LOCK(p_fkv->p_cfg);
    const flash_handle_t* p_flash = p_fkv->p_flash;
    dst_addr = p_fkv->swap_addr;
    dst_addr += (sizeof(flashkv_head_t) + 4);
    htd_flash_read(p_flash, p_fkv->swap_addr, p_hdr, sizeof(flashkv_head_t));
    swap_index = p_hdr->index;

    while (1) {
        // 全整理，先整理最近的
        i = htd_flashkv_nearest_key_find(p_fkv, p_fkv->swap_addr+p_flash->erase_unit-1, &distance);
        if (i == m_flashkv_invalid_key) {
            break;
        }
        
        addr = flashkv_addr_decode(p_fkv->p_cfg->start, p_fkv->p_key_table[i]);
        if (addr < p_fkv->p_cfg->start || addr > p_fkv->p_cfg->end) {
            print_error("gc: addr=%08x, %08x->%08x", addr, p_fkv->p_cfg->start, p_fkv->p_cfg->end);
            continue;
        }
        
        // 读取头部信息和数据
        htd_memset(p_hdr, 0xff, sizeof(flashkv_head_t));
        htd_flash_read(p_flash, addr, p_hdr, sizeof(flashkv_head_t));
        if (p_hdr->key != i) {
            print_error("gc: key=%d->%d", p_hdr->key, i);
            continue;
        }
        align_len = ALIGN_UP(p_hdr->len, 4);
        if (align_len) {
            htd_flash_read(p_flash, addr+sizeof(flashkv_head_t), &p_info[sizeof(flashkv_head_t)], align_len);
        }
        
        // 判断dst_addr页是否足够写入
        remain_len = flashkv_page_remain_len_get(p_flash, dst_addr);
        if (IS_ERASE_ADDR(dst_addr, p_flash->erase_unit)) {
           remain_len = 0;
        }
        if (remain_len < p_hdr->len + sizeof(flashkv_head_t)) {
            if (remain_len >= sizeof(flashkv_head_t)) {
                flashkv_write_invalid_data(p_fkv, dst_addr, remain_len - sizeof(flashkv_head_t));
            }
            // 擦除下一页
            if (remain_len > 0) {
                dst_addr = htd_flash_next_page_addr_get(p_fkv->p_cfg, p_flash, dst_addr);
            }
            htd_flash_erase(p_flash, dst_addr);
            // 写入新的交换页序号
            flashkv_write_swap_page_flag(p_flash, swap_index++, p_fkv->swap_addr);
        }
        
        // 写入数据到新地址
        print_debug("gc: gkey=%d, dst_addr=%08x, len=%d", i, dst_addr, p_hdr->len);
        p_fkv->p_key_table[i] = flashkv_addr_encode(p_fkv->p_cfg->start, dst_addr);
        htd_flash_write(p_flash, dst_addr, p_info, align_len+sizeof(flashkv_head_t));
        dst_addr += (sizeof(flashkv_head_t) + align_len);
    }
    htd_free(p_info);

    p_fkv->addr = dst_addr;
    // 设置新的交换页地址
    p_fkv->swap_addr = htd_flash_prev_page_addr_get(p_fkv->p_cfg, p_flash, p_fkv->swap_addr);
    // 擦除新的交换页
    htd_flash_erase(p_flash, p_fkv->swap_addr);
    // 写入新的交换页序号
    flashkv_write_swap_page_flag(p_flash, swap_index++, p_fkv->swap_addr);
    htd_flash_sync(p_flash);
    HTD_FLASH_MUTEX_UNLOCK(p_fkv->p_cfg);
}

// @brief 清除所有数据
void htd_flashkv_clear(flashkv_t* p_fkv)
{
    uint32_t addr;
    if (p_fkv->init_flag != m_flashkv_init_flag) {
        print_error("ftl dict isnot init.");
        return ;
    }
    ASSERT(p_fkv);
    
    // 所有数据作废
    HTD_FLASH_MUTEX_LOCK(p_fkv->p_cfg);
    htd_memset(p_fkv->p_key_table, 0xff, p_fkv->key_count * sizeof(key_table_t));
    htd_flash_range_erase(p_fkv->p_cfg, p_fkv->p_flash, p_fkv->p_cfg->start, 2);
    p_fkv->addr = p_fkv->p_cfg->start;
    p_fkv->swap_addr = p_fkv->p_cfg->start;
    flashkv_write_swap_page_flag(p_fkv->p_flash, 0, p_fkv->swap_addr);
    p_fkv->addr = p_fkv->swap_addr + sizeof(flashkv_head_t) + 4;
    p_fkv->swap_addr = htd_flash_next_page_addr_get(p_fkv->p_cfg, p_fkv->p_flash, p_fkv->swap_addr);
    flashkv_write_swap_page_flag(p_fkv->p_flash, 1, p_fkv->swap_addr);
    // 其他页面全部作废
    addr = p_fkv->swap_addr;
    while (1) {
        addr += p_fkv->p_flash->erase_unit;
        if (addr >= p_fkv->p_cfg->end) {
            break;
        }
        flashkv_write_invalid_page_flag(p_fkv->p_flash, addr);
    } 
    htd_flash_sync(p_fkv->p_flash);
    HTD_FLASH_MUTEX_UNLOCK(p_fkv->p_cfg);
}


// @brief 删除某条数据
void htd_flashkv_delete(flashkv_t* p_fkv, uint16_t key)
{
    if (p_fkv->init_flag != m_flashkv_init_flag) {
        print_error("ftl dict isnot init.");
        return ;
    }
    if (key < p_fkv->key_count) {
        if (0xffffffff != flashkv_addr_find(p_fkv, key, NULL)) {
            htd_flashkv_write(p_fkv, key, NULL, 0);
        }
    }
}

// @brief 读取前面N个字节
int32_t htd_flashkv_read_direct(flashkv_t* p_fkv, uint16_t key, void* p_data, uint16_t data_len)
{
    uint32_t addr;
    flashkv_head_t hdr;
    uint16_t align_len;

    if (p_fkv->init_flag != m_flashkv_init_flag) {
        print_error("ftl dict isnot init.");
        return 0;
    }
    ASSERT(p_fkv);
    ASSERT(p_data);
    ASSERT(data_len);
    
    // 查找数据
    addr = flashkv_addr_find(p_fkv, key, &hdr);
    if (addr == 0xffffffff) {
        print_warning("Can't find key: %d", key);
        return ERR_FLASH_NOT_FOUND;
    }
    if (hdr.key != key) {
        print_error("it is a invalid data., rkey=%d->%d, addr=%08x", key, hdr.key, addr);
        return ERR_FLASH_INVALID_DATA;
    }
    align_len = ALIGN_UP(hdr.len, 4);
    if (align_len > FLASHKV_VAL_MAX_LEN) {
        print_error("invalid data. align_len=%d->%d", align_len, FLASHKV_VAL_MAX_LEN);
        return ERR_FLASH_INVALID_DATA;
    }
    if (data_len > align_len) {
        data_len = align_len;
    }
    // 读取数据
    addr += sizeof(flashkv_head_t);
    if (htd_flash_read(p_fkv->p_flash, addr, p_data, data_len) != ERR_FLASH_SUCCESS) {
        print_error("flash read failed. addr=0x%08x, rkey=%d, len=%d", addr, hdr.key, hdr.len);
        return ERR_FLASH_FAILED;
    }
    return data_len;
}

// @brief 获取存储的数据地址
void* htd_flashkv_data_addr_get(flashkv_t* p_fkv, uint16_t key)
{
    uint32_t addr;
    flashkv_head_t hdr;
    ASSERT(p_fkv);

    if (p_fkv->init_flag != m_flashkv_init_flag) {
        print_error("ftl dict isnot init.");
        return NULL;
    }
    addr = flashkv_addr_find(p_fkv, key, &hdr);
    if (hdr.key != key) {
        print_error("it is a invalid data., rkey=%d->%d, addr=%08x", key, hdr.key, addr);
        return NULL;
    }
    return (void*)(addr + sizeof(flashkv_head_t));
}







//////////////////////////////////////////////////////////////////////
#if (0 && defined(DEBUG_LVL) && (DEBUG_LVL >= 4) && defined(HTD_CONSOLE_ENABLE) && defined(HTD_USING_NORFLASH))
#include "htd_dev_norflash.h"
#include "htd_console_process.h"

static flashkv_t m_flashkv_object;
static key_table_t m_flashkv_key_table[32];
static uint32_t m_flashkv_test_count = 0;
const flash_config_t m_flashkv_config = {
    .p_name = "norflash_4k",
    .start = NORFLASH_MAP_START_ADDR,
    .end = NORFLASH_MAP_START_ADDR + 64*1024,
    .lock_handle = NULL,
    .unlock_handle = NULL,
};

typedef struct {
    uint16_t data_len;
    uint8_t* p_data;
}dict_data_table_t;
dict_data_table_t m_flashkv_data_table[32];


// @brief 
void flashkv_data_add(uint8_t key, const uint8_t* p_data, uint16_t data_len)
{
    if (m_flashkv_data_table[key].p_data) {
        htd_free(m_flashkv_data_table[key].p_data);
        m_flashkv_data_table[key].p_data = NULL;
        m_flashkv_data_table[key].data_len = 0;
    }
    if (data_len) {
        m_flashkv_data_table[key].p_data = htd_malloc(data_len);
        ASSERT(m_flashkv_data_table[key].p_data);

        htd_memcpy(m_flashkv_data_table[key].p_data, p_data, data_len);
        m_flashkv_data_table[key].data_len = data_len;
    }
}

// @brief 验证数据内容
void flashkv_data_verify(void)
{
    int i;
    uint8_t *p_data = htd_malloc(512);
    int32_t data_len;

    ASSERT(p_data);
    for (i=0; i<ARRAY_COUNT(m_flashkv_data_table); ++i) {
        if (m_flashkv_data_table[i].data_len == 0 || NULL == m_flashkv_data_table[i].p_data) {
            continue;
        }
        
        data_len = htd_flashkv_read(&m_flashkv_object, i, p_data, 512);
        if (data_len != m_flashkv_data_table[i].data_len || 
            htd_memcmp(p_data, m_flashkv_data_table[i].p_data, data_len) != 0) {
            print_error("----verify failed-------data_len=%d->%d, i=%d", data_len, m_flashkv_data_table[i].data_len, i);
            htd_sleep_ms(1000);
            ASSERT(0);
        }
    }
    htd_free(p_data);
}

// @brief 
void ftl_test_timer_handler(void* arg)
{
    static uint8_t test_buff[512];
    static uint8_t test_data[512];
    uint16_t buff_len;
    uint16_t data_len;
    uint8_t key;


    int32_t ret = htd_flashkv_init(&m_flashkv_object, &m_flashkv_config, m_flashkv_key_table, ARRAY_COUNT(m_flashkv_key_table));
    ASSERT(ret == ERR_FLASH_SUCCESS);
    flashkv_data_verify();

    buff_len = htd_rng_val_get() % (ARRAY_COUNT(test_buff) - 1);
    key = htd_rng_val_get() % ARRAY_COUNT(m_flashkv_key_table);
    
    if (buff_len == 0) {
        // 删除该条数据
        htd_flashkv_delete(&m_flashkv_object, key);
        flashkv_data_add(key, NULL, 0);
        print_warning("deleate: key=%d", key);
        return ;
    }

    htd_memset(test_buff, buff_len, sizeof(test_buff));
    htd_flashkv_write(&m_flashkv_object, key, test_buff, buff_len);
    flashkv_data_add(key, test_buff, buff_len);
    print_warning("write: key=%d, buff_len=%d", key, buff_len);

    data_len = htd_flashkv_read(&m_flashkv_object, key, test_data, sizeof(test_data));
    if (data_len != buff_len) {
        print_warning("test failed: data_len=%d, buff_len=%d", data_len, buff_len);
        goto TestFailed;
    }
    if (htd_memcmp(test_data, test_buff, buff_len) != 0) {
        print_warning("test failed. buff_len=%d, data_len=%d", buff_len, data_len);
        goto TestFailed;
    }

    flashkv_data_verify();
    print_warning("test success. m_flashkv_test_count=%d\r\n\r\n", m_flashkv_test_count);
    return;
    
TestFailed:
    print_warning("test failed.\r\n\r\n");
}

// @brief 测试
int flashkv_test_cmd(int argc, void* argv) 
{
    m_flashkv_test_count = 0;
    htd_memset(m_flashkv_data_table, 0, sizeof(dict_data_table_t) * ARRAY_COUNT(m_flashkv_data_table));
    while (m_flashkv_test_count++ < 100) {
        ftl_test_timer_handler(NULL);
        htd_sleep_ms(1);
        print_debug("\r\n\r\n\r\n\r\n");
    }
    return ERR_FLASH_SUCCESS;
}
CONSOLE_CMD_EXPORT(flashkv_test_cmd, flashkv_test);
#endif
#endif /*defined(HTD_FLASH_KV_ENABLE)*/

