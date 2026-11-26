// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL   4
#define DEBUG_TAG              "flash.box"
#include "htd_flash_common.h"
#include "htd_flash_box.h"


#if defined(HTD_FLASH_BOX_ENABLE)
//////////////////////////////////////////////////////////////////////
#if 0
|------------------box------------------|
|-------block1------|-------block2------|
|  magic....data    |  magic....data    |
#endif
typedef struct {
    uint32_t magic;             // 0xf1f25b5c
    uint16_t tag;               // 该片区的标签
    uint16_t addr;              // 该片区的起始地址
    uint32_t sidx;              // 该片区的起始序列号
    uint32_t serials;         
    uint32_t user_flag;         // 页数，一个block最多32页
} block_head_t;
const static uint32_t m_flashbox_default_magic = 0xa7b85d5a;
const static uint16_t m_flashbox_init_flag = 0xa5a5;


//////////////////////////////////////////////////////////////////////
// @brief 地址编码
__STATIC_INLINE uint16_t flashbox_addr_encode(const flash_config_t* p_cfg, const flash_handle_t* p_flash, uint32_t addr)
{
    return (addr - p_cfg->start) / p_flash->erase_unit;
}
// @brief 地址解码
__STATIC_INLINE uint32_t flashbox_addr_decode(const flash_config_t* p_cfg, const flash_handle_t* p_flash, uint16_t addr)
{
    return (addr * p_flash->erase_unit) + p_cfg->start;
}

// @brief 获取片区某一条数据地址
static uint32_t flashbox_data_start_addr_get(const flash_box_t* p_fbx, block_info_t* p_block, uint16_t index)
{
    uint32_t start;
    start = flashbox_addr_decode(p_fbx->p_cfg, p_fbx->p_flash, p_block->addr);
    start += (sizeof(block_head_t) + p_fbx->user_data_len);
    start = htd_flash_next_addr_get(p_fbx->p_cfg, start, index * p_fbx->data_len);
    return start;
}

// @brief 获取片区某一条数据的结束地址
static uint32_t flashbox_data_end_addr_get(const flash_box_t* p_fbx, block_info_t* p_block, uint16_t index)
{
    uint32_t addr, offset;
    addr = flashbox_addr_decode(p_fbx->p_cfg, p_fbx->p_flash, p_block->addr);
    offset = sizeof(block_head_t) + p_fbx->user_data_len;
    index++;
    offset += index * p_fbx->data_len;
    offset--;
    return htd_flash_next_addr_get(p_fbx->p_cfg, addr, offset);
}

// @brief 写入头部数据
static void flashbox_page_head_write(flash_box_t* p_fbx, uint32_t page_addr, const block_info_t* p_block, uint32_t erase_size)
{
    uint32_t flag_user = 0;
    block_head_t hdr;
    hdr.magic = m_flashbox_default_magic;
    hdr.addr = p_block->addr;
    hdr.tag = p_block->tag;
    hdr.sidx = p_block->sidx;
    hdr.serials = p_block->serials;
    while (erase_size) {
        flag_user = flag_user << 1;
        flag_user |= 1;
        if (erase_size < p_fbx->p_flash->erase_unit) {
            break;
        }
        erase_size -= p_fbx->p_flash->erase_unit;
    }
    hdr.user_flag = 0xffffffff;
    hdr.user_flag ^= flag_user;
    print_debug("head write: block_addr=%d, page_addr=%08x, user_flag=%08x->%08x", p_block->addr, page_addr, hdr.user_flag, flag_user);
    htd_flash_write(p_fbx->p_flash, page_addr, &hdr, sizeof(block_head_t));
}

// @brief 更新使用位
static void flashbox_user_flag_update(flash_box_t* p_fbx, const block_info_t* p_block, uint32_t erase_addr)
{
    uint32_t user_flag = 0xffffffff;
    uint32_t start, addr;
    uint32_t val;
    uint32_t index = 0;
    
    start = flashbox_addr_decode(p_fbx->p_cfg, p_fbx->p_flash, p_block->addr); 
    htd_flash_read(p_fbx->p_flash, start + OFFSETOF(block_head_t, user_flag), &user_flag, sizeof(uint32_t));
    addr = start;
    while (addr != erase_addr) {
        index++;
        print_debug("addr=%08x, erase_addr=%08x", addr, erase_addr);
        addr = htd_flash_next_page_addr_get(p_fbx->p_cfg, p_fbx->p_flash, addr);
    }
    
    val = 1 << index;
    if (user_flag & val) {
        user_flag &= ~val;
        htd_flash_write(p_fbx->p_flash, start + OFFSETOF(block_head_t, user_flag), &user_flag, sizeof(uint32_t));
    }
    print_debug("user_flag=%08x, index=%d", user_flag, index);
}

// @brief 总共写了多少页
static uint32_t flashbox_user_page_count_get(const flash_box_t* p_fbx, const block_info_t* p_block)
{
    uint32_t page_count, i, start;
    uint32_t user_flag;

    page_count = p_fbx->block_len / p_fbx->p_flash->erase_unit;
    if (page_count <= 1) {
        return 1;
    }
    start = flashbox_addr_decode(p_fbx->p_cfg, p_fbx->p_flash, p_block->addr) + OFFSETOF(block_head_t, user_flag);
    user_flag = 0xffffffff;
    htd_flash_read(p_fbx->p_flash, start, &user_flag, sizeof(uint32_t));
    print_debug("eidx adjust. user_flag=%08x", user_flag);
    for (i=0; i<32; ++i) {
        if ((user_flag & 0x01) == 0) {
            page_count = i;
        }
        user_flag >>= 1;
    }
    return page_count + 1;
}

// @brief 删除片区
static void flashbox_delete(flash_box_t * p_fbx, uint8_t index, bool move)
{
    uint32_t addr;
    block_info_t* p_block;
    uint32_t val = ~m_flashbox_default_magic;

    p_block = &p_fbx->p_block[index];
    if (p_block->addr != 0xffff && p_block->tag != 0xffff) {
        addr = flashbox_addr_decode(p_fbx->p_cfg, p_fbx->p_flash, p_block->addr);
        addr += OFFSETOF(block_head_t, magic);
        htd_flash_write(p_fbx->p_flash, addr, &val, sizeof(uint32_t));
        if (move) {
            // 删除某一个，将后面的全部往前移
            htd_memmove(&p_fbx->p_block[index], &p_fbx->p_block[index+1], (p_fbx->block_count - 1 - index) * sizeof(block_info_t));
            p_block = &p_fbx->p_block[p_fbx->block_count - 1];
            print_debug("block delete: tag=%d, addr=%d, %d, %d", p_block->tag, p_block->addr, index, p_fbx->block_count-1-index);
        }
    }
    p_block->addr = 0xffff;
    p_block->tag = 0xffff;
    p_block->sidx = 0;
    p_block->eidx = 0;
    p_block->serials = 0;
}

