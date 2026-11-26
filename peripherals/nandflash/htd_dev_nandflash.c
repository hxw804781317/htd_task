// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL       4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG       "nand"
#include "htd_device.h"
#include "htd_components.h"
#include "htd_dev_nandflash.h"


//////////////////////////////////////////////////////////////////////
#define BBM_BLOCK_BAD_FLAG                  0xfffe
#define BBM_BLOCK_UNUSE_FLAG                0xffff
#define NANDFLASH_BBM_MAGIC_VAL             0xab5a8c3e
typedef struct {
    uint32_t magic;
    const dev_nandflash_t* p_flash;
    uint32_t start_block_id;              // bbt 起始id
    uint32_t block_count;                 // bbt有效数量
    uint16_t bbt[128];                    // 128 可以根据 block_count 适当调整，只要比 block_count 大即可; 
    uint32_t checkval;
    
    uint8_t* p_cache;
    const uint8_t* p_page_data;
    uint16_t page_idx;
} nandflash_bbm_t;

// @brief 显示bbm信息
static void nandflash_bbm_info_show(nandflash_bbm_t* p_bbm)
{
#if defined(DEBUG_LVL)
    print_log("magic=%x", p_bbm->magic);
    print_log("bbm start_block: %d, count=%d", p_bbm->start_block_id, p_bbm->block_count);
    for (uint32_t i=0; i<ARRAY_COUNT(p_bbm->bbt); ++i) {
        if (p_bbm->bbt[i] == BBM_BLOCK_BAD_FLAG) {
            print_log("bad block. %d", p_bbm->start_block_id+i);
        } else if (p_bbm->bbt[i] != BBM_BLOCK_UNUSE_FLAG) {
            print_log("map block id: %d->%d", p_bbm->bbt[i], p_bbm->start_block_id+i);
        } 
    }
#endif 
}

// @brief 标记一个坏块
static int32_t nandflash_bad_mark(nandflash_bbm_t* p_bbm, uint32_t block_id)
{
    const dev_nandflash_t* p_flash = p_bbm->p_flash;
    p_flash->ctrl_handle(NANDFLASH_CMD_ERASE, &block_id, sizeof(uint32_t));

    // 标记为坏块，oob最后4个字节写入0xbd,
    uint8_t* p_spare = &p_bbm->p_cache[p_flash->page_data_size];
    memset(p_spare, 0xff, p_flash->page_spare_size);
    uint32_t offset = p_flash->page_spare_size - 4;
    memset(&p_spare[offset], 0xbd, 4);
    p_flash->page_write_handle(block_id * p_flash->block_size, NULL, 0, p_spare, p_flash->page_spare_size);
    print_debug("block mark. block_id=%lu, mark_page_id=%lu", block_id, block_id * p_flash->block_size);
    return 0;
}

// @brief 读取逻辑块id，没有返回0xffffffff
uint32_t nandflash_map_block_id_get(nandflash_bbm_t* p_bbm, uint32_t block_id, uint8_t *is_bad)
{
    const dev_nandflash_t* p_flash = p_bbm->p_flash;
    uint8_t* p_spare = &p_bbm->p_cache[p_flash->page_data_size];
    memset(p_spare, 0xff, p_flash->page_spare_size);
    uint32_t page_id = block_id * p_flash->block_size;
    p_flash->page_read_handle(page_id, 0, NULL, 0, p_spare, p_flash->page_spare_size);

    // 判断是否为坏块，oob最后4个字节写入0xbd, 只要有一个字节为0xbd, 可认为是坏块
    uint32_t i, j;
    uint32_t offset = p_flash->page_spare_size - 4;
    for (i=0; i<4; i++) {
        if (p_spare[offset+i] == 0xbd) {
            if (is_bad) {
                *is_bad = true;
            }
            return 0xffffffff;
        }
    }

    uint32_t ids[8];
    uint32_t ids_count = ARRAY_COUNT(ids);
    memset(ids, 0xff, sizeof(ids));
    offset = p_flash->ecc_size;
    ids[0] = *(uint32_t*)&p_spare[offset];
    if (ids[0] == 0xffffffff) {
        return 0xffffffff;
    }
    if (ids_count > p_flash->block_size) {
        ids_count = p_flash->block_size;
    }
    for (i=1; i<ids_count; ++i) {
        page_id++;
        p_flash->page_read_handle(page_id, 0, NULL, 0, p_spare, p_flash->page_spare_size);
        ids[i] = *(uint32_t*)&p_spare[offset];
        if (ids[i] == ids[0]) {
            // 有和第一个一样的，说明第一个的值是可靠的
            return ids[0];
        }
    }
    // 只要有两个相同且非0xffffffff，就可以
    for (i=1; i<ids_count; ++i) {
        if (ids[i] == 0xffffffff) {
            continue;
        }
        for (j=i+1; j<ids_count; ++j) {
            if (ids[i] == ids[j]) {
                return ids[i];
            }
        }
    }
    return 0xffffffff;
}

