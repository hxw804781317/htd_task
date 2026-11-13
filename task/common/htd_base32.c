// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "htd_task.h"
#include "htd_base32.h"


//static const uint8_t* base32_table = "0123456789ACDEFGHJKLMNPQRTUVWXYZ";
static const uint8_t* base32_table = (const uint8_t*)"ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";


// @brief 编码一个字符
static uint8_t base32_encode_ch(uint8_t ch, uint8_t front, uint8_t fcount, char* out, uint8_t* remained)
{
    switch (fcount) {
        case 0: {    // front后0位 + ch前5位
            out[0] = base32_table[((ch >> 3) & 0x1F)];
            *remained = 3;
            return 1;
        }
        case 1: {    // front后1位 + ch前4位
            out[0] = base32_table[((front & 0x01) << 4) | ((ch >> 4) & 0x0F)];
            *remained = 4;
            return 1;
        }
        case 2: {    // front后2位 + ch前3位 + ch后5位
            out[0] = base32_table[((front & 0x03) << 3) | ((ch >> 5) & 0x07)];
            out[1] = base32_table[(ch & 0x1F)];
            *remained = 0;
            return 2;
        }
        case 3: {    // front后3位 + ch前2位 
            out[0] = base32_table[((front & 0x07) << 2) | ((ch >> 6) & 0x03)];
            out[1] = base32_table[((ch >> 1) & 0x1F)];
            *remained = 1;
            return 2;
        }
        case 4: {    // front后3位 + ch前1位
            out[0] = base32_table[((front & 0x0F) << 1) | ((ch >> 7) & 0x01)];
            out[1] = base32_table[((ch >> 2) & 0x1F)];
            *remained = 2;
            return 2;
        }
        default: {
            break;
        }
    }
    return 0;
}

// @brief base32 编码，每5个bit编一个码；3个字码编5个字码
// @return 成功返回编码后的长度，失败返回0
int htd_base32_encode(const uint8_t* in, int in_len, char* out, int out_size)
{
    int i;
    uint8_t front = 0;
    uint8_t fcount = 0;
    int offset = 0;
    uint8_t remained = 0;

    ASSERT(in && out && in_len > 0);
    
    for (i=0; i<in_len; i++) {
        ASSERT(offset + 2 < out_size);
        offset += base32_encode_ch(in[i], front, fcount, &out[offset], &remained);
        front = in[i];
        fcount = remained;
    }
    if (remained > 0) {
        out[offset++] = base32_table[(front << (5 - remained)) & 0x1F];
    }
    
    // 输出按8个字节对齐
    i = (8 - (offset & 0x07)) & 0x07;
    if (i > 0) {
        htd_memset(&out[offset], '=', i);
        offset += i;
    }
    return offset;
}

// @brief 解码一个字节
static uint8_t base32_decode_ch(char ch)
{
    if (ch > 'A' - 1 && ch < 'Z' + 1) {
        return ch - 'A';
    }
    if (ch > 'a' - 1 && ch < 'z' + 1) {
        return ch - 'a';
    }
    if (ch > '1' && ch < '8') {
        return ch - '2' + 26;
    }
    return 0;
}

// @brief base32 解码
// @return 成功返回编码后的长度，失败返回0
int htd_base32_decode(const char* in, int in_len, uint8_t* out, int out_size)
{
    int i;
    int offset = 0;
    uint8_t bit5val;
    uint8_t count = 0;
    
    ASSERT(in && out && in_len > 0);

    out[offset] = 0;
    for (i=0; i<in_len; i++) {
        count &= 0x07;
        bit5val = base32_decode_ch(in[i]);
        
        if (count <= 3) {
            out[offset] |= (bit5val << (3 - count));
            count += 5;
            if (count > 7) {
                out[++offset] = 0;
            }
        } else {
            out[offset] |= (bit5val >> (count - 3));
            out[++offset] = (bit5val << (11 - count));
            count += 5;
        }
    }
    return offset;
}


#if 0
int main(int argc, const char** argv)
{
    const char* buff = argv[1];
    char out[256];
    int outLen;
    char dst[256];
    int dstLen;

    if (argc != 2) {
        printf("use: %s string\r\n", argv[0]);
        return -1;
    }

    printf("buff=%s, strlen(buff)=%d\r\n", buff, strlen(buff));
    outLen = base32_encode(buff, strlen(buff), out, sizeof(out));
    out[outLen] = 0;
    printf("base32 encode[%d]: %s\r\n", outLen, out);

    dstLen = base32_decode(out, outLen, dst, sizeof(dst));
    dst[dstLen] = 0;
    printf("base32 decode[%d]: %s\r\n", dstLen, dst);

    return 0;
}
#endif