// @brief 参数检测
static int32_t flashbox_param_assert(const flash_box_t* p_fbx, const flash_config_t* p_cfg, uint8_t block_count, uint32_t block_len)
{
    uint32_t size;
    
    ASSERT(p_fbx);
    ASSERT(p_cfg);
    ASSERT(p_fbx->p_flash);
    ASSERT(p_fbx->p_flash->flash_erase);
    ASSERT(p_fbx->p_flash->flash_read);
    ASSERT(p_fbx->p_flash->flash_write);
    if (! IS_POWER_OF_TWO(p_fbx->p_flash->erase_unit)) {
        print_error("ftl variable init failed. erase_unit=%d", p_fbx->p_flash->erase_unit);
        return ERR_FLASH_INVALID_ARG;
    }
    if (! IS_ERASE_ADDR(p_cfg->start, p_fbx->p_flash->erase_unit)) {
        print_error("start=0x%08x, %d", p_cfg->start, p_fbx->p_flash->erase_unit);
        return ERR_FLASH_INVALID_ARG;
    }
    if (! IS_ERASE_ADDR(p_cfg->end, p_fbx->p_flash->erase_unit)) {
        print_error("end=0x%08x, %d", p_cfg->end, p_fbx->p_flash->erase_unit);
        return ERR_FLASH_INVALID_ARG;
    }
    if (! IS_ERASE_ADDR(block_len, p_fbx->p_flash->erase_unit)) {
        print_error("day_len=0x%08x, %d", block_len, p_fbx->p_flash->erase_unit);
        return ERR_FLASH_INVALID_ARG;
    }
    if (p_cfg->start > p_cfg->end) {
        print_error("end=%08x, start=%08x", p_cfg->end, p_cfg->start);
        return ERR_FLASH_INVALID_ARG;
    }
    size = p_cfg->end - p_cfg->start;
    if (size < p_fbx->p_flash->erase_unit * block_count) {
        print_error("end-start=%08x->%08x", size, p_fbx->p_flash->erase_unit * block_count);
        return ERR_FLASH_INVALID_ARG;
    }
    if (block_len <= p_fbx->user_data_len + sizeof(block_head_t)) {
        print_error("block_len=%u, user_data_len=%d", block_len, p_fbx->user_data_len);
        return ERR_FLASH_INVALID_ARG;
    }
    if (block_len / p_fbx->p_flash->erase_unit >= 32) {
        print_error("block_len error. size=%u, max=%lu", size, 32 * p_fbx->p_flash->erase_unit - p_fbx->user_data_len - sizeof(block_head_t));
        return ERR_FLASH_INVALID_ARG;
    }
    if (p_fbx->fixed_size_flag) {
        size = p_cfg->end - p_cfg->start;
        if (size < block_len * block_count) {
            print_error("end-start=%08x, block_len=%08x, block_count=%d", size, block_len, block_count);
            return ERR_FLASH_INVALID_ARG;
        }   
    } else {
        if (size < block_len) {
            print_error("end-start=%08x, block_len=%08x, block_count=%d", size, block_len, block_count);
            return ERR_FLASH_INVALID_ARG;
        }
    }
    return ERR_FLASH_SUCCESS;
}

// @brief 调整片区最后的序列号
static void flashbox_eidx_adjust(const flash_box_t* p_fbx, block_info_t* p_block)
{
    if (p_block->tag == 0xffff || p_block->addr == 0xffff || p_block->sidx >= p_fbx->data_count) {
        return;
    }
    
    uint32_t page_count = flashbox_user_page_count_get(p_fbx, p_block);
    print_debug("page_count=%d", page_count);

    uint32_t addr, start, index;
    start = sizeof(block_head_t) + p_fbx->user_data_len;
    addr = page_count * p_fbx->p_flash->erase_unit;
#if 0
    ASSERT(addr > start);
#else
    while (addr < start) {
        print_warning("addr=%08x, start=%08x", addr, start);
        addr += p_fbx->p_flash->erase_unit;
    }
#endif
    addr -= start;
    index = addr / p_fbx->data_len - 1;
    start += flashbox_addr_decode(p_fbx->p_cfg, p_fbx->p_flash, p_block->addr);
    addr = htd_flash_next_addr_get(p_fbx->p_cfg, start, index * p_fbx->data_len);
    print_debug("tag=%d, start=%08x, addr=%08x, sidx=%d, index=%d", p_block->tag, start, addr, p_block->sidx, index);

    // 从后往前遍历
    uint8_t data[FTL_DATA_VAL_MAX_LEN];
    p_block->eidx = p_block->sidx;
    while (index >= p_block->sidx) {
        htd_memset(data, 0, p_fbx->data_len);
        htd_flash_range_read(p_fbx->p_cfg, p_fbx->p_flash, addr, data, p_fbx->data_len);
        //print_debug("addr=%08x, index=%d, val=%d", addr, index, *(uint32_t*)data);
        if (!htd_flash_mem_allof(data, 0xff, p_fbx->data_len)) {
            p_block->eidx = index + 1;
            break;
        }
        if (index == p_block->sidx) {
            break;
        }
        index--;
        addr -= p_fbx->data_len;
        if (addr < p_fbx->p_cfg->start) {
            addr = p_fbx->p_cfg->start - addr;
            addr = p_fbx->p_cfg->end - addr;
        }
    }
    print_debug("eidx adjust. tag=%d, index=%d, sidx=%d, eidx=%d, addr=%08x", p_block->tag, index, p_block->sidx, p_block->eidx, addr);
}

// @brief 查找对应地址的片区序号
static block_info_t* flashbox_block_find(flash_box_t* p_fbx, uint16_t tag)
{
    uint32_t i = p_fbx->block_count;
    while (i--) {
        if (p_fbx->p_block[i].tag == tag) {
            return &p_fbx->p_block[i];
        }
    }
    return NULL;
}

// @brief 移除起始地址相同的片区
static void flashbox_remove_same_start_addr_block(flash_box_t* p_fbx, uint16_t tag, uint32_t addr)
{
    uint32_t i;
    for (i=0; i<p_fbx->block_count; ++i) {
        if (tag == p_fbx->p_block[i].tag || p_fbx->p_block[i].tag == 0xffff) {
            continue;
        }
        if (flashbox_addr_decode(p_fbx->p_cfg, p_fbx->p_flash, p_fbx->p_block[i].addr) == addr) {
            p_fbx->p_block[i].tag = 0xffff;
        }
    }
}

// @brief 上一次写入的地址
static uint32_t flashbox_prev_write_addr_get(flash_box_t* p_fbx, block_info_t* p_block)
{
    uint32_t addr = flashbox_data_end_addr_get(p_fbx, p_block, p_block->eidx-1);
    return ERASE_ADDR(addr, p_fbx->p_flash->erase_unit);
}

// @brief 往片区写入一条数据
static int32_t flashbox_insert(flash_box_t* p_fbx, block_info_t* p_block, uint16_t index, const void* p_data)
{
    uint32_t waddr;
    if (index < p_block->eidx) {
        //ftl_warming("ftl block write failed. index=%d, eidx=%d", index, p_block->eidx);
        return ERR_FLASH_INVALID_ARG;
    }
    
    const flash_handle_t* p_flash = p_fbx->p_flash;
    uint32_t start_erase = flashbox_prev_write_addr_get(p_fbx, p_block);
    waddr = flashbox_data_end_addr_get(p_fbx, p_block, index);
    uint32_t end_erase = ERASE_ADDR(waddr, p_flash->erase_unit);
    while (start_erase != end_erase) {
        start_erase = htd_flash_next_page_addr_get(p_fbx->p_cfg, p_flash, start_erase);
        htd_flash_erase(p_flash, start_erase);
        flashbox_user_flag_update(p_fbx, p_block, start_erase);
    }
    
    //TODO: 写入数据之前，可以先对数据进行加密
    p_block->eidx = index + 1;
    waddr = flashbox_data_start_addr_get(p_fbx, p_block, index);
    print_debug("write: tag=%d, addr=%08x, index=%d, sidx=%d->%d", p_block->tag, waddr, index, p_block->sidx, p_block->eidx);
    htd_flash_range_write(p_fbx->p_cfg, p_flash, waddr, p_data, p_fbx->data_len);
    return ERR_FLASH_SUCCESS;
}

// @brief 往片区追加一条数据
static int32_t flashbox_append(flash_box_t* p_fbx, block_info_t* p_block, const void* p_data)
{
    const flash_handle_t* p_flash;
    uint32_t waddr;
    uint32_t end_erase, start_erase;

    if (p_block->eidx > p_fbx->data_count) {
        print_warning("ftl block write failed. eidx=%d, max=%d", p_block->eidx, p_fbx->data_count);
        return ERR_FLASH_INVALID_ARG;
    }
    p_flash = p_fbx->p_flash;
    start_erase = flashbox_prev_write_addr_get(p_fbx, p_block);
    waddr = flashbox_data_end_addr_get(p_fbx, p_block, p_block->eidx);
    end_erase = ERASE_ADDR(waddr, p_flash->erase_unit);
    if (start_erase != end_erase) {
        if (false == p_fbx->fixed_size_flag) {
            flashbox_remove_same_start_addr_block(p_fbx, p_block->tag, end_erase);
        }
        htd_flash_erase(p_flash, end_erase);
        flashbox_user_flag_update(p_fbx, p_block, end_erase);
    }
    //TODO: 写入数据之前，可以先对数据进行加密
    waddr = flashbox_data_start_addr_get(p_fbx, p_block, p_block->eidx);
    htd_flash_range_write(p_fbx->p_cfg, p_flash, waddr, p_data, p_fbx->data_len);
    print_debug("write: tag=%d, addr=%08x, sidx=%d->%d", p_block->tag, waddr, p_block->sidx, p_block->eidx);
    p_block->eidx++;
    return ERR_FLASH_SUCCESS;
}