// @brief 拷贝整块数据
static int32_t nandflash_block_copy(nandflash_bbm_t* p_bbm, uint32_t dst, uint32_t src)
{
    // 擦除 dst
    const dev_nandflash_t* p_flash = p_bbm->p_flash;
    int32_t ret = p_flash->ctrl_handle(NANDFLASH_CMD_ERASE, &dst, sizeof(uint32_t));
    if (0 != ret) {
        return -1;
    }
    
    if (p_bbm->p_page_data == NULL) {
        return 0;
    }

    // 拷贝
    uint8_t has_data;
    uint8_t* p_spare = &p_bbm->p_cache[p_flash->page_data_size];
    uint32_t dst_page_id = dst * p_flash->block_size;
    uint32_t src_page_id = src * p_flash->block_size;
    for (uint32_t i=0; i<p_flash->block_size; ++i) {
        has_data = true;
        memset(p_spare, 0xff, p_flash->page_spare_size);
        if (i == p_bbm->page_idx) {
            memcpy(p_bbm->p_cache, p_bbm->p_page_data, p_flash->page_data_size);
            p_flash->ecc_generate_handle(p_bbm->p_cache, p_flash->page_data_size, p_spare);
        } else {
            ret = p_flash->page_read_handle(src_page_id, 0, p_bbm->p_cache, p_flash->page_data_size, p_spare, p_flash->ecc_size);
            if (htd_mem_allof(p_spare, 0xff, p_flash->ecc_size)) {
                has_data = false;
            }
        }
        if (has_data) {
            // 同时写入映射块id
            memcpy(&p_spare[p_flash->ecc_size], &src, sizeof(uint32_t));
            ret = p_flash->page_write_handle(dst_page_id, p_bbm->p_cache, p_flash->page_data_size, p_spare, p_flash->page_spare_size);
            if (ret < 0) {
                print_error("page write failed. ret=%d", ret);
                return -1;
            }
        }
        dst_page_id++;
        src_page_id++;
    }
    return 0;
}

// @brief 获取校验值
static void bbm_checkval_update(nandflash_bbm_t* p_bbm)
{
    uint8_t *p_start = (uint8_t*)p_bbm;
    uint8_t *p_end = (uint8_t*)&p_bbm->checkval;

    htd_crc32_ctx_t ctx;
    ctx.crc32_type = HTD_CRC32_GEN;
    ctx.result = 0xffffffff;
    htd_crc32_init(&ctx);
    htd_crc32_update(&ctx, p_start, p_end - p_start);
    p_bbm->checkval = htd_crc32_final(&ctx);
}

// @brief 获取未使用块
static int32_t bbm_unused_idx_get(nandflash_bbm_t* p_bbm)
{
    for (int32_t i=0; i<ARRAY_COUNT(p_bbm->bbt); ++i) {
        if (p_bbm->bbt[i] == BBM_BLOCK_UNUSE_FLAG) {
            return i;
        }
    }
    ASSERT("unused idx get failed.");
    return -1;
}

