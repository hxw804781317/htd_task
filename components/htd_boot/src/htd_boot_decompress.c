// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL       2       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG       "boot.compress"
#include "htd_boot_process.h"
#include "htd_boot_decompress.h"



//////////////////////////////////////////////////////////////////////
#if defined(HTD_BOOT_IN_BOOTLOADER)
// @brief 保存解压进度
static int32_t boot_config_decompress_progress_set(htd_boot_object_t* p_object, uint32_t rd_offset, uint32_t wr_offset)
{
    boot_info_t boot_config;
    boot_config_get(&boot_config);
    boot_config.offset = rd_offset;
    boot_config.offset1 = wr_offset;
    boot_config.state = BOOT_STATE_DECOMPRESS;
    boot_config.start_addr = p_object->fwB_download_addr;
    boot_config.size = p_object->fwB_size;
    return boot_config_set(&boot_config);
}
#endif /*defined(HTD_BOOT_IN_BOOTLOADER)*/


#if defined(HTD_BOOT_SUPPORT_COMPRESS_QUICKLZ)
//////////////////////////////////////////////////////////////////////
#include "quicklz.h"

// @brief quicklz 解压
bool boot_decompress_quicklz(htd_boot_object_t* p_object)
{
    boot_state_t state = boot_config_state_get();
    if (state != BOOT_STATE_DECOMPRESS && state != BOOT_STATE_DOWNLOAD_FINISH) {
        return false;
    }

    // 恢复进度
    boot_info_t boot_config;
    uint32_t rd_offset = sizeof(boot_header_t);
    uint32_t wr_offset = p_object->fwB_offset;
    boot_config_get(&boot_config);
    if (boot_config.offset > rd_offset) {
        rd_offset = boot_config.offset;
        wr_offset = boot_config.offset1;
        p_object->fwB_size = boot_config.size;
        p_object->fwB_download_addr = boot_config.start_addr;
    } 
    p_object->addrB_src = rd_offset + p_object->fwB_download_addr;
    p_object->addrB_dst = wr_offset + p_object->p_env->bankB_addr;
    
    // 打开覆盖的位置
    const boot_flash_handle_t * p_flash = boot_flash_handle_get(p_object->fwB_download_addr);
    ASSERT(p_flash);
    // 申请内存
    uint8_t *de_buff = (uint8_t*)htd_malloc(BOOT_DECOMPRESS_BUFFER_SIZE);
    ASSERT(de_buff);
    uint8_t *rd_buff = (uint8_t*)htd_malloc(BOOT_COMPRESS_BUFFER_SIZE + BOOT_COMPRESS_BUFFER_PADDING);
    ASSERT(rd_buff);
    qlz_state_decompress *p_decompress = (qlz_state_decompress*)htd_malloc(sizeof(qlz_state_decompress));
    ASSERT(p_decompress);
    htd_memset(p_decompress, 0, sizeof(qlz_state_decompress));

    boot_debug("[qlz]decompress start: fwB_download_addr=0x%08x, rd_offset=%u", p_object->fwB_download_addr, rd_offset);
    uint32_t opt_size;
    while (rd_offset < p_object->fwB_size) {
        // 读取块头部大小
        boot_flash_read(p_object->p_flash_fwB, p_object->addrB_src, (uint8_t*)&opt_size, sizeof(uint32_t));
        rd_offset += sizeof(uint32_t);
        p_object->addrB_src += sizeof(uint32_t);
        if (opt_size > BOOT_COMPRESS_BUFFER_SIZE + BOOT_COMPRESS_BUFFER_PADDING) {
            htd_free(de_buff);
            htd_free(rd_buff);
            htd_free(p_decompress);
            return false;
        }
        // 读取数据块
        boot_debug("rd_offset=%d, opt_size=0x%08x", rd_offset, opt_size);
        htd_memset(rd_buff, 0, BOOT_COMPRESS_BUFFER_SIZE + BOOT_COMPRESS_BUFFER_PADDING);
        boot_flash_read(p_object->p_flash_fwB, p_object->addrB_src, rd_buff, opt_size);
        rd_offset += opt_size;
        p_object->addrB_src += opt_size;
        
        // 解压数据块，并将解压后的数据块存储到 flash 中
        htd_memset(de_buff, 0, BOOT_DECOMPRESS_BUFFER_SIZE);
        opt_size = qlz_decompress((const char *)rd_buff, de_buff, p_decompress);
        boot_flash_erase(p_flash, p_object->addrB_dst, opt_size);
        boot_debug("wr_offset=0x%08x", wr_offset);
        boot_flash_write(p_flash, p_object->addrB_dst, de_buff, opt_size);
        wr_offset += opt_size;
        p_object->addrB_dst += opt_size;
        // 保存解压进度
        boot_config_decompress_progress_set(p_object, rd_offset, wr_offset);
    }

    htd_free(de_buff);
    htd_free(rd_buff);
    htd_free(p_decompress);
    p_object->fwB_download_addr = p_object->p_env->bankB_addr + p_object->fwB_offset;
    return true;
}
#endif
#if defined(HTD_BOOT_SUPPORT_COMPRESS_FASTLZ)
//////////////////////////////////////////////////////////////////////
#include "fastlz.h"