// @brief 获取片区地址
static uint16_t flashbox_block_addr_get(flash_box_t* p_fbx, block_info_t* p_block)
{
    uint32_t addr;
    if (p_fbx->fixed_size_flag) {
        addr = flashbox_addr_decode(p_fbx->p_cfg, p_fbx->p_flash, p_block->addr) + p_fbx->block_len;
    } else {
        addr = flashbox_data_start_addr_get(p_fbx, p_block, p_block->eidx);
        addr = htd_flash_next_page_addr_get(p_fbx->p_cfg, p_fbx->p_flash, addr);
    }
    if (addr >= p_fbx->p_cfg->end) {
        addr -= p_fbx->p_cfg->end;
        addr += p_fbx->p_cfg->start;
    }
    return flashbox_addr_encode(p_fbx->p_cfg, p_fbx->p_flash, addr);
}

// @brief 判断连续N个block的tag是否都未使用
bool flashbox_nblock_is_all_unused(block_info_t* p_block, uint32_t n)
{
    for (uint32_t i=0; i<n; ++i) {
        if (p_block->tag != 0xffff) {
            return false;
        }
    }
    return true;
}

// @brief 创建一个新的块
void flashbox_new_block_create(flash_box_t* p_fbx, block_info_t* p_block)
{
    uint32_t waddr, addr;
    addr = flashbox_addr_decode(p_fbx->p_cfg, p_fbx->p_flash, p_block->addr);
    flashbox_remove_same_start_addr_block(p_fbx, p_block->tag, addr);

    uint32_t erase_size = (sizeof(block_head_t) + p_fbx->user_data_len);
    uint32_t page_count = erase_size / p_fbx->p_flash->erase_unit;
    if (! IS_ERASE_ADDR(erase_size, p_fbx->p_flash->erase_unit)) {
        page_count++;
    }
    htd_flash_range_erase(p_fbx->p_cfg, p_fbx->p_flash, addr, page_count);
    flashbox_page_head_write(p_fbx, addr, p_block, erase_size);
    addr += erase_size;
    
    if (p_fbx->fixed_size_flag) {
        waddr = flashbox_data_start_addr_get(p_fbx, p_block, p_block->sidx);
        waddr = ERASE_ADDR(waddr, p_fbx->p_flash->erase_unit);
        if (ERASE_ADDR(addr, p_fbx->p_flash->erase_unit) != waddr) {
            htd_flash_erase(p_fbx->p_flash, waddr);
            flashbox_user_flag_update(p_fbx, p_block, waddr);
        }
    }
}

// @brief 获取新的片区
static block_info_t* flashbox_new_block_get(flash_box_t* p_fbx, uint16_t tag, uint16_t index)
{
    uint32_t i;
    block_info_t* p_block;

    p_block = flashbox_block_find(p_fbx, tag);
    if (p_block) {
        return p_block;
    }

    // 查找未使用的
    for (i=0; i<p_fbx->block_count; ++i) {
        if (p_fbx->p_block[i].tag == 0xffff) {
            ASSERT(flashbox_nblock_is_all_unused(&p_fbx->p_block[i], p_fbx->block_count-i));
            p_block = &p_fbx->p_block[i];
            htd_memset(p_block, 0, sizeof(block_info_t));
            if (i == 0) {
                p_block->addr = flashbox_addr_encode(p_fbx->p_cfg, p_fbx->p_flash, p_fbx->p_cfg->start);
            } else {
                p_block->addr = flashbox_block_addr_get(p_fbx, &p_fbx->p_block[i-1]);
            }
            p_fbx->last_serials++;
            p_block->serials = p_fbx->last_serials;
            p_block->tag = tag;
            p_block->sidx = index;
            p_block->eidx = index;
            flashbox_new_block_create(p_fbx, p_block);
            return p_block;
        }
    }
    
    // 片区已经满了, 删掉第一个
    flashbox_delete(p_fbx, 0, true);
    i = p_fbx->block_count - 1;
    p_block = &p_fbx->p_block[i];
    htd_memset(p_block, 0, sizeof(block_info_t));
    p_block->addr = flashbox_block_addr_get(p_fbx, &p_fbx->p_block[i-1]);
    p_block->tag = tag;
    p_block->sidx = index;
    p_block->eidx = index;
    p_fbx->last_serials++;
    p_block->serials = p_fbx->last_serials;
    flashbox_new_block_create(p_fbx, p_block);
    return p_block;
}


//////////////////////////////////////////////////////////////////////
// @brief 根据序列号插入新的块
static uint32_t flashbox_block_insert(block_info_t* p_block, uint32_t n, uint32_t nmax, block_info_t* p_new)
{
    uint32_t i;
    for (i=0; i<n; ++i) {
        if (p_new->serials < p_block[i].serials) {
            htd_memmove(&p_block[i+1], &p_block[i], sizeof(block_info_t) * (n - i));
            break;
        }
    }
    if (i < nmax) {
        htd_memcpy(&p_block[i], p_new, sizeof(block_info_t));
        return n + 1;
    } else {
        ASSERT(0);
        return n;
    }
}

// @brief 数据初始化
int32_t htd_flashbox_init(flash_box_t* p_fbx, const flash_config_t* p_cfg, block_info_t* p_block, uint8_t block_count)
{
    uint32_t addr;
    block_head_t head;
    uint32_t idx;
    int32_t page_count;
    block_info_t block;
    
    ASSERT(p_block);
    ASSERT(block_count > 0);

    p_fbx->p_flash = htd_flash_handle_find(p_cfg->p_name);
    if (p_fbx->p_flash == NULL) {
        return ERR_FLASH_NOT_FOUND;
    }
    if (flashbox_param_assert(p_fbx, p_cfg, block_count, p_fbx->block_len) != ERR_FLASH_SUCCESS) {
        ASSERT(0);
        return ERR_FLASH_INVALID_ARG;
    }
    if ((p_fbx->data_len & 0x03) || p_fbx->data_len > FTL_DATA_VAL_MAX_LEN) {
        print_error("ftl data init failed. data_len=%d", p_fbx->data_len);
        ASSERT(0);
        return ERR_FLASH_INVALID_ARG;
    }
    p_fbx->last_serials = 0;
    p_fbx->user_data_len = ALIGN_UP(p_fbx->user_data_len, 4);
    p_fbx->p_cfg = p_cfg;
    p_fbx->p_block = p_block;
    p_fbx->block_count = block_count; 
    p_fbx->data_count = (p_fbx->block_len - sizeof(block_head_t) - p_fbx->user_data_len) / p_fbx->data_len;
    page_count = (p_cfg->end - p_cfg->start) / p_fbx->p_flash->erase_unit;
    htd_memset(p_block, 0xff, sizeof(block_info_t) * block_count);

    // 根据序列号进行排序，第一个放的序列号最小
    idx = 0;
    for (addr=p_cfg->start; addr<p_cfg->end; addr+=p_fbx->p_flash->erase_unit) {
        htd_memset(&head, 0xff, sizeof(block_head_t));
        htd_flash_read(p_fbx->p_flash, addr, &head, sizeof(block_head_t));
        if (head.magic != m_flashbox_default_magic || flashbox_addr_decode(p_fbx->p_cfg, p_fbx->p_flash, head.addr) != addr) {
            print_debug("magic=%08x, tag=%d, sidx=%d, serials=%d", head.magic, head.tag, head.sidx, head.serials);
            continue;
        }
        block.addr = head.addr;
        block.tag = head.tag;
        block.sidx = head.sidx;
        block.serials = head.serials;
        idx = flashbox_block_insert(p_fbx->p_block, idx, p_fbx->block_count, &block);
        if (p_fbx->last_serials < block.serials) {
            p_fbx->last_serials = block.serials;
        }
        print_warning("serials=%u, tag=%d, sidx=%d, addr=%08x->%08x, idx=%d", head.serials, head.tag, head.sidx, head.addr, addr, idx);
    }
    
    // 调整结束位置
    if (idx > 0) {
        for (addr=0; addr<idx; ++addr) {
            flashbox_eidx_adjust(p_fbx, &p_block[addr]);
        }
    } 
    p_fbx->init_flag = m_flashbox_init_flag;
    
    htd_flashbox_block_info_print(p_fbx);
    return ERR_FLASH_SUCCESS;
}

