// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_DEV_HASH_H_
#define _HTD_DEV_HASH_H_

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "htd_sfw_sha256.h"
#include "htd_sfw_sha1.h"
#include "htd_sfw_md5.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/



//////////////////////////////////////////////////////////////////////
// @brief 初始化(WEAK 函数, 硬件实现)
int32_t htd_dev_sha1_init(htd_sha1_ctx_t *p_ctx);
// @brief 更新 sha1 (WEAK 函数, 硬件实现)
void htd_dev_sha1_update(htd_sha1_ctx_t *p_ctx, const uint8_t* p_data, uint32_t data_len);
// @brief 获取 sha1 结果(WEAK 函数, 硬件实现)
void htd_dev_sha1_final(htd_sha1_ctx_t *p_ctx, uint8_t output[20]);


// @brief 初始化 sha1; 优先使用硬件，如果没有硬件或者硬件正在运行，自动使用软件
void htd_sha1_init(htd_sha1_ctx_t *p_ctx);
// @brief 更新 sha1
void htd_sha1_update(htd_sha1_ctx_t *p_ctx, const uint8_t* p_data, uint32_t data_len);
// @brief 获取 sha1 结果
void htd_sha1_final(htd_sha1_ctx_t *p_ctx, uint8_t output[20]);
// @brief sha1
void htd_sha1(const uint8_t* p_data, uint32_t data_len, uint8_t output[20]);
// @brief sha1 加密计算
void htd_sha1_hmac(const uint8_t *p_key, int key_len, const uint8_t *p_data, int data_len, uint8_t out[20]);





//////////////////////////////////////////////////////////////////////
// @brief 初始化(WEAK 函数, 硬件实现)
int32_t htd_dev_sha256_init(htd_sha256_ctx_t *p_ctx);
// @brief 更新 sha256 (WEAK 函数, 硬件实现)
void htd_dev_sha256_update(htd_sha256_ctx_t *p_ctx, const uint8_t* p_data, uint32_t data_len);
// @brief 获取 sha256 结果(WEAK 函数, 硬件实现)
void htd_dev_sha256_final(htd_sha256_ctx_t *p_ctx, uint8_t output[32]);


// @brief 初始化 sha256; 优先使用硬件，如果没有硬件或者硬件正在运行，自动使用软件
void htd_sha256_init(htd_sha256_ctx_t *p_ctx);
// @brief 更新 sha256
void htd_sha256_update(htd_sha256_ctx_t *p_ctx, const uint8_t* p_data, uint32_t data_len);
// @brief 获取 sha256 结果
void htd_sha256_final(htd_sha256_ctx_t *p_ctx, uint8_t output[32]);
// @brief sha256
void htd_sha256(const uint8_t* p_data, uint32_t data_len, uint8_t output[32]);
// @brief sha256 加密计算
void htd_sha256_hmac(const uint8_t *p_key, int key_len, const uint8_t *p_data, int data_len, uint8_t out[32]);




//////////////////////////////////////////////////////////////////////
// @brief 初始化(WEAK 函数)
int32_t htd_dev_md5_init(htd_md5_ctx_t *p_ctx);
// @brief 更新 sha1 (WEAK 函数)
void htd_dev_md5_update(htd_md5_ctx_t *p_ctx, const uint8_t* p_data, uint32_t data_len);
// @brief 获取 sha1 结果(WEAK 函数)
void htd_dev_md5_final(htd_md5_ctx_t *p_ctx, uint8_t output[16]);


// @brief 初始化 md5; 优先使用硬件，如果没有硬件或者硬件正在运行，自动使用软件
void htd_md5_init(htd_md5_ctx_t *p_ctx);
// @brief 更新 md5
void htd_md5_update(htd_md5_ctx_t *p_ctx, const uint8_t* p_data, uint32_t data_len);
// @brief 获取 md5 结果
void htd_md5_final(htd_md5_ctx_t *p_ctx, uint8_t output[16]);
// @brief md5
void htd_md5(const uint8_t* p_data, uint32_t data_len, uint8_t output[16]);
// @brief md5 加密计算
void htd_md5_hmac(const uint8_t *p_key, int key_len, const uint8_t *p_data, int data_len, uint8_t out[16]);


#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_DEV_HASH_H_*/

