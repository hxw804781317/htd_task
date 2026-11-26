// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
//#define DEBUG_LVL       4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG       "nand.win32"
#include "htd_common.h"
#include "htd_dev_nandflash.h"
#include "ecc_hamming.h"
#include "ecc_bch.h"

#if defined(HTD_VFS_ENABLE) && defined(_WIN32) && 1
#include "htd_vfs_process.h"
//////////////////////////////////////////////////////////////////////
// 模拟一块64M的 nandflash
#define NANDFLASH_FILE_PATH                 "/nandflash.bin"
#define NANDFLASH_START_ADDR                0x64000000
#define NANDFLASH_SIZE                      (64*1024*1024)
#define NANDFLASH_PAGE_DATA_SIZE            2048                // 512
#define NANDFLASH_PAGE_SPARE_SIZE           64                  // 16
#define NANDFLASH_PAGE_PER_BLOCK            64                  // 32

#define NANDFLASH_PAGE_SIZE                 (NANDFLASH_PAGE_DATA_SIZE + NANDFLASH_PAGE_SPARE_SIZE)
#define NANDFLASH_BLOCK_SIZE                (NANDFLASH_PAGE_SIZE * NANDFLASH_PAGE_PER_BLOCK)
#define NANDFLASH_BLOCK_COUNT               (NANDFLASH_SIZE / NANDFLASH_PAGE_DATA_SIZE / NANDFLASH_PAGE_PER_BLOCK)
#define NANDFLASH_ECC_BLOCK_LEN             (256)
#define NANDFLASH_ECC_SIZE                  ((NANDFLASH_PAGE_DATA_SIZE) * 5 / NANDFLASH_ECC_BLOCK_LEN)      // 2048/256*5=8*5=40，必须小于 NANDFLASH_PAGE_PER_BLOCK


// SLC(单层单元，理论可以擦写次数10万次)
// MLC(双层单元，理论可以擦写次数为3000-5000次)
// TLC(三层单元，理论可以擦写次数为1000次)
#define NANDFLASH_PROGRAM_COUNT              1000
typedef struct {
    int32_t program_cnt;
} nandflash_block_head_t;



//////////////////////////////////////////////////////////////////////
static int32_t m_nandflash_fd = -1;
static const ecc_bch_t *m_ecc_bch = &bch_3bit;       // 和这个 NANDFLASH_ECC_SIZE 宏关联

// @brief 打开文件
static int32_t nandflash_file_open(void)
{
    if (m_nandflash_fd < 0) {
        m_nandflash_fd = open(NANDFLASH_FILE_PATH, O_BINARY|O_RDWR|O_CREAT, 0777);
        if (m_nandflash_fd < 0) {
            print_error("open file failed. filepath=%s, fd=%d", NANDFLASH_FILE_PATH, m_nandflash_fd);
            ASSERT(0);
        }
    }
    return m_nandflash_fd;
}

// @brief 关闭文件
static int32_t nandflash_file_close(void)
{
#if 0
    if (m_nandflash_fd > 0) {
        close(m_nandflash_fd);
        m_nandflash_fd = -1;
    }
#endif
    return 0;
}

// @brief 随机修改一个bit位
static void nandflash_simulator_change_one_bit(uint8_t *b, int size)
{
    int which = rand() % (size * 8);
    int byte = which >> 3;
    uint8_t bit = 1 << (which & 7);
    b[byte] ^= bit;
}

// @brief 页读取
static int32_t nandflash_simulator_page_read(
    uint32_t page_id, uint32_t offset, uint8_t* p_page_data, uint32_t page_data_len, uint8_t* p_spare, uint32_t spare_len)
{
    ASSERT(page_id < NANDFLASH_BLOCK_COUNT * NANDFLASH_PAGE_PER_BLOCK);
    ASSERT(offset + page_data_len <= NANDFLASH_PAGE_DATA_SIZE);

    uint32_t block_id = page_id / NANDFLASH_PAGE_PER_BLOCK;
    offset += block_id * (NANDFLASH_BLOCK_SIZE + sizeof(nandflash_block_head_t)) + sizeof(nandflash_block_head_t);
    offset += (page_id % NANDFLASH_PAGE_PER_BLOCK) * NANDFLASH_PAGE_SIZE;
    
    int32_t fd = nandflash_file_open();
    ASSERT(fd > 0);
    
    int32_t ret;
    if (p_page_data && page_data_len) {
        ret = lseek(fd, offset, SEEK_SET);
        ASSERT(ret == offset);
        ret = read(fd, p_page_data, page_data_len);
        ASSERT(ret == page_data_len);
        
        // 模拟产生数据位翻转，概率0.01%
        if (page_data_len == NANDFLASH_PAGE_DATA_SIZE) {
            if ((rand() % 10000) < 5) {
                //print_warning("read flip one bit. page_id=%u", page_id);
                nandflash_simulator_change_one_bit(p_page_data, page_data_len);
                ret = lseek(fd, offset, SEEK_SET);
                ASSERT(ret == offset);
                ret = write(fd, p_page_data, page_data_len);
                ASSERT(ret == page_data_len);
            }
        }
    }
    if (p_spare && spare_len) {
        offset += NANDFLASH_PAGE_DATA_SIZE;
        ret = lseek(fd, offset, SEEK_SET);
        ASSERT(ret == offset);
        ret = read(fd, p_spare, spare_len);
        ASSERT(ret == spare_len);
        //page_data_len += spare_len;
    }
    nandflash_file_close();
    return page_data_len;
}