// @brief quicklz 解压
bool boot_decompress_fastlz(htd_boot_object_t* p_object)
{
    boot_state_t state = boot_config_state_get();
    if (state != BOOT_STATE_DECOMPRESS && state != BOOT_STATE_DOWNLOAD_FINISH) {
        return false;
    }

    // 恢复进度
    boot_info_t boot_config;
    uint32_t rd_offset = sizeof(boot_header_t);
    uint32_t wr_offset = p_object->fwB_offset;
    boot_config_get(&boot_config);
    if (boot_config.offset > rd_offset) {
        rd_offset = boot_config.offset;
        wr_offset = boot_config.offset1;
        p_object->fwB_size = boot_config.size;
        p_object->fwB_download_addr = boot_config.start_addr;
    }
    p_object->addrB_src = rd_offset + p_object->fwB_download_addr;
    p_object->addrB_dst = wr_offset + p_object->p_env->bankB_addr;
    
    // 打开覆盖的位置
    const boot_flash_handle_t * p_flash = boot_flash_handle_get(p_object->fwB_download_addr);
    ASSERT(p_flash);
    // 申请内存
    uint8_t *de_buff = (uint8_t*)htd_malloc(BOOT_DECOMPRESS_BUFFER_SIZE);
    ASSERT(de_buff);
    uint8_t *rd_buff = (uint8_t*)htd_malloc(BOOT_COMPRESS_BUFFER_SIZE + BOOT_COMPRESS_BUFFER_PADDING);
    ASSERT(rd_buff);

    boot_debug("[flz]decompress start: rd_offset=%u, fwB_size=%u, fwB_download_addr=0x%08x", rd_offset, p_object->fwB_size, p_object->fwB_download_addr);
    uint32_t opt_size;
    while (rd_offset < p_object->fwB_size) {
        // 读取块头部大小
        boot_flash_read(p_object->p_flash_fwB, p_object->addrB_src, (uint8_t*)&opt_size, sizeof(uint32_t));
        boot_debug("rd_offset=0x%08x, wr_offset=0x%08x, opt_size=0x%08x", rd_offset, wr_offset, opt_size);
        p_object->addrB_src += sizeof(uint32_t);
        rd_offset += sizeof(uint32_t);
        if (opt_size > BOOT_COMPRESS_BUFFER_SIZE + BOOT_COMPRESS_BUFFER_PADDING) {
            htd_free(de_buff);
            htd_free(rd_buff);
            return false;
        }
        // 读取数据块
        htd_memset(rd_buff, 0, BOOT_COMPRESS_BUFFER_SIZE + BOOT_COMPRESS_BUFFER_PADDING);
        boot_flash_read(p_object->p_flash_fwB, p_object->addrB_src, rd_buff, opt_size);
        p_object->addrB_src += opt_size;
        rd_offset += opt_size;
        
        // 解压数据块，并将解压后的数据块存储到 flash 中
        htd_memset(de_buff, 0, BOOT_DECOMPRESS_BUFFER_SIZE);
        opt_size = fastlz_decompress(rd_buff, opt_size, de_buff, BOOT_DECOMPRESS_BUFFER_SIZE);
        boot_flash_erase(p_flash, p_object->addrB_dst, opt_size);
        boot_flash_write(p_flash, p_object->addrB_dst, de_buff, opt_size);
        wr_offset += opt_size;
        p_object->addrB_dst += opt_size;
        // 保存解压进度
        boot_config_decompress_progress_set(p_object, rd_offset, wr_offset);
    }

    htd_free(de_buff);
    htd_free(rd_buff);
    p_object->fwB_download_addr = p_object->p_env->bankB_addr + p_object->fwB_offset;
    return true;
}
#endif

// @brief 解压
bool boot_decompress(htd_boot_object_t* p_object)
{
#if defined(HTD_BOOT_SUPPORT_COMPRESS_FASTLZ)
    if (boot_config_update_type_get() == BOOT_UPDATE_TYPE_COMPRESS_FASTLZ) {
        return boot_decompress_fastlz(p_object);
    }
#endif
#if defined(HTD_BOOT_SUPPORT_COMPRESS_QUICKLZ)
    if (boot_config_update_type_get() == BOOT_UPDATE_TYPE_COMPRESS_QUICKLZ) {
        return boot_decompress_quicklz(p_object);
    }
#endif /*defined(HTD_BOOT_UPDATE_TYPE_COMPRESS_QUICKLZ)*/
    return false;
}