// @brief 初始化
static int32_t bbm_init(const dev_nandflash_t* p_flash, nandflash_bbm_t* p_bbm, uint32_t start_block_id, uint32_t block_count)
{
    if (p_bbm->magic == NANDFLASH_BBM_MAGIC_VAL) {
        return 0;
    }
    if (start_block_id + block_count > BBM_BLOCK_BAD_FLAG) {
        print_error("bbt bad_flag/unused_flag error. start_block_id+block_count=%d", start_block_id + block_count);
        ASSERT(0);
        return 0;
    }
    ASSERT(block_count <= ARRAY_COUNT(p_bbm->bbt));
    
    memset(p_bbm, 0, sizeof(nandflash_bbm_t));
    memset(p_bbm->bbt, 0xff, sizeof(p_bbm->bbt));
    p_bbm->magic = NANDFLASH_BBM_MAGIC_VAL;
    p_bbm->p_flash = p_flash;
    p_bbm->start_block_id = start_block_id;
    p_bbm->block_count = block_count;
    p_bbm->p_cache = htd_malloc(p_flash->page_data_size + p_flash->page_spare_size);
    ASSERT(p_bbm->p_cache);

    // 扫描
    uint8_t is_bad;
    uint32_t map_block_id;
    uint32_t i;
    for (i=0; i<block_count; ++i) {
        is_bad = false;
        map_block_id = nandflash_map_block_id_get(p_bbm, start_block_id + i, &is_bad);
        print_debug("block map. %u->%u, is_bad=%d", start_block_id + i, map_block_id, is_bad);
        if (is_bad) {
            p_bbm->bbt[i] = BBM_BLOCK_BAD_FLAG;
        } else {
            p_bbm->bbt[i] = map_block_id;
        }
    }
    bbm_checkval_update(p_bbm);
    return 0;
}

// @brief 获取物理id
static int32_t bbm_map_block_id_get(nandflash_bbm_t* p_bbm, uint32_t block_id)
{
    ASSERT(p_bbm->magic == NANDFLASH_BBM_MAGIC_VAL);
    ASSERT(block_id != BBM_BLOCK_UNUSE_FLAG && block_id != BBM_BLOCK_BAD_FLAG);
    
    for (uint32_t i=0; i<ARRAY_COUNT(p_bbm->bbt); ++i) {
        if (p_bbm->bbt[i] == block_id) {
            return p_bbm->start_block_id + i;
        }
    }
    return block_id;
}

// @brief 块映射, p_bbm->p_page_data 为空
static int32_t bbm_block_map(nandflash_bbm_t* p_bbm, uint32_t block_id, uint32_t page_id, const uint8_t* p_page_data)
{
    uint32_t new_block_id;
    int32_t idx, new_idx;
    ASSERT(block_id != BBM_BLOCK_UNUSE_FLAG && block_id != BBM_BLOCK_BAD_FLAG);
    
    // 该块为已经映射的块
    p_bbm->p_page_data = p_page_data;
    p_bbm->page_idx = page_id % p_bbm->p_flash->block_size;
    if (block_id >= p_bbm->start_block_id && block_id < p_bbm->start_block_id + p_bbm->block_count) {
        idx = block_id - p_bbm->start_block_id;
        if (p_bbm->bbt[idx] == BBM_BLOCK_BAD_FLAG || p_bbm->bbt[idx] == BBM_BLOCK_UNUSE_FLAG) {
            // 状态错误
            print_error("error status. bbt[%d]=0x%04x", idx, p_bbm->bbt[idx]);
            ASSERT(0);
        }
        
        // 获取新的未使用块
        new_idx = bbm_unused_idx_get(p_bbm);
        ASSERT(new_idx < (int32_t)p_bbm->block_count);
        new_block_id = p_bbm->start_block_id + new_idx;
        // 拷贝原有的数据, 并标记好映射的块
        if (0 != nandflash_block_copy(p_bbm, new_block_id, block_id)) {
            p_bbm->bbt[new_idx] = BBM_BLOCK_BAD_FLAG;
            nandflash_bad_mark(p_bbm, new_block_id);
            bbm_checkval_update(p_bbm);
            return -1;
        }
        print_debug("bbm remap. %d->%d", p_bbm->bbt[idx], new_block_id);
        p_bbm->bbt[new_idx] = p_bbm->bbt[idx];
        // 原有的标记为坏块
        nandflash_bad_mark(p_bbm, block_id);
        p_bbm->bbt[idx] = BBM_BLOCK_BAD_FLAG;
        bbm_checkval_update(p_bbm);
        return 0;
    }
    
    // 获取新的未使用块
    new_idx = bbm_unused_idx_get(p_bbm);
    ASSERT(new_idx < (int32_t)p_bbm->block_count);
    new_block_id = p_bbm->start_block_id + new_idx;
    // 拷贝原有的数据, 并标记好映射的块
    if (0 != nandflash_block_copy(p_bbm, new_block_id, block_id)) {
        nandflash_bad_mark(p_bbm, new_block_id);
        p_bbm->bbt[new_idx] = BBM_BLOCK_BAD_FLAG;
        bbm_checkval_update(p_bbm);
        return -1;
    }
    
    print_debug("bbm map. %d->%d", block_id, new_block_id);
    p_bbm->bbt[new_idx] = block_id;
    bbm_checkval_update(p_bbm);
    return 0;
}

