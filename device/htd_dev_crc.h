// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_DEV_CRC_H_
#define _HTD_DEV_CRC_H_

#include <stdint.h>
#include <stdbool.h>
#include "htd_sfw_crc.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


//////////////////////////////////////////////////////////////////////
typedef enum {
    HTD_CRC8_DEF,
    HTD_CRC8_ITU,
    HTD_CRC8_ROHC,
    HTD_CRC8_MAXIM,
}htd_crc8_type_t;

typedef struct {
    uint16_t result;
    htd_crc8_type_t crc8_type;
    uint8_t user_hw_flag;
} htd_crc8_ctx_t;
const static htd_crc8_ctx_t m_crc8_ctx_def = {.result = 0, .crc8_type = HTD_CRC8_DEF};
const static htd_crc8_ctx_t m_crc8_ctx_itu_def = {.result = 0, .crc8_type = HTD_CRC8_ITU};
const static htd_crc8_ctx_t m_crc8_ctx_rohc_def = {.result = 0xff, .crc8_type = HTD_CRC8_ROHC};
const static htd_crc8_ctx_t m_crc8_ctx_maxim_def = {.result = 0, .crc8_type = HTD_CRC8_MAXIM};



// @brief 开始(WEAK 函数, 驱动重定义)
int32_t htd_dev_crc8_init(htd_crc8_ctx_t* p_ctx);
// @brief 计算(WEAK 函数, 驱动重定义)
uint16_t htd_dev_crc8_update(htd_crc8_ctx_t* p_ctx, const uint8_t* p_data, uint32_t data_len);
// @brief 结束(WEAK 函数, 驱动重定义)
int32_t htd_dev_crc8_final(htd_crc8_ctx_t* p_ctx);


// @brief 开始crc8; 优先使用硬件，如果没有硬件或者硬件正在运行，自动使用软件
int32_t htd_crc8_init(htd_crc8_ctx_t* p_ctx);
// @brief 计算
uint16_t htd_crc8_update(htd_crc8_ctx_t* p_ctx, const uint8_t* p_data, uint32_t data_len);
// @brief 结束
int32_t htd_crc8_final(htd_crc8_ctx_t* p_ctx);





//////////////////////////////////////////////////////////////////////
typedef enum {
    HTD_CRC16_CCITT_FALSE,
    HTD_CRC16_CCITT,
    HTD_CRC16_X25,
    HTD_CRC16_XMODEM,
    HTD_CRC16_MODBUS,
}htd_crc16_type_t;

typedef struct {
    uint16_t result;
    htd_crc16_type_t crc16_type;
    uint8_t user_hw_flag;
} htd_crc16_ctx_t;
const static htd_crc16_ctx_t m_crc16_ctx_ccitt_false_def = {.result = 0xffff, .crc16_type = HTD_CRC16_CCITT_FALSE};
const static htd_crc16_ctx_t m_crc16_ctx_ccitt_def = {.result = 0x0000, .crc16_type = HTD_CRC16_CCITT};
const static htd_crc16_ctx_t m_crc16_ctx_x25_def = {.result = 0xffff, .crc16_type = HTD_CRC16_X25};
const static htd_crc16_ctx_t m_crc16_ctx_xmodem_def = {.result = 0x0000, .crc16_type = HTD_CRC16_XMODEM};
const static htd_crc16_ctx_t m_crc16_ctx_modbus_def = {.result = 0xffff, .crc16_type = HTD_CRC16_MODBUS};


// @brief 开始(WEAK 函数, 驱动重定义)
int32_t htd_dev_crc16_init(htd_crc16_ctx_t* p_ctx);
// @brief 计算(WEAK 函数, 驱动重定义)
uint16_t htd_dev_crc16_update(htd_crc16_ctx_t* p_ctx, const uint8_t* p_data, uint32_t data_len);
// @brief 结束(WEAK 函数, 驱动重定义)
uint16_t htd_dev_crc16_final(htd_crc16_ctx_t* p_ctx);


// @brief 开始CRC16; 优先使用硬件，如果没有硬件或者硬件正在运行，自动使用软件
int32_t htd_crc16_init(htd_crc16_ctx_t* p_ctx);
// @brief 计算
uint16_t htd_crc16_update(htd_crc16_ctx_t* p_ctx, const uint8_t* p_data, uint32_t data_len);
// @brief 结束
uint16_t htd_crc16_final(htd_crc16_ctx_t* p_ctx);




//////////////////////////////////////////////////////////////////////
typedef enum {
    HTD_CRC32_GEN,
    HTD_CRC32_MPEG2,
}htd_crc32_type_t;

typedef struct {
    uint32_t result;
    htd_crc32_type_t crc32_type;
    uint8_t user_hw_flag;
} htd_crc32_ctx_t;
const static htd_crc32_ctx_t m_crc32_ctx_gen_def = {.result = 0xffffffff, .crc32_type = HTD_CRC32_GEN};
const static htd_crc32_ctx_t m_crc32_ctx_mpeg2_def = {.result = 0xffffffff, .crc32_type = HTD_CRC32_MPEG2};

// @brief 开始(WEAK 函数, 驱动重定义)
int32_t htd_dev_crc32_init(htd_crc32_ctx_t* p_ctx);
// @brief 计算(WEAK 函数, 驱动重定义)
uint32_t htd_dev_crc32_update(htd_crc32_ctx_t* p_ctx, const uint8_t* p_data, uint32_t data_len);
// @brief 结束(WEAK 函数, 驱动重定义)
int32_t htd_dev_crc32_final(htd_crc32_ctx_t* p_ctx);


// @brief 开始; 优先使用硬件，如果没有硬件或者硬件正在运行，自动使用软件
int32_t htd_crc32_init(htd_crc32_ctx_t* p_ctx);
// @brief 计算
uint32_t htd_crc32_update(htd_crc32_ctx_t* p_ctx, const uint8_t* p_data, uint32_t data_len);
// @brief 结束
uint32_t htd_crc32_final(htd_crc32_ctx_t* p_ctx);



#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_DEV_CRC_H_*/

