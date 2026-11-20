// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL       4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG       "swap"
#include <stddef.h>
#include <stdbool.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include "htd_boot_process.h"
#include "htd_boot_overwrite_swap.h"


// 使用 swap-offset 算法进行分区覆盖，需要满足以下几个条件
//     1: 备份区的擦除单位必须大小一致
//     2: 主分区的擦除单位必须大于等于备份区的擦除单位
//     3: 主分区的擦除单位除以备份区的擦除单位，都能整除
#if defined(HTD_BOOT_OVERWRITE_SWAP)
//////////////////////////////////////////////////////////////////////
// @brief 从 B->A 拷贝数据
static uint32_t swap_onepage_b2a(htd_boot_object_t* p_object, uint32_t page_len)
{
    // B->A, A为写入，地址往前
    uint32_t total_len = page_len;
    uint32_t addrB_end = p_object->p_env->bankB_addr + p_object->fwB_offset + p_object->fwB_size;
    uint32_t data_len = sizeof(p_object->swap_data);
    while (total_len) {
        if (total_len < data_len) {
            data_len = total_len;
        }
        boot_flash_erase(p_object->p_flash_fwA, p_object->addrB_dst, 1);
        if (p_object->addrB_src < addrB_end) {
            // 有数据需要读取
            boot_flash_read(p_object->p_flash_fwB, p_object->addrB_src, p_object->swap_data, data_len);
            boot_flash_write(p_object->p_flash_fwA, p_object->addrB_dst, p_object->swap_data, data_len);
        }
        total_len -= data_len;
        p_object->addrB_dst += data_len;
        p_object->addrB_src += data_len;
    }
    return page_len;
}

// @brief 从 A->B 拷贝数据
static uint32_t swap_onepage_a2b(htd_boot_object_t* p_object, uint32_t page_len)
{
    // A->B, B为写入，地址往前
    uint32_t total_len = page_len;
    uint32_t addrA_end = p_object->p_env->bankA_addr + p_object->fwA_size;
    uint32_t data_len = sizeof(p_object->swap_data);
    while (total_len) {
        if (total_len < data_len) {
            data_len = total_len;
        }
        
        boot_flash_erase(p_object->p_flash_fwB, p_object->addrA_dst, 1);
        if (p_object->addrA_src < addrA_end) {
            // 有数据需要读取
            boot_flash_read(p_object->p_flash_fwA, p_object->addrA_src, p_object->swap_data, data_len);
            boot_flash_write(p_object->p_flash_fwB, p_object->addrA_dst, p_object->swap_data, data_len);
        }
        total_len -= data_len;
        p_object->addrA_src += data_len;
        p_object->addrA_dst += data_len;
    }
    return page_len;
}

// @brief 数据交换进度恢复
static uint32_t swap_progress_recover(htd_boot_object_t * p_object)
{
    uint32_t page_len = boot_flash_erase_unit_get(p_object->p_flash_fwA, p_object->addrA_src);
    // A->B 已经完成，补充地址偏移
    p_object->addrA_src += page_len;
    p_object->addrA_dst += page_len;
    // B->A 继续操作
    boot_debug("B->A: 0x%08x->0x%08x", p_object->addrB_src, p_object->addrB_dst);
    return swap_onepage_b2a(p_object, page_len);
}

// @brief 交换一页数据
static uint32_t swap_one_page(htd_boot_object_t * p_object, uint32_t offset)
{
    uint32_t page_len = boot_flash_erase_unit_get(p_object->p_flash_fwA, p_object->addrA_src);
    // A->B
    boot_debug("A->B: 0x%08x->0x%08x", p_object->addrA_src, p_object->addrA_dst);
    swap_onepage_a2b(p_object, page_len);
    boot_config_offset_state_set(offset, BOOT_STATE_SWAP_A2B);
    // B->A
    boot_debug("B->A: 0x%08x->0x%08x", p_object->addrB_src, p_object->addrB_dst);
    return swap_onepage_b2a(p_object, page_len);
}

// @brief 数据交换
void boot_overwrite(htd_boot_object_t* p_object)
{
    boot_state_t state = boot_config_state_get();
    if (state != BOOT_STATE_SWAP && 
        state != BOOT_STATE_SWAP_A2B && 
        state != BOOT_STATE_DOWNLOAD_FINISH) {
        return;
    }
    
    // 初始化地址
    p_object->addrA_src = p_object->p_env->bankA_addr;
    p_object->addrA_dst = p_object->p_env->bankB_addr;
    p_object->addrB_src = p_object->p_env->bankB_addr + p_object->fwB_offset;
    p_object->addrB_dst = p_object->addrA_src;
    
    // 进度恢复
    uint32_t swap_size = BOOT_MAX(p_object->fwA_size, p_object->fwB_size);
    uint32_t offset = boot_config_offset_get();
    if (state == BOOT_STATE_SWAP || state == BOOT_STATE_SWAP_A2B) {
        p_object->addrA_src += offset;
        p_object->addrB_src += offset;
        p_object->addrA_dst += offset;
        p_object->addrB_dst += offset;
        boot_debug("addrA=0x%08x, addrB=0x%08x", p_object->addrA_src, p_object->addrB_src);
        if (state == BOOT_STATE_SWAP_A2B) {
            offset += swap_progress_recover(p_object);
            boot_config_offset_state_set(offset, BOOT_STATE_SWAP);
        }
        swap_size -= offset;
    } else {
        offset = 0;
    } 
    boot_debug("fwA_size=%u, fwB_size=%u, swap_size=%u", p_object->fwA_size, p_object->fwB_size, swap_size);
    
    // 交换数据
    uint32_t buff_len;
    while (swap_size) {
        buff_len = swap_one_page(p_object, offset);
        if (swap_size <= buff_len) {
            break;
        }
        offset += buff_len;
        boot_config_offset_state_set(offset, BOOT_STATE_SWAP);
        swap_size -= buff_len;
    }
    boot_config_offset_state_set(0, BOOT_STATE_SWAP_FINISH);
}

// @brief 数据回滚，回滚都是从 B->A 搬数据
void boot_rollback(htd_boot_object_t* p_object)
{
    boot_state_t state = boot_config_state_get();
    if (state != BOOT_STATE_IMAGE_ERROR && state != BOOT_STATE_ROLLBACK) {
        return;
    }

    // 初始化地址
    p_object->addrB_src = p_object->p_env->bankB_addr;
    p_object->addrB_dst = p_object->p_env->bankA_addr;
    p_object->fwB_offset = 0;
    
    // 进度恢复
    uint32_t swap_size = p_object->fwB_size;
    uint32_t offset = boot_config_offset_get();
    if (state == BOOT_STATE_ROLLBACK) {
        p_object->addrB_src += offset;
        p_object->addrB_dst += offset;
        swap_size -= offset;
    } else {
        offset = 0;
    }
    boot_warning("--->start rollback. fwB_size=0x%08x", p_object->fwB_size);
    
    // 回滚
    uint32_t buff_len;
    uint32_t page_len;
    while (swap_size) {
        page_len = boot_flash_erase_unit_get(p_object->p_flash_fwA, p_object->addrB_dst);
        buff_len = swap_onepage_b2a(p_object, page_len);
        if (swap_size <= buff_len) {
            break;
        }
        offset += buff_len;
        boot_config_offset_state_set(offset, BOOT_STATE_ROLLBACK);
        swap_size -= buff_len;
    }
    boot_config_offset_state_set(0, BOOT_STATE_ROLLBACK_FINISH);
}

#endif /*defined(HTD_BOOT_OVERWRITE_SWAP)*/


