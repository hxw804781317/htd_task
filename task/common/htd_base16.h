// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_BASE16_H_
#define _HTD_BASE16_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


// @brief 进行 base16 编码；0x01->"01", 0xab->"ab"
// @return 成功返回编码后的长度，失败返回0
int htd_base16_encode(const uint8_t* p_in, uint32_t in_len, uint8_t* p_out, uint32_t out_size);

// @brief base16 解码；"01ab"->0x01ab
// @return 成功返回编码后的长度，失败返回0
int htd_base16_decode(const uint8_t* p_in, uint32_t in_len, uint8_t* p_out, uint32_t out_size);


#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_BASE16_H_*/