// @brief 获取映射的page_id
static uint32_t bbm_map_page_id_get(nandflash_bbm_t* p_bbm, uint32_t page_id)
{
    const dev_nandflash_t* p_flash = p_bbm->p_flash;
    uint32_t block_id = page_id / p_flash->block_size;
    uint32_t map_block_id = bbm_map_block_id_get(p_bbm, block_id);
    uint32_t map_page_id = page_id % p_flash->block_size + map_block_id * p_flash->block_size;
    return map_page_id;
}

// @brief 映射写入
static void bbm_page_map_and_write(nandflash_bbm_t* p_bbm, const uint8_t* p_page_data, uint32_t page_id)
{
    const dev_nandflash_t* p_flash = p_bbm->p_flash;
    uint32_t block_id = page_id / p_flash->block_size;
    for (int i=0; ;++i) {
        if (i > 3) {
            print_error("bbm_page_map_and_write failed.");
            ASSERT(0);
        }
        if (0 == bbm_block_map(p_bbm, block_id, page_id, p_page_data)) {
            break;
        }
    }
    p_bbm->p_page_data = NULL;
}






//////////////////////////////////////////////////////////////////////
static nandflash_bbm_t m_bbm;
static const dev_nandflash_t *m_nandflash = NULL;
static htd_mutex_lock_t* m_nandflash_lock = NULL;
#define nandflash_dev_lock_init()           do { if (m_nandflash_lock == NULL) {m_nandflash_lock = htd_mutex_lock_create("nandflash"); ASSERT(m_nandflash_lock);} }while(0)
#define nandflash_dev_lock()                do { htd_mutex_lock(m_nandflash_lock);} while(0)
#define nandflash_dev_unlock()              do { htd_mutex_unlock(m_nandflash_lock);} while(0)


// @brief 注册(只能调用一次)
int32_t htd_dev_nandflash_register(const dev_nandflash_t* p_nandflash)
{
    ASSERT(m_nandflash == NULL);
    ASSERT(p_nandflash);
    ASSERT(p_nandflash->page_read_handle);
    ASSERT(p_nandflash->page_write_handle);
    ASSERT(p_nandflash->ctrl_handle);
    ASSERT(p_nandflash->init_handle);
    
    m_nandflash = p_nandflash;
    return 0;
}

// @brief 按页读数据
int32_t htd_dev_nandflash_page_read(uint32_t page_id, uint32_t offset, uint8_t* p_data, uint32_t data_len)
{
    int32_t ret;
    ASSERT(m_nandflash);
    
    nandflash_dev_lock();
    uint32_t map_page_id = bbm_map_page_id_get(&m_bbm, page_id);
    print_debug("read. page_id=%lu->%lu, offset=%lu, data_len=%lu", page_id, map_page_id, offset, data_len);
    uint8_t *p_spare = m_bbm.p_cache;
    ret = m_nandflash->page_read_handle(page_id, offset, p_data, data_len, p_spare, m_nandflash->page_spare_size);
    ASSERT(ret > 0);
    
    // 读写的是整页数据，进行校准
    if (offset == 0 && data_len == m_nandflash->page_data_size) {
        ret = m_nandflash->ecc_repair_handle(p_data, data_len, p_spare);
        if (ret == 0) {             // 数据正确
            nandflash_dev_unlock();
            return data_len;
        } else if (ret > 0) {      // 数据有错误，已经修复
            // 是否需要将正确的数据覆盖
            nandflash_dev_unlock();
            return data_len;
        } else {
            // 数据错误，通过ecc无法恢复; 
            // 1、可以使用更强的ecc校正，或者将ecc校正块划分更小
            // 2、flash是否已经擦写过载？当做坏块处理
            ASSERT(0);
        }
    }
    nandflash_dev_unlock();
    
    return data_len;
}