// @brief 数据初始化
int32_t htd_flashbox_uninit(flash_box_t* p_fbx)
{
    if (p_fbx->init_flag != m_flashbox_init_flag) {
        return ERR_FLASH_INVALID_ARG;
    }
    p_fbx->init_flag = 0;
    return 0;
}

// @brief 判断是否初始化
bool htd_flashbox_is_inited(flash_box_t* p_fbx)
{
    return p_fbx->init_flag == m_flashbox_init_flag;
}

// @brief 指定序号添加数据, fixed_size_flag 必须为 true, 才能指定index进行添加
// @param tag 标签; index 序号; p_data 数据内容
// @node: 加入的数据p_data 不能全部都是0xff
int32_t htd_flashbox_add(flash_box_t* p_fbx, uint16_t tag, uint16_t index, const void* p_data)
{
    block_info_t* p_block;
    int32_t ret;
    
    ASSERT(NULL != p_data);
    ASSERT(NULL != p_fbx);
    ASSERT(p_fbx->fixed_size_flag);
    if (p_fbx->init_flag != m_flashbox_init_flag) {
        return ERR_FLASH_INVALID_ARG;
    }
    
    if (index >= p_fbx->data_count) {
        print_warning("ftl block write failed. index=%d, max=%d", index, p_fbx->data_count);
        return ERR_FLASH_INVALID_ARG;
    }
    // 获取对应的片区
    HTD_FLASH_MUTEX_LOCK(p_fbx->p_cfg);
    p_block = flashbox_new_block_get(p_fbx, tag, index);
    if (p_block == NULL) {
        HTD_FLASH_MUTEX_UNLOCK(p_fbx->p_cfg);
        return ERR_FLASH_NO_MEM;
    }
    ret = flashbox_insert(p_fbx, p_block, index, p_data);
    htd_flash_sync(p_fbx->p_flash);
    HTD_FLASH_MUTEX_UNLOCK(p_fbx->p_cfg);
    return ret;
}

// @brief 按序添加数据
// @node: 加入的数据p_data 不能全部都是0xff
int32_t htd_flashbox_append(flash_box_t* p_fbx, uint16_t tag, const void* p_data)
{
    block_info_t* p_block;
    int32_t ret;
    
    ASSERT(NULL != p_data);
    ASSERT(NULL != p_fbx);
    if (p_fbx->init_flag != m_flashbox_init_flag) {
        return ERR_FLASH_INVALID_ARG;
    }
    
    // 获取对应的片区
    HTD_FLASH_MUTEX_LOCK(p_fbx->p_cfg);
    p_block = flashbox_new_block_get(p_fbx, tag, 0);
    if (p_block == NULL) {
        HTD_FLASH_MUTEX_UNLOCK(p_fbx->p_cfg);
        return ERR_FLASH_NO_MEM;
    }

    // 计算对应区域位置
    ret = flashbox_append(p_fbx, p_block, p_data);
    htd_flash_sync(p_fbx->p_flash);
    //ftl_warming("ret=%d, tag=%d, eidx=%d", ret, tag, p_block->eidx);
    HTD_FLASH_MUTEX_UNLOCK(p_fbx->p_cfg);
    return ret;
}

// @brief 新增一个块
int32_t htd_flashbox_block_add(flash_box_t* p_fbx, uint16_t tag)
{
    block_info_t* p_block;
    
    ASSERT(NULL != p_fbx);
    if (p_fbx->init_flag != m_flashbox_init_flag) {
        return ERR_FLASH_INVALID_ARG;
    }

     // 获取对应的片区
    HTD_FLASH_MUTEX_LOCK(p_fbx->p_cfg);
    p_block = flashbox_new_block_get(p_fbx, tag, 0);
    if (p_block == NULL) {
        HTD_FLASH_MUTEX_UNLOCK(p_fbx->p_cfg);
        return ERR_FLASH_NO_MEM;
    }
    htd_flash_sync(p_fbx->p_flash);
    print_warning("tag=%d, eidx=%d", tag, p_block->eidx);
    HTD_FLASH_MUTEX_UNLOCK(p_fbx->p_cfg);
    return ERR_FLASH_SUCCESS;
}

// @brief 添加添加多个日志
bool htd_flashbox_append_lots(flash_box_t* p_fbx, uint16_t tag, const void* p_data, uint16_t unit_len, uint8_t count)
{
    bool flag = false;
    block_info_t* p_block;
    int32_t ret = ERR_FLASH_SUCCESS;
    uint8_t buff[64];           //FIXME
    
    ASSERT(NULL != p_data);
    ASSERT(NULL != p_fbx);
    ASSERT(unit_len <= p_fbx->data_len);
    ASSERT(p_fbx->data_len <= sizeof(buff));
    if (p_fbx->init_flag != m_flashbox_init_flag) {
        return ERR_FLASH_INVALID_ARG;
    }
    
    // 获取对应的片区
    HTD_FLASH_MUTEX_LOCK(p_fbx->p_cfg);
    p_block = flashbox_new_block_get(p_fbx, tag, 0);
    if (p_block == NULL) {
        HTD_FLASH_MUTEX_UNLOCK(p_fbx->p_cfg);
        return ERR_FLASH_NO_MEM;
    }
    while (count--) {
        htd_memset(buff, 0, sizeof(buff));
        htd_memcpy(buff, p_data, unit_len);
        ret = flashbox_append(p_fbx, p_block, buff);
        if (ret != ERR_FLASH_SUCCESS) {
            break;
        }
        p_data = (uint8_t*)p_data + unit_len;
    }
    htd_flash_sync(p_fbx->p_flash);
    HTD_FLASH_MUTEX_UNLOCK(p_fbx->p_cfg);
    return ret;
}

// @brief 将一个块截短
int32_t htd_flashbox_block_trunc(flash_box_t* p_fbx, uint16_t tag, uint32_t index)
{
    block_info_t* p_prev, *p_block;
    uint16_t new_tag;
    uint8_t* p_data;
    uint32_t i;
    
    ASSERT(NULL != p_fbx);
    if (p_fbx->init_flag != m_flashbox_init_flag) {
        return ERR_FLASH_SUCCESS;
    }
    // 查找对应地址
    HTD_FLASH_MUTEX_LOCK(p_fbx->p_cfg);
    p_prev = flashbox_block_find(p_fbx, tag);
    if (p_prev == NULL || p_prev->eidx <= index) {
        HTD_FLASH_MUTEX_UNLOCK(p_fbx->p_cfg);
        return ERR_FLASH_SUCCESS;
    }

    new_tag = 0xfffe;
    while (1) {
        p_block = flashbox_block_find(p_fbx, new_tag);
        if (p_block == NULL) {
            break;
        }
        new_tag -= p_fbx->block_count;
    }
    
    p_block = flashbox_new_block_get(p_fbx, new_tag, 0);
    if (p_block == NULL) {
        HTD_FLASH_MUTEX_UNLOCK(p_fbx->p_cfg);
        return ERR_FLASH_NO_MEM;
    }
    p_prev = flashbox_block_find(p_fbx, tag);
    if (p_prev == NULL) {
        p_block->tag = 0xffff;
        HTD_FLASH_MUTEX_UNLOCK(p_fbx->p_cfg);
        return ERR_FLASH_SUCCESS;
    }
    
    // 擦除需要写入的数据页
    uint32_t waddr, wlen, length;
    wlen = index * p_fbx->data_len;
    waddr = flashbox_data_start_addr_get(p_fbx, p_block, 0);
    print_warning("wlen=%d", wlen);
    while (1) {
        length = htd_flash_page_remain_len(p_fbx->p_flash, waddr);
        if (length > wlen) {
            length = wlen;
        }
        wlen -= length; 
        if (wlen == 0) {
            break;
        }
        
        waddr = htd_flash_next_addr_get(p_fbx->p_cfg, waddr, length);
        print_warning("wlen=%d, length=%d, waddr=%d", wlen, length, waddr);
        htd_flash_erase(p_fbx->p_flash, waddr);
        flashbox_user_flag_update(p_fbx, p_block, waddr);
    }

    uint32_t start = 0, raddr, unit;
    p_data = htd_malloc(4096);
    ASSERT(p_data);
    unit = 4096 / p_fbx->data_len;
    raddr = flashbox_data_start_addr_get(p_fbx, p_prev, start);
    waddr = flashbox_data_start_addr_get(p_fbx, p_block, 0);
    while (start < index) {
        if (start + unit > index) {
            unit = index - start;
        }
        htd_memset(p_data, 0xff, p_fbx->data_len * unit);
        htd_flash_range_read(p_fbx->p_cfg, p_fbx->p_flash, raddr, p_data, p_fbx->data_len * unit);
        raddr += (p_fbx->data_len*unit);
        if (raddr >= p_fbx->p_cfg->end) {
            raddr = raddr - p_fbx->p_cfg->end + p_fbx->p_cfg->start;
        }
        htd_flash_range_write(p_fbx->p_cfg, p_fbx->p_flash, waddr, p_data, p_fbx->data_len * unit);
        waddr += (p_fbx->data_len*unit);
        if (waddr >= p_fbx->p_cfg->end) {
            waddr = waddr - p_fbx->p_cfg->end + p_fbx->p_cfg->start;
        }
        start += unit;
    }
    p_block->eidx = index;
    htd_free(p_data);

    for (i=0; i<p_fbx->block_count; ++i) {
        if (p_fbx->p_block[i].tag == tag) {
            flashbox_delete(p_fbx, i, true);
            break;
        }
    }
    for (i=0; i<p_fbx->block_count; ++i) {
        if (p_fbx->p_block[i].tag == new_tag) {
            p_fbx->p_block[i].tag = tag;
            break;
        }
    }
    ASSERT(i < p_fbx->block_count);
    htd_flash_sync(p_fbx->p_flash);
    HTD_FLASH_MUTEX_UNLOCK(p_fbx->p_cfg);
    return ERR_FLASH_SUCCESS;
}

