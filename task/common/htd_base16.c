// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "htd_task.h"
#include "htd_base16.h"


// @brief base16 编码, 将16进制数据编码成可视字符；每4个bit编一个字节；0x01->"01", 0xab->"ab"
// @return 成功返回编码后的长度，失败返回0
int htd_base16_encode(const uint8_t* p_in, uint32_t in_len, uint8_t* p_out, uint32_t out_size)
{
    const static unsigned char base16_encode_table[] = {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
    };
    int offset = 0;
    uint8_t temp;

    ASSERT(p_in && p_out && in_len > 0);
    ASSERT(out_size >= (in_len * 2));
    
    while (in_len--) {
        temp = *p_in++;
        p_out[offset++] = base16_encode_table[(temp >> 4) & 0x0F];
        p_out[offset++] = base16_encode_table[temp & 0x0F];
    }
    return offset;
}

// @brief base16 解码；"01ab"->0x01ab
// @return 成功返回编码后的长度，失败返回0
int htd_base16_decode(const uint8_t* p_in, uint32_t in_len, uint8_t* p_out, uint32_t out_size)
{
    static const unsigned char base16_decode_table[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, /*8 = '0'-'7'*/
        0x08, 0x09, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, /*2 = '8'-'9'*/
        0xFF, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0xFF, /*6 = 'A'-'F'*/
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0xFF  /*6 = 'a'-'f' (same as 'A'-'F')*/
    };
    uint8_t hi, lo;
    uint32_t i;

    ASSERT(p_in && p_out && in_len > 0);
    ASSERT(out_size >= (in_len / 2));

    in_len >>= 1;
    for (i=0; i<in_len; ++i) {
        hi = *p_in++;
        lo = *p_in++;
        hi = (hi & 0x7f) - 0x30;
        lo = (lo & 0x7f) - 0x30;
        *p_out++ = ((base16_decode_table[hi] << 4) & 0xf0) | (base16_decode_table[lo] & 0x0f);
    }
    return in_len;
}