// @brief 按页写数据，每次写入的数据必须是一整页
int32_t htd_dev_nandflash_page_write(uint32_t page_id, const uint8_t* p_data, uint32_t data_len)
{
    int32_t ret;
    ASSERT(m_nandflash);
    ASSERT(p_data);
    ASSERT(data_len == m_nandflash->page_data_size);

    nandflash_dev_lock();
    uint8_t *p_spare = &m_bbm.p_cache[m_nandflash->page_data_size];
    uint32_t block_id = page_id / m_nandflash->block_size;
    uint32_t map_page_id = bbm_map_page_id_get(&m_bbm, page_id);
    print_debug("write. page_id=%lu->%lu, data_len=%lu, page_data_size=%d", page_id, map_page_id, data_len, m_nandflash->page_data_size);

    do {
        memset(p_spare, 0xff, m_nandflash->page_spare_size);
        m_nandflash->ecc_generate_handle(p_data, data_len, p_spare);
        if (map_page_id != page_id) {
            *(uint32_t*)&p_spare[m_nandflash->ecc_size] = block_id;
        }
        ret = m_nandflash->page_write_handle(map_page_id, p_data, data_len, p_spare, m_nandflash->page_spare_size);
        if (ret >= 0){
            break;
        }
        if (ret == NANDFLASH_ERR_BAD_BLOCK) {
            print_warning("write generate bad block.");
            bbm_page_map_and_write(&m_bbm, p_data, map_page_id);
        } else {
            print_error("write nandflash page failed. ret=%d", ret);
            ASSERT(0);
        }
    } while (0);
    nandflash_dev_unlock();
    return data_len;
}

// @brief 擦除一块数据
int32_t htd_dev_nandflash_block_erase(uint32_t block_id)
{
    ASSERT(m_nandflash);
    
    nandflash_dev_lock();
    uint32_t map_block_id = bbm_map_block_id_get(&m_bbm, block_id);
    print_debug("erase. block_id=%lu->%lu", block_id, map_block_id);
    int32_t ret = m_nandflash->ctrl_handle(NANDFLASH_CMD_ERASE, &map_block_id, sizeof(uint32_t));
    if (NANDFLASH_ERR_BAD_BLOCK == ret) {
        print_warning("bad block. %d", map_block_id);
        for (int i=0; ;++i) {
            if (i > 3) {
                print_error("bbm set logic id failed.");
                ASSERT(0);
            }
            if (0 == bbm_block_map(&m_bbm, map_block_id, 0, NULL)) {
                print_warning("block map success.");
                break;
            }
        }
    } else if (ret < 0) {
        print_error("erase error. ret=%d", ret);
    }
    nandflash_dev_unlock();
    return 0;
}

// @brief 展示已经挂载的nandflash信息
void htd_dev_nandflash_show(void)
{
#if defined(DEBUG_LVL)
    if (m_nandflash == NULL) {
        print_warning("m_nandflash unregistered.");
        return;
    }
    print_log("\tphy_saddr: 0x%08x", m_nandflash->phy_saddr);
    print_log("\tpage_data_size=%d(byte), page_spare_size=%d(byte)", m_nandflash->page_data_size, m_nandflash->page_spare_size);
    print_log("\tecc_size=%d(byte), block_size=%d(page)", m_nandflash->ecc_size, m_nandflash->block_size);
    print_log("\tplane_size=%d(block), plane_count=%d(plane)", m_nandflash->plane_size, m_nandflash->plane_count);
    m_nandflash->ctrl_handle(NANDFLASH_CMD_INFO_SHOW, NULL, 0);
    nandflash_bbm_info_show(&m_bbm);
#endif
}

// @brief 获取页数据内容大小
uint32_t htd_dev_nandflash_page_data_size_get(void)
{
    ASSERT(m_nandflash);
    return m_nandflash->page_data_size;
}

// @brief 获取spare大小
uint32_t htd_dev_nandflash_page_spare_size_get(void)
{
    ASSERT(m_nandflash);
    return m_nandflash->page_spare_size;
}

// @brief 获取ecc大小
uint32_t htd_dev_nandflash_ecc_size_get(void)
{
    ASSERT(m_nandflash);
    return m_nandflash->ecc_size;
}

// @brief 获取一个块多少页
uint32_t htd_dev_nandflash_pages_per_block_get(void)
{
    ASSERT(m_nandflash);
    return m_nandflash->block_size;
}

// @brief 获取总共多少块
uint32_t htd_dev_nandflash_block_count_get(void)
{
    ASSERT(m_nandflash);
    return m_nandflash->plane_count * m_nandflash->plane_size - m_bbm.block_count;
}