// @brief 指定序号读取数据
int32_t htd_flashbox_get(flash_box_t* p_fbx, uint16_t tag, uint16_t index, void* p_data)
{
    uint32_t raddr;
    block_info_t* p_block;
    
    // 参数判断
    ASSERT(NULL != p_data);
    ASSERT(NULL != p_fbx);
    if (p_fbx->init_flag != m_flashbox_init_flag) {
        return ERR_FLASH_INVALID_ARG;
    }
    
    // 查找对应的片
    HTD_FLASH_MUTEX_LOCK(p_fbx->p_cfg);
    p_block = flashbox_block_find(p_fbx, tag);
    if (p_block == NULL) {
        HTD_FLASH_MUTEX_UNLOCK(p_fbx->p_cfg);
        print_warning("can't find block. tag=%d", tag);
        return ERR_FLASH_FAILED;
    }
    if (index < p_block->sidx || index >= p_block->eidx) {
        HTD_FLASH_MUTEX_UNLOCK(p_fbx->p_cfg);
        htd_memset(p_data, 0xff, p_fbx->data_len);
        print_warning("index=%d, sidx=%d, eidx=%d", index, p_block->sidx, p_block->eidx);
        return ERR_FLASH_SUCCESS;
    }
    // 读取对应区域数据
    raddr = flashbox_data_start_addr_get(p_fbx, p_block, index);
    htd_flash_range_read(p_fbx->p_cfg, p_fbx->p_flash, raddr, p_data, p_fbx->data_len);
    HTD_FLASH_MUTEX_UNLOCK(p_fbx->p_cfg);
    return ERR_FLASH_SUCCESS;
}

// @brief 获取片区最大序列号
// @return 返回0表明不存在该tag对应的片区
uint16_t htd_flashbox_max_serials_get(flash_box_t* p_fbx, uint16_t tag)
{
    uint16_t idx = 0;
    block_info_t* p_block;
    
    ASSERT(NULL != p_fbx);
    if (p_fbx->init_flag != m_flashbox_init_flag) {
        return 0;
    }

    // 查找对应地址
    HTD_FLASH_MUTEX_LOCK(p_fbx->p_cfg);
    p_block = flashbox_block_find(p_fbx, tag);
    if (p_block) {
        idx = p_block->eidx;
    }
    HTD_FLASH_MUTEX_UNLOCK(p_fbx->p_cfg);
    return idx;
}

// @brief 轮训获取标签
uint16_t htd_flashbox_tag_get(const flash_box_t* p_fbx, uint16_t tag, bool is_forward)
{
    uint16_t dst_tag = 0xffff;
    
    ASSERT(NULL != p_fbx);
    if (p_fbx->init_flag != m_flashbox_init_flag) {
        return 0xffff;
    }
    
    HTD_FLASH_MUTEX_LOCK(p_fbx->p_cfg);
    if (is_forward) {
        for (uint32_t i=0; i<p_fbx->block_count; ++i) {
            if (tag == 0xffff) {
                if (p_fbx->p_block[i].tag != 0xffff) {
                    dst_tag = p_fbx->p_block[i].tag;
                    break;
                }
            } else {
                if (tag == p_fbx->p_block[i].tag) {
                    tag = 0xffff;
                }
            }
        }
    } else {
        int32_t i = p_fbx->block_count - 1;
        for (; i>=0; --i) {
            if (tag == 0xffff) {
                if (p_fbx->p_block[i].tag != 0xffff) {
                    dst_tag = p_fbx->p_block[i].tag;
                    break;
                }
            } else {
                if (tag == p_fbx->p_block[i].tag) {
                    tag = 0xffff;
                }
            }
        }
    }
    HTD_FLASH_MUTEX_UNLOCK(p_fbx->p_cfg);
    return dst_tag;
}

// @brief 获取最后的标签
uint16_t htd_flashbox_last_tag_get(const flash_box_t* p_fbx)
{
    uint32_t i;
    uint16_t tag = 0xffff;
    
    ASSERT(NULL != p_fbx);
    if (p_fbx->init_flag != m_flashbox_init_flag) {
        return tag;
    }
    
    HTD_FLASH_MUTEX_LOCK(p_fbx->p_cfg);
    for (i=0; i<p_fbx->block_count; ++i) {
        if (p_fbx->p_block[i].tag == 0xffff) {
            break;
        }
        tag = p_fbx->p_block[i].tag;
    }
    HTD_FLASH_MUTEX_UNLOCK(p_fbx->p_cfg);
    return tag;
}

// @brief 获取最后一个数值
int32_t htd_flashbox_last_get(flash_box_t* p_fbx, uint16_t tag, void* p_data)
{
    uint32_t raddr;
    block_info_t* p_block;
    
    // 参数判断
    ASSERT(NULL != p_data);
    ASSERT(NULL != p_fbx);
    if (p_fbx->init_flag != m_flashbox_init_flag) {
        return ERR_FLASH_INVALID_ARG;
    }
    
    // 查找对应的片
    HTD_FLASH_MUTEX_LOCK(p_fbx->p_cfg);
    p_block = flashbox_block_find(p_fbx, tag);
    if (p_block == NULL) {
        HTD_FLASH_MUTEX_UNLOCK(p_fbx->p_cfg);
        print_warning("can't find block. tag=%d", tag);
        return ERR_FLASH_FAILED;
    }
    // 读取对应区域数据
    raddr = flashbox_data_start_addr_get(p_fbx, p_block, p_block->eidx - 1);
    htd_flash_range_read(p_fbx->p_cfg, p_fbx->p_flash, raddr, p_data, p_fbx->data_len);
    HTD_FLASH_MUTEX_UNLOCK(p_fbx->p_cfg);
    return ERR_FLASH_SUCCESS;
}

// @brief 是否存在某个片区
bool htd_flashbox_block_check(flash_box_t* p_fbx, uint16_t tag)
{
    block_info_t* p_block;
    
    ASSERT(NULL != p_fbx);
    if (p_fbx->init_flag != m_flashbox_init_flag) {
        return false;
    }
    
    HTD_FLASH_MUTEX_LOCK(p_fbx->p_cfg);
    p_block = flashbox_block_find(p_fbx, tag);
    HTD_FLASH_MUTEX_UNLOCK(p_fbx->p_cfg);
    return (p_block != NULL);
}

