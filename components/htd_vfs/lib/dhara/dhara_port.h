/* Dhara - NAND flash management layer
 * Copyright (C) 2013 Daniel Beer <dlbeer@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#ifndef _DHARA_PORT_H_
#define _DHARA_PORT_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "error.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


/* Each page in a NAND device is indexed, starting at 0. It's required
 * that there be a power-of-two number of pages in a eraseblock, so you can
 * view a page number is being a concatenation (in binary) of a block
 * number and the number of a page within a block.
 */
typedef uint32_t dhara_page_t;

/* Blocks are also indexed, starting at 0. */
typedef uint32_t dhara_block_t;


typedef struct dhara_nand {
    /* Base-2 logarithm of the page size. If your device supports
     * partial programming, you may want to subdivide the actual
     * pages into separate ECC-correctable regions and present those
     * as pages.
     */
    uint8_t log2_page_size;
    // 一个块多少页，转成2的N次方
    uint8_t log2_ppb;
    // 块数量，一个块对应一个擦除单位
    unsigned int num_blocks;
    // 相对起始块 id
    dhara_block_t start_block_id;
    
    bool (*is_bad_handle)(dhara_block_t block_id);
    void (*mark_bad_handle)(dhara_block_t block_id);
    
    int32_t (*block_erase_handle)(uint32_t block_id);
    int32_t (*page_write_handle)(uint32_t page_id, const uint8_t* p_data, uint32_t data_len);
    int32_t (*page_read_handle)(uint32_t page_id, uint32_t offset, uint8_t* p_data, uint32_t data_len);
}dhara_nand_dev_t;



// @brief 检查是否为坏块
int dhara_nand_is_bad(const dhara_nand_dev_t *n, dhara_block_t b);

// @brief 标记为坏块
void dhara_nand_mark_bad(const dhara_nand_dev_t *n, dhara_block_t b);

// @brief 擦除一个块
int dhara_nand_erase(const dhara_nand_dev_t *n, dhara_block_t b, dhara_error_t *err);

// @brief 编程一个NAND页，包括ECC和校验和
int dhara_nand_prog(const dhara_nand_dev_t *n, dhara_page_t p, const uint8_t *data, dhara_error_t *err);

// @brief 判断一个页面是否为空
int dhara_nand_is_free(const dhara_nand_dev_t *n, dhara_page_t p);

// @brief 读取（可能是部分）NAND页面，如有必要尝试ECC
// @return 成功返回0，失败返回-1，如果ECC校验失败，设置错误码为 E_ECC
int dhara_nand_read(const dhara_nand_dev_t *n, dhara_page_t p,
                    size_t offset, size_t length, uint8_t *data, dhara_error_t *err);

// @brief 利用可能的内部缓存将一页复制到另一页
int dhara_nand_copy(const dhara_nand_dev_t *n, dhara_page_t src, dhara_page_t dst, dhara_error_t *err);

// @brief 求数值 data 是2的n次方
uint32_t dhara_log2_off_data(uint32_t data);


#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_DHARA_PORT_H_*/