// @brief 写入数据，nandflash最小写入单元为一页
static int32_t nandflash_simulator_page_write(
    uint32_t page_id, const uint8_t* p_page_data, uint32_t page_data_len, const uint8_t* p_spare, uint32_t spare_len)
{
    ASSERT(page_id < NANDFLASH_BLOCK_COUNT * NANDFLASH_PAGE_PER_BLOCK);

    uint32_t i;
    uint32_t block_id = page_id / NANDFLASH_PAGE_PER_BLOCK;
    int32_t fd = nandflash_file_open();
    ASSERT(fd > 0);

    // 块偏移
    uint32_t offset = block_id * (NANDFLASH_BLOCK_SIZE + sizeof(nandflash_block_head_t));
    offset += sizeof(nandflash_block_head_t);

    uint8_t* p_cache = (uint8_t*)htd_malloc(NANDFLASH_PAGE_SIZE+32);
    ASSERT(p_cache);
    // 写入数据
    offset += (page_id % NANDFLASH_PAGE_PER_BLOCK) * NANDFLASH_PAGE_SIZE;
    int32_t ret = lseek(fd, offset, SEEK_SET);
    ASSERT(ret == offset);
    ret = read(fd, p_cache, NANDFLASH_PAGE_SIZE);
    ASSERT(ret == NANDFLASH_PAGE_SIZE);
    for (i=0; i<page_data_len; ++i) {       // 数据只能从1->0
        p_cache[i] &= p_page_data[i];
    }
    for (i=0; i<spare_len; ++i) {
        p_cache[NANDFLASH_PAGE_DATA_SIZE + i] &= p_spare[i];
    }
    // 模拟产生数据位翻转
    if ((rand() % 300) == 1) {
        nandflash_simulator_change_one_bit(p_cache, NANDFLASH_PAGE_DATA_SIZE);
        //print_debug("write flip one bit. page_id=%d", page_id);
    }
    ret = lseek(fd, offset, SEEK_SET);
    ASSERT(ret == offset);
    ret = write(fd, p_cache, NANDFLASH_PAGE_SIZE);
    ASSERT(ret == NANDFLASH_PAGE_SIZE);
    htd_free(p_cache);
    
    // 随机产生坏块
    if ((rand() % 100000) == 1) {
        nandflash_block_head_t header;
        header.program_cnt = NANDFLASH_PROGRAM_COUNT * 10;
        offset = block_id * (NANDFLASH_BLOCK_SIZE + sizeof(nandflash_block_head_t));
        ret = lseek(fd, offset, SEEK_SET);
        ASSERT(ret == offset);
        write(fd, &header, sizeof(header));
        nandflash_file_close();
        return NANDFLASH_ERR_BAD_BLOCK;
    }
    nandflash_file_close();
    return page_data_len + spare_len;
}