// @brief 获取某个片区的起始序列号和结束序列号
bool htd_flashbox_block_serials_get(flash_box_t* p_fbx, uint16_t tag, uint16_t* p_sidx, uint16_t* p_eidx)
{
    block_info_t* p_block;
    
    ASSERT(NULL != p_fbx);
    if (p_fbx->init_flag != m_flashbox_init_flag) {
        return false;
    }
    
    HTD_FLASH_MUTEX_LOCK(p_fbx->p_cfg);
    p_block = flashbox_block_find(p_fbx, tag);
    if (p_block) {
        if (p_sidx) {
            *p_sidx = p_block->sidx;
        }
        if (p_eidx) {
            *p_eidx = p_block->eidx - 1;
        }
    }
    HTD_FLASH_MUTEX_UNLOCK(p_fbx->p_cfg);
    return (p_block != NULL);
}

// @brief 擦除整块数据
int32_t htd_flashbox_clear(flash_box_t* p_fbx)
{
    uint32_t i;
    ASSERT(NULL != p_fbx);
    if (p_fbx->init_flag != m_flashbox_init_flag) {
        return ERR_FLASH_INVALID_ARG;
    }
    
    HTD_FLASH_MUTEX_LOCK(p_fbx->p_cfg);
    for (i=0; i<p_fbx->block_count; ++i) {
        flashbox_delete(p_fbx, i, false);
    }
    htd_flash_sync(p_fbx->p_flash);
    HTD_FLASH_MUTEX_UNLOCK(p_fbx->p_cfg);
    return ERR_FLASH_SUCCESS;
}

// @brief 擦除一片数据
void htd_flashbox_block_clear(flash_box_t* p_fbx, uint16_t tag)
{
    uint32_t i;
    
    ASSERT(NULL != p_fbx);
    if (p_fbx->init_flag != m_flashbox_init_flag) {
        return ;
    }
    
    HTD_FLASH_MUTEX_LOCK(p_fbx->p_cfg);
    for (i=0; i<p_fbx->block_count; ++i) {
        if (p_fbx->p_block[i].tag == tag) {
            flashbox_delete(p_fbx, i, true);
            break;
        }
    }
    htd_flash_sync(p_fbx->p_flash);
    HTD_FLASH_MUTEX_UNLOCK(p_fbx->p_cfg);
}

// @brief 获取用户数据
int32_t htd_flashbox_user_data_read(flash_box_t* p_fbx, uint16_t tag, uint32_t offset, void* p_data, uint16_t data_size)
{
    int32_t ret = 0;
    uint32_t addr;
    block_info_t* p_block;
    
    ASSERT(p_fbx);
    ASSERT(p_fbx->user_data_len > 0);
    ASSERT(p_data);
    ASSERT(data_size + offset <= p_fbx->user_data_len);
    if (p_fbx->init_flag != m_flashbox_init_flag) {
        return ret;
    }
    
    HTD_FLASH_MUTEX_LOCK(p_fbx->p_cfg);
    p_block = flashbox_block_find(p_fbx, tag);
    if (p_block) {
        addr = flashbox_addr_decode(p_fbx->p_cfg, p_fbx->p_flash, p_block->addr);
        addr += sizeof(block_head_t) + offset;
        ret = htd_flash_read(p_fbx->p_flash, addr, p_data, data_size);
        if (ret == ERR_FLASH_SUCCESS) {
            ret = data_size;
        }
    }
    HTD_FLASH_MUTEX_UNLOCK(p_fbx->p_cfg);
    return ret;
}

// @brief 写入用户数据
int32_t htd_flashbox_user_data_write(flash_box_t* p_fbx, uint16_t tag, uint32_t offset, const void* p_data, uint16_t data_len)
{
    int32_t ret = 0;
    uint32_t addr;
    block_info_t* p_block;
    uint32_t remain_len;
    
    ASSERT(p_fbx);
    ASSERT(p_data);
    ASSERT(data_len + offset <= p_fbx->user_data_len);
    if (p_fbx->init_flag != m_flashbox_init_flag) {
        return ERR_FLASH_INVALID_ARG;
    }
   
    HTD_FLASH_MUTEX_LOCK(p_fbx->p_cfg);
    p_block = flashbox_block_find(p_fbx, tag);
    if (p_block && data_len) {
        addr = flashbox_addr_decode(p_fbx->p_cfg, p_fbx->p_flash, p_block->addr);
        addr += sizeof(block_head_t) + offset;
        remain_len = p_fbx->user_data_len - offset;
        
        uint8_t* p_buff = htd_malloc(remain_len);
        ASSERT(p_buff);
        htd_memset(p_buff, 0, remain_len);
        htd_flash_read(p_fbx->p_flash, addr, p_buff, remain_len);
        if (htd_flash_mem_allof(p_buff, 0xff, data_len)) {
            ret = htd_flash_write(p_fbx->p_flash, addr, p_data, data_len);
            if (ret == ERR_FLASH_SUCCESS) {
                ret = data_len;
            }
            htd_flash_sync(p_fbx->p_flash);
        }
        htd_free(p_buff);
    }
    HTD_FLASH_MUTEX_UNLOCK(p_fbx->p_cfg);
    return ret;
}


// @brief 打印所有片区信息
void htd_flashbox_block_info_print(flash_box_t* p_fbx)
{
    uint32_t i;
    block_info_t* p_block;
    
    ASSERT(NULL != p_fbx);
    if (p_fbx->init_flag != m_flashbox_init_flag) {
        return ;
    }
    
    print_debug("------------------------------------------------------");
    HTD_FLASH_MUTEX_LOCK(p_fbx->p_cfg);
    print_debug("ftlb info: start=%08x, end=%08x, data_len=%d, data_count=%d", 
        p_fbx->p_cfg->start, p_fbx->p_cfg->end, p_fbx->data_len, p_fbx->data_count);
    print_debug("           block_len=%d", p_fbx->block_len);
    for (i=0; i<p_fbx->block_count; ++i) {
        p_block = &p_fbx->p_block[i];
        if (p_block->tag == 0xffff) {
            break;
        }
        print_debug("block[%p]: addr=%08x, tag=%u, sidx=%3d, eidx=%4d, serials=%u", p_block, 
                  flashbox_addr_decode(p_fbx->p_cfg, p_fbx->p_flash, p_block->addr), p_block->tag, p_block->sidx, p_block->eidx, p_block->serials);
    }
    HTD_FLASH_MUTEX_UNLOCK(p_fbx->p_cfg);
    print_debug("------------------------------------------------------");
}

// @brief 连续读取多个数据
// @param pos 偏移几个字节, len 获取几个字节
int32_t htd_flashbox_get_lots_continuity(flash_box_t* p_fbx, uint16_t tag, uint32_t index, uint8_t* p_data, uint8_t pos, uint8_t len, uint32_t* p_count, bool* p_is_end)
{
    block_info_t* p_block;
    uint32_t raddr;
    uint8_t buff[256];
    uint8_t* p_buff;
    uint32_t read_count, i, unit, real_cnt;
    int32_t ret = ERR_FLASH_SUCCESS;
    
    // 参数判断
    ASSERT(NULL != p_fbx);
    ASSERT(NULL != p_data);
    ASSERT(NULL != p_count);
    if (p_fbx->init_flag != m_flashbox_init_flag) {
        return ERR_FLASH_INVALID_ARG;
    }
    
    read_count = *p_count;
    *p_count = 0;
    HTD_FLASH_MUTEX_LOCK(p_fbx->p_cfg);
    if (pos + len > p_fbx->data_len) {
        HTD_FLASH_MUTEX_UNLOCK(p_fbx->p_cfg);
        print_error("pos=%d, len=%d, data_len=%d", pos, len, p_fbx->data_len);
        return ERR_FLASH_INVALID_ARG;
    }
    // 查找对应地址
    p_block = flashbox_block_find(p_fbx, tag);
    if (p_block == NULL) {
        HTD_FLASH_MUTEX_UNLOCK(p_fbx->p_cfg);
        print_warning("can't find start addr. tag=%d", tag);
        return ERR_FLASH_FAILED;
    }
    
    // 计算对应区域位置
    if (index + read_count >= p_block->eidx) {
        read_count = p_block->eidx - index;
    }
    if (read_count <= 0) {
        if (p_is_end) {
            *p_is_end = true;
        }
        HTD_FLASH_MUTEX_UNLOCK(p_fbx->p_cfg);
        return ERR_FLASH_SUCCESS;
    }
    //print_debug("read_count=%d, sidx=%d, eidx=%d, index=%d, data_len=%d", read_count, p_block->sidx, p_block->eidx, index, p_fbx->data_len);
    
    // 读取数据
    real_cnt = 0;
    raddr = flashbox_data_start_addr_get(p_fbx, p_block, index);
    while (read_count) {
        unit = sizeof(buff) / p_fbx->data_len;
        if (read_count < unit) {
            unit = read_count;
        }
        htd_memset(buff, 0xff, sizeof(buff));
        if (index >= p_block->sidx) {
            //print_debug("raddr=%08x, unit=%d, %d, sizeof(buff)=%d", raddr, unit, p_fbx->data_len*unit, sizeof(buff));
            htd_flash_range_read(p_fbx->p_cfg, p_fbx->p_flash, raddr, buff, p_fbx->data_len*unit);
        } else {
            if (p_block->sidx - index < unit) {
                unit = p_block->sidx - index;
            }
        }
        
        p_buff = buff;
        for (i=0; i<unit; ++i) {
            htd_memcpy(p_data, &p_buff[pos], len);
            p_data += len;
            p_buff += p_fbx->data_len;
        }
        raddr += (p_fbx->data_len*unit);
        if (raddr >= p_fbx->p_cfg->end) {
            raddr = raddr - p_fbx->p_cfg->end + p_fbx->p_cfg->start;
        }
        read_count -= unit;
        index += unit;
        real_cnt += unit;
        //print_debug("unit=%d, read_count=%d, index=%d, len=%d", unit, read_count, index, len);
    }

    *p_count = real_cnt;
    if (p_is_end) {
        *p_is_end = (index+1 >= p_block->eidx) ? true : false;
    }
    print_debug("real_cnt=%d, start=%d, end=%d, index=%u", real_cnt, p_block->sidx, p_block->eidx, index);

    HTD_FLASH_MUTEX_UNLOCK(p_fbx->p_cfg);
    return ret;
}