// @brief 初始化
int32_t htd_dev_nandflash_init(void)
{
    ASSERT(m_nandflash);

    uint32_t start_block_id;
    uint32_t block_count, all_count;

    uint32_t start_ms = htd_time_ms_get();
    print_debug("nandflash init start. ms=%u", start_ms);
    nandflash_dev_lock_init();
    nandflash_dev_lock();
    int32_t ret = m_nandflash->init_handle();
    if (ret != 0) {
        nandflash_dev_unlock();
        print_error("nandflash init failed. ret=%d", ret);
        return ret;
    }
    print_debug("nandflash init end. ms=%u", htd_time_ms_diff(htd_time_ms_get(), start_ms));
    
    all_count = m_nandflash->plane_count * m_nandflash->plane_size;
    block_count = all_count / 16;       // 1/16作为坏块区域，这里需要根据项目调整
    start_block_id = all_count - block_count;
    bbm_init(m_nandflash, &m_bbm, start_block_id, block_count);
    nandflash_dev_unlock();
    print_debug("nandflash bbm end. ms=%u", htd_time_ms_diff(htd_time_ms_get(), start_ms));
    return ret;
}
















//////////////////////////////////////////////////////////////////////
#if defined(HTD_CONSOLE_ENABLE) && DEBUG_LVL > 0 
#include "htd_console_process.h"
#include "ecc_hamming.h"
#include "ecc_bch.h"


// @brief 随机修改一个bit位
static void flip_one_bit(uint8_t *b, int size)
{
    const int which = rand() % (size * 8);
    const int byte = which >> 3;
    const uint8_t bit = 1 << (which & 7);
    b[byte] ^= bit;
}

// @brief bch 调试
static void ecc_bch_test(uint8_t type)
{
    const ecc_bch_t* p_bch = &bch_1bit;
    if (type == 2) {
        p_bch = &bch_2bit;
    } else if (type == 3) {
        p_bch = &bch_3bit;
    } else if (type == 1) {
        p_bch = &bch_1bit;
    } else {
        type = 4;
        p_bch = &bch_4bit; 
    }

    int i;
    uint8_t bad[512];
    uint8_t chunk[512];
    for (i=0; i<sizeof(chunk); ++i) {
        chunk[i] = rand();
    }
    
    uint8_t ecc[7];
    ecc_bch_generate(p_bch, chunk, sizeof(chunk), ecc);
    int32_t ret = ecc_bch_verify(p_bch, chunk, sizeof(chunk), ecc);
    print_debug("ecc_bch_verify. ret=%d, type=%d", ret, type);
    // 随机修改一个bit值
    memcpy(bad, chunk, sizeof(bad));
    for (i=0; i<type; ++i) {
        flip_one_bit(bad, sizeof(bad));
    }
    ecc_bch_repair(p_bch, bad, sizeof(bad), ecc);
    ret = ecc_bch_verify(p_bch, bad, sizeof(bad), ecc);
    print_debug("ecc_bch_verify. ret=%d", ret);
    if (memcmp(bad, chunk, sizeof(bad)) == 0) {
        print_warning("ecc bch repair success.");
    } else {
        print_error("ecc bch repair failed.");
    }

    // ECC数据发生位跳变
    print_hex("ecc", ecc, sizeof(ecc));
    flip_one_bit(ecc, sizeof(ecc));
    ret = ecc_bch_verify(p_bch, bad, sizeof(bad), ecc);
    print_debug("ecc_bch_verify. ret=%d", ret);
    if (ret != 0) {
        print_hex("ecc", ecc, sizeof(ecc));
        ecc_bch_repair(p_bch, bad, sizeof(bad), ecc);
        if (memcmp(bad, chunk, sizeof(bad)) == 0) {
            print_hex("ecc", ecc, sizeof(ecc));
            print_warning("-----ecc bch repair success.");
        } else {
            print_error("-----ecc bch repair failed.");
        }
    }
}

