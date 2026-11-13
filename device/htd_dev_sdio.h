// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_DEV_SDIO_H_
#define _HTD_DEV_SDIO_H_

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


//////////////////////////////////////////////////////////////////////
typedef struct {
    uint32_t card_type; 
    uint32_t card_version; 
    uint32_t card_class;
    uint32_t rel_card_addr;
    uint32_t block_count;
    uint32_t block_size; 
} dev_sdcard_info_t;

typedef struct {
    uint32_t block_idx;
    uint32_t block_count;
} htd_sdio_erase_param_t;

typedef struct {
    uint32_t block_idx;
    uint8_t* p_data;
    uint32_t block_count;
} htd_sdio_xfer_t;


// 控制命令
#define HTD_SDIO_CMD_NONE               0
#define HTD_SDIO_CMD_ERASE              1               // 擦除, @htd_sdcard_erase_param_t
#define HTD_SDIO_CMD_INFO_GET           2               // 获取信息，@htd_sdcard_info_t
#define HTD_SDIO_CMD_OPEN               3               // 
#define HTD_SDIO_CMD_CLOSE              4               // 
#define HTD_SDIO_CMD_READ               5               // 
#define HTD_SDIO_CMD_WRITE              6               // 
typedef uint32_t sdcard_cmd_t;

//  设备
typedef struct {
    const htd_dev_t *p_dev;
    pin_num_t pin_cmd;
    pin_num_t pin_sck;
    pin_num_t pin_d0;
    pin_num_t pin_d1;
    pin_num_t pin_d2;
    pin_num_t pin_d3;
} sdio_handle_t;
#define HTD_SDIO_DEV_IDENTIFY           (HTD_DEV_MAGIC_MASK | 0xa55f)

// 配置
typedef struct {
    pin_num_t pin_cmd;
    pin_num_t pin_sck;
    pin_num_t pin_d0;
    pin_num_t pin_d1;
    pin_num_t pin_d2;
    pin_num_t pin_d3;
} sdio_user_config_t;




//////////////////////////////////////////////////////////////////////
// @brief 打开
sdio_handle_t* htd_dev_sdio_open(const char* p_name, const sdio_user_config_t* p_user_config);
// @brief 关闭 
int32_t htd_dev_sdio_close(sdio_handle_t* p_handle);
// @brief 读块数据
// @return 成功返回0，失败返回对应的错误码
int32_t htd_dev_sdio_block_read(sdio_handle_t* p_handle, uint32_t block_idx, uint8_t* p_data, uint32_t block_count);
// @brief 写块数据
// @return 成功返回0，失败返回对应的错误码
int32_t htd_dev_sdio_block_write(sdio_handle_t* p_handle, uint32_t block_idx, const uint8_t* p_data, uint32_t block_count);
// @brief 擦除
// @return 成功返回0，失败返回对应的错误码
int32_t htd_dev_sdio_block_erase(sdio_handle_t* p_handle, uint32_t block_idx, uint32_t block_count);
// @brief 获取 SD 卡信息
int32_t htd_dev_sdio_info_get(sdio_handle_t* p_handle, dev_sdcard_info_t* p_info);


#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_DEV_SDIO_H_*/