// @brief 擦除
static int32_t nandflash_simulator_block_erase(uint32_t block_id)
{
    if (block_id >= NANDFLASH_BLOCK_COUNT) {
        print_error("block_id=%d", block_id);
        ASSERT(0);
    }
    print_debug("erase. block_id=%d", block_id);
    int32_t fd = nandflash_file_open();
    ASSERT(fd > 0);
    
    uint32_t offset = block_id * (NANDFLASH_BLOCK_SIZE + sizeof(nandflash_block_head_t));
    offset += sizeof(nandflash_block_head_t);
    int32_t ret = lseek(fd, offset, SEEK_SET);
    ASSERT(ret == offset);
    // 清除块数据内容，全部置为0xff
    uint8_t buff[256];
    uint32_t data_len = sizeof(buff);
    memset(buff, 0xff, data_len);
    uint32_t size = NANDFLASH_BLOCK_SIZE;
    while (size) {
        if (data_len > size) {
            data_len = size;
        }
        ret = write(fd, buff, data_len);
        ASSERT(ret == data_len);
        size -= data_len;
    }
    
    // 读块头部信息
    offset = block_id * (NANDFLASH_BLOCK_SIZE + sizeof(nandflash_block_head_t));
    ret = lseek(fd, offset, SEEK_SET);
    ASSERT(ret == offset);
    nandflash_block_head_t header;
    ret = read(fd, &header, sizeof(header));
    if (header.program_cnt >= NANDFLASH_PROGRAM_COUNT) {
        nandflash_file_close();
        return NANDFLASH_ERR_BAD_BLOCK;
    }
    // 随机产生坏块
    srand(htd_time_ms_get());
    if ((rand() % 8000) == 1) {
        header.program_cnt = NANDFLASH_PROGRAM_COUNT * 10;
        ret = lseek(fd, offset, SEEK_SET);
        ASSERT(ret == offset);
        write(fd, &header, sizeof(header));
        nandflash_file_close();
        return NANDFLASH_ERR_BAD_BLOCK;
    }
    
    // 擦写次数递增
    header.program_cnt++;
    ret = lseek(fd, offset, SEEK_SET);
    ASSERT(ret == offset);
    ret = write(fd, &header, sizeof(header));
    ASSERT(ret == sizeof(header));
    nandflash_file_close();
    return 0;
}

// @brief 生成ecc值，返回生成的 ecc 数据长度
static uint32_t nandflash_simulator_ecc_generate(const uint8_t* p_data, uint32_t data_len, uint8_t *ecc)
{
#if 0
    uint32_t ecc_len = 0;
    uint32_t len = NANDFLASH_ECC_BLOCK_LEN;
    while (data_len) {
        if (data_len < len) {
            len = data_len;
        }
        ecc_hamming_generate(p_data, len, ecc);
        data_len -= len;
        p_data += len;
        ecc += 3;
        ecc_len += 3;
    }
    return ecc_len;
#else
    uint32_t ecc_len = 0;
    uint32_t len = NANDFLASH_ECC_BLOCK_LEN;
    while (data_len) {
        if (data_len < len) {
            len = data_len;
        }
        ecc_bch_generate(m_ecc_bch, p_data, len, ecc);
        data_len -= len;
        p_data += len;
        ecc += m_ecc_bch->ecc_bytes;
        ecc_len += m_ecc_bch->ecc_bytes;
    }
    return ecc_len;
#endif
}

// @brief 修复, 数据正确不用修复返回0，修复失败返回-1，数据修复成功返回1
static int32_t nandflash_simulator_ecc_repair(uint8_t* p_data, uint32_t data_len, const uint8_t* ecc)
{
#if 0
    int32_t ret = 0;
    ecc_hamming_t ecc_val;
    uint8_t* p_start = p_data;
    uint32_t len = NANDFLASH_ECC_BLOCK_LEN;
    while (data_len) {
        if (data_len < len) {
            data_len = len;
        }
        
        ecc_val = ecc_hamming_syndrome(p_data, len, ecc);
        if (ecc_val) {
            if (0 != ecc_hamming_repair(p_data, len, ecc_val)) {
                return -1;
            }
            if (ecc_hamming_syndrome(p_data, len, ecc)) {
                return -1;
            }
            ret = 1;
        }
        
        ecc += 3;
        p_data += len;
        data_len -= len;
    }
    return ret;
#else
    int32_t ret = 0;
    uint8_t* p_start = p_data;
    uint32_t len = NANDFLASH_ECC_BLOCK_LEN;
    while (data_len) {
        if (data_len < len) {
            data_len = len;
        }
        if (ecc_bch_verify(m_ecc_bch, p_data, len, ecc) != 0) {
            ecc_bch_repair(m_ecc_bch, p_data, len, (uint8_t*)ecc);
            if (ecc_bch_verify(m_ecc_bch, p_data, len, ecc)) {
                return -1;
            }
            ret = 1;
        }
        ecc += m_ecc_bch->ecc_bytes;
        p_data += len;
        data_len -= len;
    }
    return ret;
#endif
}

