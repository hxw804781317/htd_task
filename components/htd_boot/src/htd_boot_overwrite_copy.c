// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL       4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG       "copy"
#include <stddef.h>
#include <stdbool.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include "htd_boot_process.h"
#include "htd_boot_overwrite_swap.h"


#if defined(HTD_BOOT_OVERWRITE_COPY)
//////////////////////////////////////////////////////////////////////
// @brief 从 B->A 拷贝数据
static uint32_t copy_onepage_b2a(htd_boot_object_t* p_object)
{
    uint32_t len, total_len;
    len = total_len = boot_flash_erase_unit_get(p_object->p_flash_fwA, p_object->addrB_dst);
    
    // B->A, A为写入，地址往前
    uint32_t addrB_end = p_object->p_env->bankB_addr + p_object->fwB_size;
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
    return len;
}

// @brief 数据交换
void boot_overwrite(htd_boot_object_t* p_object)
{
    boot_state_t state = boot_config_state_get();
    if (state != BOOT_STATE_SWAP && state != BOOT_STATE_DOWNLOAD_FINISH) {
        return;
    }
    
    uint32_t buff_len;
    uint32_t swap_size = BOOT_MAX(p_object->fwA_size, p_object->fwB_size);
    p_object->addrB_src = p_object->p_env->bankB_addr;
    p_object->addrB_dst = p_object->p_env->bankA_addr;
    print_debug("fwA_size=%u, fwB_size=%u", p_object->fwA_size, p_object->fwB_size);
    
    // 进度恢复
    uint32_t offset = boot_config_offset_get();
    if (state == BOOT_STATE_SWAP) {
        p_object->addrB_src += offset;
        p_object->addrB_dst += offset;
        print_debug("addrB=0x%08x, addrB_dst=0x%08x", p_object->addrB_src, p_object->addrB_dst);
        swap_size -= offset;
    } else {
        offset = 0;
    } 
    
    // 按页交换数据
    while (swap_size) {
        buff_len = copy_onepage_b2a(p_object);
        if (swap_size <= buff_len) {
            break;
        }
        offset += buff_len;
        boot_config_offset_state_set(offset, BOOT_STATE_SWAP);
        swap_size -= buff_len;
    }
    boot_config_offset_state_set(0, BOOT_STATE_SWAP_FINISH);
}

#endif /*defined(HTD_BOOT_OVERWRITE_COPY)*/