// @brief 计算一个块有多大
uint32_t htd_flashbox_block_size_get(uint32_t data_size, uint32_t user_size, uint32_t erase_unit)
{
    erase_unit--;
    data_size += user_size + sizeof(block_head_t);
    data_size += erase_unit;
    data_size &= ~erase_unit;
    return data_size;
}

// @brief 增加一整片的数据(一条一条添加，对内存要求较低，速度较慢)
int32_t htd_flashbox_add_block_data(flash_box_t* p_fbx, uint16_t tag, const void* p_data, int32_t data_len, uint8_t pos, uint8_t len)
{
    block_info_t* p_block;
    uint8_t* p_fdata;
    int32_t ret = ERR_FLASH_SUCCESS;

    ASSERT(p_fbx);
    ASSERT(p_data && data_len > len);
    if (p_fbx->init_flag != m_flashbox_init_flag) {
        return ERR_FLASH_INVALID_ARG;
    }

    if (pos + len > p_fbx->data_len) {
        return ERR_FLASH_INVALID_ARG;
    }
    p_fdata = htd_malloc(p_fbx->data_len);
    ASSERT(p_fdata);
    htd_memset(p_fdata, 0xff, p_fbx->data_len);

    htd_flashbox_block_clear(p_fbx, tag);
    HTD_FLASH_MUTEX_LOCK(p_fbx->p_cfg);
    p_block = flashbox_new_block_get(p_fbx, tag, 0);
    while (data_len > 0) {
        htd_memcpy(&p_fdata[pos], p_data, len);
        ret = flashbox_insert(p_fbx, p_block, p_block->eidx, p_fdata);
        if (ERR_FLASH_SUCCESS != ret) {
            print_error("ftlb write data failed. ret=%d", ret);
            break;
        }
        data_len -= len;
        p_data = (const void*)((uint8_t*)p_data + len);
    }
    htd_flash_sync(p_fbx->p_flash);
    HTD_FLASH_MUTEX_UNLOCK(p_fbx->p_cfg);
    
    htd_free(p_fdata);
    return ret;
}

// @brief 快速增加一整片的数据(p_data 数据已经整理好，一次性写入，速度较快)
// @node 未做压测
int32_t htd_flashbox_fast_add_block_data(flash_box_t* p_fbx, uint16_t tag, const void* p_data, int32_t data_len)
{
    block_info_t* p_block;
    uint32_t addr;
    uint32_t length = 0;

    ASSERT(p_fbx);
    ASSERT(p_data && data_len >= p_fbx->data_len);
    if (p_fbx->init_flag != m_flashbox_init_flag) {
        return ERR_FLASH_INVALID_ARG;
    }

    uint32_t wlen = data_len;
    htd_flashbox_block_clear(p_fbx, tag);
    HTD_FLASH_MUTEX_LOCK(p_fbx->p_cfg);
    p_block = flashbox_new_block_get(p_fbx, tag, 0);
    // 擦除需要写入的数据区间
    addr = flashbox_data_start_addr_get(p_fbx, p_block, 0);
    while (1) {
        length = htd_flash_page_remain_len(p_fbx->p_flash, addr);
        if (length > wlen) {
            length = wlen;
        }
        wlen -= length; 
        if (wlen == 0) {
            break;
        }
        addr += length;
        htd_flash_erase(p_fbx->p_flash, addr);
        flashbox_user_flag_update(p_fbx, p_block, addr);
    }
    
    // 写入整块数据
    wlen = data_len;
    addr = flashbox_data_start_addr_get(p_fbx, p_block, 0);
    p_block->eidx = wlen / p_fbx->data_len;
    while (wlen) {
        length = htd_flash_page_remain_len(p_fbx->p_flash, addr);
        if (wlen < length) {
            length = wlen;
        }
        htd_flash_write(p_fbx->p_flash, addr, p_data, length);
        wlen -= length;
        p_data = (const void*)((uint8_t*)p_data + length);
        addr = htd_flash_next_page_addr_get(p_fbx->p_cfg, p_fbx->p_flash, addr);
    }
    htd_flash_sync(p_fbx->p_flash);
    HTD_FLASH_MUTEX_UNLOCK(p_fbx->p_cfg);
    return ERR_FLASH_SUCCESS;
}

// @brief 读取多个数据(可以间隔的读)
// @param inter 间隔多长, pos 偏移几个字节, len 获取几个字节
// @node 未做压测
int32_t htd_flashbox_get_lots(flash_box_t* p_fbx, uint16_t tag, uint16_t index, uint8_t* p_data, uint32_t ftlb_info, uint32_t* p_count, bool* p_is_end)
{
    block_info_t* p_block;
    uint32_t raddr;
    uint8_t buff[FTL_DATA_VAL_MAX_LEN];
    uint32_t read_count, i;
    int32_t ret = ERR_FLASH_SUCCESS;
    uint16_t real_cnt;
    uint8_t inter, pos, len;

    ASSERT(p_fbx);
    ASSERT(p_data && p_count);
    if (p_fbx->init_flag != m_flashbox_init_flag) {
        return ERR_FLASH_INVALID_ARG;
    }

    read_count = *p_count;
    *p_count = 0;
    // 参数判断
    inter = (ftlb_info >> 16) & 0xff;
    pos = (ftlb_info >> 8) & 0xff;
    len = ftlb_info & 0xff;
    if (inter == 0) {
        print_error("inter=%d", inter);
        return ERR_FLASH_INVALID_ARG;
    }

    HTD_FLASH_MUTEX_LOCK(p_fbx->p_cfg);
    if (pos + len > p_fbx->data_len) {
        HTD_FLASH_MUTEX_UNLOCK(p_fbx->p_cfg);
        print_error("pos=%d, len=%d, data_len=%d", pos, len, p_fbx->data_len);
        return ERR_FLASH_INVALID_ARG;
    }

    // 查找对应地址
    p_block = flashbox_block_find(p_fbx, tag);
    if (p_block == NULL) {
        HTD_FLASH_MUTEX_UNLOCK(p_fbx->p_cfg);
        print_error("can't find start addr.");
        return ERR_FLASH_FAILED;
    }

    // 读取数据
    real_cnt = 0;
    raddr = flashbox_data_start_addr_get(p_fbx, p_block, index);
    for (i=0; i<read_count; i++) {
        htd_memset(buff, 0xff, sizeof(buff));
        if (index >= p_block->eidx) {
            break;
        }
        if (index >= p_block->sidx) {
            ret = htd_flash_read(p_fbx->p_flash, raddr, buff, p_fbx->data_len);
            if (ret != ERR_FLASH_SUCCESS) {
                print_error("ftl data read failed. ret=%d, raddr=%08x", ret, raddr);
                break;
            }
            //TODO: 这里可以进行解密
        }
        htd_memcpy(p_data, &buff[pos], len);
        p_data += len;
        raddr += p_fbx->data_len * inter;
        index += inter;
        real_cnt++;
    }
    *p_count = real_cnt;
    if (p_is_end) {
        *p_is_end = (index+1 >= p_block->eidx) ? true : false;
    }
    print_debug("real_cnt=%d, start=%d, end=%d, index=%d, read_count=%d", real_cnt, p_block->sidx, p_block->eidx, index, read_count);
    HTD_FLASH_MUTEX_UNLOCK(p_fbx->p_cfg);
    return ret;
}




