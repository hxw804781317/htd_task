// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_BASE64_H_
#define _HTD_BASE64_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


// @brief: base64 解码，将可视字符解码成数据
// @return: 成功返回解码后的数据长度，失败返回-1
int htd_base64_decode(const uint8_t *p_src, uint32_t src_len, uint8_t* p_dst, uint32_t dst_size);
// @brief: base64 编码，将数据编成可视字符
// @return: 成功返回编码后的数据长度，失败返回-1
int htd_base64_encode(const uint8_t *p_src, uint32_t src_len, uint8_t* p_dst, int dst_size);


#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_BASE64_H_*/

