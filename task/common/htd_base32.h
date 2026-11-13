// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_BASE32_H_
#define _HTD_BASE32_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


// @brief base32 编码，每5个bit编一个码；3个字码编5个字码
// @return 成功返回编码后的长度，失败返回0
int htd_base32_encode(const uint8_t* in, int in_len, char* out, int out_size);

// @brief base32 解码
// @return 成功返回编码后的长度，失败返回0
int htd_base32_decode(const char* in, int in_len, uint8_t* out, int out_size);


#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_BASE32_H_*/