// @brief hamming 调试
static void ecc_hamming_test(void)
{
    uint8_t bad[256];
    uint8_t chunk[256];
    uint8_t ecc[HAMMING_ECC_SIZE];
    
    for (int i=0; i<sizeof(chunk); ++i) {
        chunk[i] = rand();
    }
    ecc_hamming_generate(chunk, sizeof(chunk), ecc);

#if 0
    ecc_hamming_t ecc_val = ecc_hamming_syndrome(chunk, sizeof(chunk), ecc);
    if (ecc_val == 0) {
        print_debug("ecc verify success.");
    }
    memcpy(bad, chunk, sizeof(bad));
    flip_one_bit(bad, sizeof(bad));
    ecc_val = ecc_hamming_syndrome(bad, sizeof(bad), ecc);
    if (ecc_val != 0) {
        if (0 == ecc_hamming_repair(bad, sizeof(bad), ecc_val)) {
            if (memcmp(bad, chunk, sizeof(bad)) == 0) {
                print_warning("ecc repair success.");
                return;
            } 
        } 
    }
#else
    flip_one_bit(ecc, sizeof(ecc));
    ecc_hamming_t ecc_val = ecc_hamming_syndrome(bad, sizeof(bad), ecc);
    if (ecc_val != 0) {
        if (0 == ecc_hamming_repair(bad, sizeof(bad), ecc_val)) {
            if (memcmp(bad, chunk, sizeof(bad)) == 0) {
                print_warning("ecc repair success.");
                return;
            } 
        } 
    }
#endif
    
    print_error("ecc repair failed.");
}


// @brief 调试命令
static int cmd_ecc(int argc, char **argv)
{
    if (argc != 2) {
        print_error("Usage: %s bch1/bch2/bch3/bch4/hamming", argv[0]);
        return 0;
    }
    srand(htd_time_ms_get());
    if (memcmp(argv[1], "bch1", 5) == 0) {
        ecc_bch_test(1);
    } else if (memcmp(argv[1], "bch2", 5) == 0) {
        ecc_bch_test(2);
    } else if (memcmp(argv[1], "bch3", 5) == 0) {
        ecc_bch_test(3);
    } else if (memcmp(argv[1], "bch4", 5) == 0) {
        ecc_bch_test(4);
    } else if (memcmp(argv[1], "hamming", 8) == 0) {
        ecc_hamming_test();
    } else {
        print_error("Usage: %s bch1/bch2/bch3/bch4/hamming", argv[0]);
    }
    return 0;
}
CONSOLE_CMD_EXPORT(cmd_ecc, ecc);

// @brief 压力测试
static void nandflash_stress_test(void)
{
    uint8_t byte;
    uint32_t i, j;
    uint32_t min_page_id = 0;

    uint8_t* p_page_data = htd_malloc(m_nandflash->page_data_size);
    ASSERT(p_page_data);
    
    for (i=0; i<1001; ++i) {
        for (j=0; j<64*8; ++j) {
            if (min_page_id < j) {
                min_page_id = j;
            }
            // 擦
            if ((j % m_nandflash->block_size) == 0) {
                htd_dev_nandflash_block_erase(j / m_nandflash->block_size);
            }
            byte = rand() % 254;
            memset(p_page_data, byte, m_nandflash->page_data_size);
            // 写
            htd_dev_nandflash_page_write(j, p_page_data, m_nandflash->page_data_size);
        }
        // 读验证
        for (j=0; j<=min_page_id; ++j) {
            memset(p_page_data, 0xff, m_nandflash->page_data_size);
            htd_dev_nandflash_page_read(j, 0, p_page_data, m_nandflash->page_data_size);
        }
    }
    htd_free(p_page_data);
}

// @brief 调试命令
static int nandflash_cmd(int argc, char **argv)
{
    // 压力测试
    if (argc != 2) {
        print_error("Usage: %s init/stress/show", argv[0]);
        return 0;
    }

    if (memcmp(argv[1], "init", 5) == 0) {
        htd_dev_nandflash_init();
    } else if (memcmp(argv[1], "stress", 7) == 0) {
        nandflash_stress_test();
    } else if (memcmp(argv[1], "show", 5) == 0) {
        htd_dev_nandflash_show();
    } else {
        print_error("Usage: %s init/stress/show", argv[0]);
    }
    return 0;
}
CONSOLE_CMD_EXPORT(nandflash_cmd, nandflash);
#endif /**/



//////////////////////////////////////////////////////////////////////
#if defined(HTD_VFS_USING_LIB_DHARA) && defined(HTD_CONSOLE_ENABLE) && (DEBUG_LVL > 0)
#include "dhara_port.h"
#include "map.h"