// @brief 初始化, 创建一个对等大小的文件
static int32_t nandflash_simulator_init(void)
{
    srand(htd_time_ms_get());
    uint32_t flash_size = (sizeof(nandflash_block_head_t) + NANDFLASH_BLOCK_SIZE) * NANDFLASH_BLOCK_COUNT;
    int32_t fd = nandflash_file_open();
    ASSERT(fd > 0);

    // 已经创建好了，不用再创建
    int32_t ret = lseek(fd, 0, SEEK_END);
    if (ret == flash_size) {
        nandflash_file_close();
        return 0;
    }

    // 创建虚拟nandflash，随机标记4%的区块为坏块
    uint8_t buff[256];
    uint32_t size;
    uint32_t data_len = sizeof(buff);
    memset(buff, 0xff, data_len);
    nandflash_block_head_t header;
    for (uint32_t i=0; i<NANDFLASH_BLOCK_COUNT; ++i) {
        header.program_cnt = 0;
        if ((rand() % 100) < 4 || i == 2) {
            header.program_cnt = NANDFLASH_PROGRAM_COUNT * 10;
        }
        // 先写入块头
        ret = write(fd, &header, sizeof(nandflash_block_head_t));
        ASSERT(ret == sizeof(nandflash_block_head_t));
        // 写入块数据
        data_len = sizeof(buff);
        size = NANDFLASH_BLOCK_SIZE;
        while (size) {
            if (data_len > size) {
                data_len = size;
            }
            ret = write(fd, buff, data_len);
            ASSERT(ret == data_len);
            size -= data_len;
        }
    }
    nandflash_file_close();
    return 0;
}

// @brief 展示每一block的擦除次数
static void nandflash_simulator_info_show(void)
{
    nandflash_block_head_t page_head;
    int32_t fd = nandflash_file_open();
    ASSERT(fd > 0);

    uint32_t block_id = 0;
    uint32_t total_cnt = 0;
    int32_t max_cnt = 0, min_cnt = 0x7fffffff;
    uint32_t offset = 0;
    uint32_t flash_size = (sizeof(nandflash_block_head_t) + NANDFLASH_BLOCK_SIZE) * NANDFLASH_BLOCK_COUNT;
    while (offset < flash_size) {
        lseek(fd, offset, SEEK_SET);
        memset(&page_head, 0, sizeof(page_head));
        read(fd, &page_head, sizeof(page_head));
        offset += sizeof(nandflash_block_head_t) + NANDFLASH_BLOCK_SIZE;
        
        print_debug("block_id=%u, program_cnt=%-6d", block_id, page_head.program_cnt);
        if (page_head.program_cnt <= NANDFLASH_PROGRAM_COUNT) {
            if (page_head.program_cnt > max_cnt) {
                max_cnt = page_head.program_cnt;
            }
            if (page_head.program_cnt < min_cnt) {
                min_cnt = page_head.program_cnt;
            }
            total_cnt += page_head.program_cnt;
        }
        block_id++;
    }
    nandflash_file_close();
    print_debug("program: total(%d), max(%d), min(%d)", total_cnt, max_cnt, min_cnt);
    print_debug("program data count: %u(byte)", total_cnt * NANDFLASH_BLOCK_SIZE);
    print_debug("NANDFLASH_BLOCK_COUNT=%d", NANDFLASH_BLOCK_COUNT);
}

// @brief 控制函数
static int32_t nandflash_simulator_ctrl(nandflash_cmd_t cmd, void* p_param, uint32_t param_len)
{
    if (cmd == NANDFLASH_CMD_ERASE) {                   // 擦除
        return nandflash_simulator_block_erase(*(uint32_t*)p_param);
    } else if (cmd == NANDFLASH_CMD_INFO_SHOW) {        // 信息展示
        nandflash_simulator_info_show();
    }
    return 0;
}

// @brief 注册到上层应用中
static int32_t nandflash_simulator_register(void)
{
    static const dev_nandflash_t nandflash_simulator = {
        .phy_saddr = NANDFLASH_START_ADDR,
        .page_data_size = NANDFLASH_PAGE_DATA_SIZE, 
        .page_spare_size = NANDFLASH_PAGE_SPARE_SIZE,
        .ecc_size = NANDFLASH_ECC_SIZE,
        .block_size = NANDFLASH_PAGE_PER_BLOCK,
        .plane_size = NANDFLASH_BLOCK_COUNT,
        .plane_count = 1,
        .page_write_handle = nandflash_simulator_page_write,
        .page_read_handle = nandflash_simulator_page_read,
        .ecc_generate_handle = nandflash_simulator_ecc_generate,
        .ecc_repair_handle = nandflash_simulator_ecc_repair,
        .ctrl_handle = nandflash_simulator_ctrl,
        .init_handle = nandflash_simulator_init,
    };
    print_debug("nandflash simulator register.");
    return htd_dev_nandflash_register(&nandflash_simulator);
}
DEVICE_INIT_FUNC_REGISTER(nandflash_simulator_register);

#endif /*defined(HTD_VFS_ENABLE) && defined(_WIN32)*/


