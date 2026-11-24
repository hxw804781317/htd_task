// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL 4
#define DEBUG_TAG              "dhara"
#include "htd_vfs_port.h"
#include "dhara_port.h"
#include "map.h"


// @brief 检查是否为坏块
int dhara_nand_is_bad(const struct dhara_nand *n, dhara_block_t b)
{
    if (n->is_bad_handle) {
        b += n->start_block_id;
        return n->is_bad_handle(b);
    }
    return false;
}

// @brief 标记为坏块
void dhara_nand_mark_bad(const struct dhara_nand *n, dhara_block_t b)
{
    if (n->mark_bad_handle) {
        b += n->start_block_id;
        n->mark_bad_handle(b);
    }
}

// @brief 擦除一个块
int dhara_nand_erase(const struct dhara_nand *n, dhara_block_t b, dhara_error_t *err)
{
    b += n->start_block_id;
    if (0 == n->block_erase_handle(b)) {
        return 0;
    }
    if (err) {
        *err = DHARA_E_BAD_BLOCK;
    }
    print_error("erase failed. ");
    return -1;
}

// @brief 编程一个NAND页，包括ECC和校验和
int dhara_nand_prog(const struct dhara_nand *n, dhara_page_t p, const uint8_t *data, dhara_error_t *err)
{
    int32_t data_len = 1 << n->log2_page_size;
    p += n->start_block_id * (1 << n->log2_ppb);
    int32_t ret = n->page_write_handle(p, data, data_len);
    if (ret == data_len) {
        return 0;
    }
    if (err) {
        *err = DHARA_E_BAD_BLOCK;
    }
    print_error("write failed. ret=%d. data_len=%d", ret, data_len);
    return -1;
}

// @brief 判断一个页面是否为空
int dhara_nand_is_free(const struct dhara_nand *n, dhara_page_t p)
{
    uint32_t i;
    bool is_free = true;
    uint32_t page_size = 1 << n->log2_page_size;
    
    p += n->start_block_id * (1 << n->log2_ppb);
    uint32_t *p_data = htd_malloc(page_size);
    ASSERT(p_data);
    
    int32_t ret = n->page_read_handle(p, 0, (uint8_t*)p_data, page_size);
    if (ret == page_size) {
        page_size = page_size / sizeof(uint32_t);
        for (i=0; i<page_size; i++) {
            if (p_data[i] != 0xffffffff) {
                is_free = false;
                break;
            }
        }
    } else {
        is_free = false;
    }
    htd_free(p_data);

    //print_warning("page[%d] is free. is_free=%d, ret=%d", p, is_free, ret);
    return is_free;
}

// @brief 读取（可能是部分）NAND页面，如有必要尝试ECC
// @return 成功返回0，失败返回-1，如果ECC校验失败，设置错误码为 E_ECC
int dhara_nand_read(const struct dhara_nand *n, dhara_page_t p, size_t offset, size_t length, uint8_t *data, dhara_error_t *err)
{
    p += n->start_block_id * (1 << n->log2_ppb);
    int32_t ret = n->page_read_handle(p, offset, data, length);
    if (ret == length) {
        return 0;
    }
    if (err) {
        *err = DHARA_E_ECC;
    }
    print_error("read failed. ret=%d", ret);
    return -1;
}

// @brief 利用可能的内部缓存将一页复制到另一页
int dhara_nand_copy(const struct dhara_nand *n, dhara_page_t src, dhara_page_t dst, dhara_error_t *err)
{
    dst += n->start_block_id * (1 << n->log2_ppb);
    src += n->start_block_id * (1 << n->log2_ppb);
    
    uint32_t data_len = 1 << n->log2_page_size;
    char* p_data = htd_malloc(data_len);
    ASSERT(p_data);

    int32_t ret = n->page_read_handle(src, 0, p_data, data_len);
    if (ret != data_len) {
        if (err) {
            *err = DHARA_E_ECC;
        }
        htd_free(p_data);
        return -1;
    }

    ret = n->page_write_handle(dst, p_data, data_len);
    htd_free(p_data);
    if (ret == data_len) {
        return 0;
    }
    if (err) {
        *err = DHARA_E_BAD_BLOCK;
    }
    return -1;
}

// @brief 求数值 data 是2的n次方
uint32_t dhara_log2_off_data(uint32_t data)
{
    uint32_t n = 0;
    while (data >= 2) {
        n++;
        data >>= 1;
    }
    return n;
}