// @brief 创建 nandflash dhara 设备
// @param gc_ratio
void* htd_nandflash_dhara_dev_create(uint8_t gc_ratio)
{
    uint8_t* p_page_buff = NULL;
    struct dhara_nand* p_nand_dev = NULL;
    uint32_t page_size = htd_dev_nandflash_page_data_size_get();

    struct dhara_map* p_dhara = (struct dhara_map*)htd_malloc(page_size + sizeof(struct dhara_nand) + sizeof(struct dhara_map));
    ASSERT(p_dhara);
    p_nand_dev = (struct dhara_nand*)(p_dhara + 1);
    p_nand_dev->log2_page_size = dhara_log2_off_data(page_size);
    print_warning("page_sie=%d, log2_page_size=%d", page_size, p_nand_dev->log2_page_size);
    p_nand_dev->log2_ppb = dhara_log2_off_data(htd_dev_nandflash_pages_per_block_get());
    p_nand_dev->num_blocks = htd_dev_nandflash_block_count_get();
    p_nand_dev->block_erase_handle = htd_dev_nandflash_block_erase;
    p_nand_dev->page_write_handle = htd_dev_nandflash_page_write;
    p_nand_dev->page_read_handle = htd_dev_nandflash_page_read;
    // 驱动已经做了坏块管理，不需要这组接口
    p_nand_dev->is_bad_handle = NULL;
    p_nand_dev->mark_bad_handle = NULL;
    
    p_page_buff = (uint8_t*)(p_nand_dev + 1);
    dhara_map_init(p_dhara, p_nand_dev, p_page_buff, gc_ratio);
    return p_dhara;
}

// @brief 销毁 nandflash dhara 设备
void htd_nandflash_dhara_dev_destroy(void* p_dhara_dev)
{
    if (p_dhara_dev) {
        htd_free(p_dhara_dev);
    }
}

// @brief 调试命令
static int cmd_nandflash_dhara(int argc, char **argv)
{
    dhara_error_t err_code;
    
    srand(htd_time_ms_get());
    struct dhara_map *m_dhara = htd_nandflash_dhara_dev_create(2);

    // map_resume will return a bad status in the case of an empty map, however this just
    // means that the file system is empty
    dhara_map_resume(m_dhara, &err_code);

    // gc=0; sector_count=14578, 32768
    // gc=1; sector_count=14578, 32768
    // gc=2; sector_count=19608, 32768
    // gc=3; sector_count=22123, 32768
    // gc=4; sector_count=23632, 32768
    uint32_t sector_count = dhara_map_capacity(m_dhara);
    print_warning("sector_count=%d, %d", sector_count, 
        htd_dev_nandflash_block_count_get() * htd_dev_nandflash_pages_per_block_get());

    dhara_sector_t s;
    uint32_t page_size = htd_dev_nandflash_page_data_size_get();
    uint8_t* p_wdata = htd_malloc(page_size);
    ASSERT(p_wdata);

    for (uint32_t i=0; i<5; ++i) {
        s = rand() % sector_count;
        print_warning("select page: %u", s);
        memset(p_wdata, 'a', page_size);
        if (0 != dhara_map_write(m_dhara, s, p_wdata, &err_code)) {
            print_error("dhara write failed. %s", dhara_strerror(err_code));
            htd_free(p_wdata);
            htd_nandflash_dhara_dev_destroy(m_dhara);
            return -1;
        }
        // 读取
        memset(p_wdata, 0, page_size);
        if (0 != dhara_map_read(m_dhara, s, p_wdata, &err_code)) {
            print_error("dhara read failed. %s", dhara_strerror(err_code));
            htd_free(p_wdata);
            htd_nandflash_dhara_dev_destroy(m_dhara);
            return -1;
        }
        // 判断
        if (false == htd_mem_allof(p_wdata, 'a', page_size)) {
            print_error("compare data failed.");
            htd_free(p_wdata);
            htd_nandflash_dhara_dev_destroy(m_dhara);
            return -1;
        }
    }
    
    htd_free(p_wdata);
    htd_nandflash_dhara_dev_destroy(m_dhara);
    print_warning("dhara test success.");
    return 0;
}
CONSOLE_CMD_EXPORT(cmd_nandflash_dhara, nandflash_dhara);

#endif /*VFS_USING_LIB_DHARA*/