/////////////////////////////////////////////////////////////////////////////
#if (1 && defined(DEBUG_LVL) && (DEBUG_LVL >= 4) && defined(HTD_CONSOLE_ENABLE) && defined(HTD_USING_NORFLASH))
#include "htd_dev_norflash.h"
#include "htd_console_process.h"


static flash_box_t m_flashbox_object;
static block_info_t m_flashbox_slice[7];
static uint32_t m_flashbox_user_data_len = 4096+1024; //1024; //4096+1024;
static uint32_t m_flashbox_block_len = 4096*4;
const static flash_config_t m_flashbox_config = {
    .p_name = "norflash_4k",
    .start = NORFLASH_MAP_START_ADDR,
    .end = NORFLASH_MAP_START_ADDR + 1024 * (12 * 7),
    .lock_handle = NULL,
    .unlock_handle = NULL,
};


typedef struct {
    uint32_t val;
} test_item_t;

static uint16_t last_tag = 0xffff;
static uint16_t last_index = 0xffff;
static test_item_t last_val;
static void flash_box_cache_safe(uint16_t tag, uint16_t index, test_item_t val)
{
    last_tag = tag;
    last_index = index;
    last_val = val;
}
static void flash_box_value_test(void)
{
    if (last_tag == 0xffff) {
        return;
    }

    uint16_t tag = htd_flashbox_last_tag_get(&m_flashbox_object);
    if (tag != last_tag) {
        print_error("test failed. last_tag=%d, tag=%d", last_tag, tag);
        ASSERT(0);
        return;
    }
    uint16_t index = htd_flashbox_max_serials_get(&m_flashbox_object, tag);
    if (index != last_index + 1) {
        print_error("test failed. last_index=%d, index=%d", last_index, index);
        ASSERT(0);
        return;
    }
    test_item_t val;
    int32_t ret = htd_flashbox_get(&m_flashbox_object, tag, last_index, &val);
    if (htd_memcmp(&val, &last_val, sizeof(val)) != 0) {
        print_error("test failed. tag=%d, index=%d, last_val=%d, val=%d, ret=%d", tag, index, last_val.val, val.val, ret);
        ASSERT(0);
        return;
    }
}

// @brief 固定长度块测试
void flash_box_fix_size_test(void)
{
    // 初始化
    int32_t ret;
    FLASH_BOX_DEFAULT(&m_flashbox_object, m_flashbox_block_len, sizeof(test_item_t), 1, m_flashbox_user_data_len);
    ret = htd_flashbox_init(&m_flashbox_object, &m_flashbox_config, m_flashbox_slice, ARRAY_COUNT(m_flashbox_slice));
    ASSERT(ret == ERR_FLASH_SUCCESS);
    flash_box_value_test();

    // 随机增加一个tag
    uint16_t tag;
    while (1) {
        tag = htd_rng_val_get() & 0xffff;
        if (tag != 0xffff && false == htd_flashbox_block_check(&m_flashbox_object, tag)) {
            break;
        }
    }

    // 添加数据
    uint32_t max_serials = (m_flashbox_block_len - m_flashbox_user_data_len - sizeof(block_head_t)) / sizeof(test_item_t);
    uint16_t index = htd_rng_val_get() % max_serials;
    test_item_t val;
    while (1) {
        val.val = index;
        flash_box_cache_safe(tag, index, val);
        ret = htd_flashbox_add(&m_flashbox_object, tag, index, &val);
        ASSERT(ret == ERR_FLASH_SUCCESS);
        
        htd_memset(&val, 0xff, sizeof(val));
        ret = htd_flashbox_get(&m_flashbox_object, tag, index, &val);
        ASSERT(ret == ERR_FLASH_SUCCESS);
        ASSERT(val.val == index);

        index += 1 + (htd_rng_val_get() % 100);
        if (index >= max_serials) {
            break;
        }
    }
    //flash_box_uninit(&m_flashbox_object);
}

// @brief 非固定长度块测试
void flash_box_nonfix_test(bool flag)
{
    // 初始化
    int32_t ret;
    FLASH_BOX_DEFAULT(&m_flashbox_object, 4096*3, sizeof(test_item_t), 0, m_flashbox_user_data_len);
    ret = htd_flashbox_init(&m_flashbox_object, &m_flashbox_config, m_flashbox_slice, ARRAY_COUNT(m_flashbox_slice));
    ASSERT(ret == ERR_FLASH_SUCCESS);

#if 1
    if (flag) {
        flash_box_value_test();
    }

    // 随机增加一个tag
    uint16_t tag;
    while (1) {
        tag = htd_rng_val_get() & 0xffff;
        if (tag != 0xffff && false == htd_flashbox_block_check(&m_flashbox_object, tag)) {
            break;
        }
    }

    // 添加数据
    uint32_t max_serials = (m_flashbox_block_len - m_flashbox_user_data_len - sizeof(block_head_t)) / sizeof(test_item_t);
    uint16_t index = 0;
    test_item_t val;
    uint32_t count = htd_rng_val_get() % (max_serials / 2);
    print_debug("max_serials=%d, count=%d", max_serials, count);
    while (count--) {
        val.val = index;
        flash_box_cache_safe(tag, index, val);
        ret = htd_flashbox_append(&m_flashbox_object, tag, &val);
        ASSERT(ret == ERR_FLASH_SUCCESS);
        
        htd_memset(&val, 0xff, sizeof(val));
        ret = htd_flashbox_get(&m_flashbox_object, tag, index, &val);
        ASSERT(ret == ERR_FLASH_SUCCESS);
        ASSERT(val.val == index);
        index++;
        if (index >= max_serials) {
            break;
        }
    }
#endif
}

// @brief 截断测试
void flash_box_trunc_test(void)
{
    int32_t ret;
    test_item_t val;
    uint16_t tag;
    uint32_t i;
    uint32_t last_idx;

    flash_box_nonfix_test(false);
    tag = htd_flashbox_last_tag_get(&m_flashbox_object);
    last_idx = htd_flashbox_max_serials_get(&m_flashbox_object, tag);
    print_warning("tag=%d, last_idx=%d, trunc=%d", tag, last_idx, last_idx / 2);
    htd_flashbox_block_trunc(&m_flashbox_object, tag, last_idx / 2);

    last_idx = htd_flashbox_max_serials_get(&m_flashbox_object, tag);
    print_warning("last_idx=%d, tag=%d, tag=%p, val=%p", last_idx, tag, &tag, &val);
    for (i=0; i<last_idx; ++i) {
        htd_memset(&val, 0xff, sizeof(val));
        ret = htd_flashbox_get(&m_flashbox_object, tag, i, &val);
        ASSERT(ret == ERR_FLASH_SUCCESS);
        ASSERT(val.val == i);
    }
    return;
}

int flashbox_test_cmd(int argc, void** argv)
{
    htd_memset(&last_val, 0xff, sizeof(last_val));
#if 0
    for (uint32_t i=0; i<10000; ++i) {
        flash_box_trunc_test();
    }
#endif
#if 1
    for (uint32_t i=0; i<10000; ++i) {
        flash_box_nonfix_test(true);
        //flash_box_fix_size_test();
        htd_flashbox_block_info_print(&m_flashbox_object);
        print_debug("\r\n\r\n\r\n");
    }
#else
    flash_box_nonfix_test(true);
#endif

    print_warning("test finish");
    return 0;
}
CONSOLE_CMD_EXPORT(flashbox_test_cmd, flashbox_test);
#endif
#endif /*defined(HTD_FLASH_KV_ENABLE)*/

