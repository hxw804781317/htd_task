// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_BOOT_PROCESS_H_
#define _HTD_BOOT_PROCESS_H_

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "htd_boot_port.h"
#include "htd_boot_config.h"
#include "htd_boot_ymodem.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


// boot 头部信息, 80字节
typedef struct {
    uint32_t magic;                 // 魔数，固件和boot必须一致
    uint8_t verify_code[64];        // 使用私钥对sha256的值做签名，使用公钥验证
    uint32_t fw_size;               // 固件大小，单位字节，包含所有头部数据

    uint16_t fw_hdr_size;           // 固件头部大小，sizeof(boot_firmware_header_t)
    boot_update_type_t update_type; // 升级方式, 固件类型
    uint8_t reserve0;
    
    uint8_t ver_major;              // 主版本号
    uint8_t ver_minor;              // 次版本号
    uint8_t ver_revision;           // 修订版本
    uint8_t ver_reserve;            // 保留版本号
}boot_header_t;
#define HTD_BOOT_MAGIC              0xae667182
#define HTD_BOOT_VERIFY_CODE_OFFSET (uint32_t)(&((boot_header_t*)0)->verify_code)


// 固件头部信息, 该结构体大小必须为512字节
typedef struct {
    boot_header_t boot_hdr;
    uint32_t fw_run_addr;           // 固件运行地址, 在内存中启动时需要用到
    
    // 固件端保持 512 字节对齐
    uint8_t reserve1[512-sizeof(boot_header_t)-4];
}boot_firmware_header_t;


// 环境信息
typedef struct {
    uint32_t bankA_addr;            // 固件A区的地址
    uint32_t bankA_size;
    uint32_t bankB_addr;            // 固件B区的地址
    uint32_t bankB_size;
} htd_boot_env_t;

// 
typedef struct {
    uint32_t version;

    uint16_t fwB_offset;            // 固件B偏移
    uint16_t image_hdr_len;         // 固件头部大小
    uint32_t fwA_size;              // 固件A的大小, 没有包含头部
    
    uint32_t fwB_download_addr;     // 固件B下载的地址
    uint32_t fwB_size;              // 固件B的大小, 没有包含头部

    htd_boot_env_t* p_env;
    const boot_flash_handle_t* p_flash_fwA;      // 固件A区操作句柄
    const boot_flash_handle_t* p_flash_fwB;      // 固件B区操作句柄
    uint32_t addrA_src, addrB_src;
    uint32_t addrA_dst, addrB_dst;
    uint8_t swap_data[HTD_BOOT_OVERWRITE_UNIT];
}htd_boot_object_t;

// @brief 获取环境信息
htd_boot_env_t* htd_boot_env_info_get(void);
// @brief boot 主流程
bool htd_boot_process(uint32_t* program_addr);
// @brief 获取全量升级下载地址
uint32_t htd_boot_full_download_addr_get(void);
// @brief 获取压缩升级下载地址
uint32_t htd_boot_lz_download_addr_get(uint32_t filesize);
// @brief 下载固件流程, while 循环不断调用，返回1则代表下载完成
uint32_t htd_boot_download_process(void);


#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_BOOT_PROCESS_H_*/

