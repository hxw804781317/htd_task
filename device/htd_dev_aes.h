// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_DEV_AES_H_
#define _HTD_DEV_AES_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/



// @brief 加密数据(WEAK函数，芯片驱动可以重定义)
// @param input 必须为16字节
// @param key 必须为16字节
// @param output 必须为16字节
int32_t htd_dev_aes128_encrypt(const uint8_t* input, const uint8_t* key, uint8_t *output);
// @brief 解密数据(WEAK函数，芯片驱动可以重定义)
// @param input 必须为16字节
// @param key 必须为16字节
// @param output 必须为16字节
int32_t htd_dev_aes128_decrypt(const uint8_t* input, const uint8_t* key, uint8_t *output);



// @brief 加密数据, zeropadding
// @return 返回加密后的长度
int htd_aes128_ecb_zeropadding_encrypt(const uint8_t* p_in, uint16_t in_len, const uint8_t* p_key, uint8_t* p_out, uint16_t* out_len);
// @brief 解密数据，zeropadding
// @return 返回解密后的长度
int htd_aes128_ecb_zeropadding_decrypt(const uint8_t* p_in, uint16_t in_len, const uint8_t* p_key, uint8_t *p_out);

// @brief 加密数据 pkc7padding
// @return 返回加密后的长度
int htd_aes128_ecb_pkc7padding_encrypt(const uint8_t* p_in, uint16_t in_len, const uint8_t* p_key, uint8_t* p_out, uint16_t* out_len);
// @brief 解密数据 pkc7padding
// @return 返回解密后的长度
int htd_aes128_ecb_pkc7padding_decrypt(const uint8_t* p_in, uint16_t in_len, const uint8_t* p_key, uint8_t *p_out);

// @brief 加密数据 pkc5padding
int htd_aes128_ecb_pkc5padding_encrypt(const uint8_t* p_in, uint16_t in_len, const uint8_t* p_key, uint8_t* p_out, uint16_t* out_len);
// @brief 解密数据 pkc5padding
int htd_aes128_ecb_pkc5padding_decrypt(const uint8_t* p_in, uint16_t in_len, const uint8_t* p_key, uint8_t *p_out);

// @brief 加密数据 iso10126
// @return 返回加密后的长度
int htd_aes128_ecb_iso10126_encrypt(const uint8_t* p_in, uint16_t in_len, const uint8_t* p_key, uint8_t* p_out, uint16_t* out_len);
// @brief 解密数据 iso10126
// @return 返回解密后的长度
int htd_aes128_ecb_iso10126_decrypt(const uint8_t* p_in, uint16_t in_len, const uint8_t* p_key, uint8_t *p_out);

// @brief 加密数据 ANSIX923
// @return 返回加密后的长度
int htd_aes128_ecb_ansix923_encrypt(const uint8_t* p_in, uint16_t in_len, const uint8_t* p_key, uint8_t* p_out, uint16_t* out_len);
// @brief 解密数据 ANSIX923
// @return 返回解密后的长度
int htd_aes128_ecb_ansix923_decrypt(const uint8_t* p_in, uint16_t in_len, const uint8_t* p_key, uint8_t *p_out);



#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_DEV_AES_H_*/


