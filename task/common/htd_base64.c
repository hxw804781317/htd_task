// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "htd_task.h"
#include "htd_base64.h"


static const char* base64_table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static uint8_t base64_decode_byte(uint8_t code)
{
    if (code >= 'A' && code <= 'Z') {
        return (code - 'A');
    } else if (code >= 'a' && code <= 'z') {
        return ((code - 'a') + 26);
    } else if (code >= '0' && code <= '9') {
        return ((code - '0') + 52);
    } else if (code == '+') {
        return 62;
    } else if (code == '/') {
        return 63;
    } else {
        return 64;
    }
}

// @brief: base64 解码，将可视字符解码成数据
// @return: 成功返回解码后的数据长度，失败返回-1
int htd_base64_decode(const uint8_t *p_src, uint32_t src_len, uint8_t* p_dst, uint32_t dst_size)
{
    uint32_t offset = 0;
    uint8_t high_bits = 0;
    uint8_t ch;
    uint8_t pos;
    
    if (NULL == p_src || NULL == p_dst || dst_size <= 0) {
        return -1;
    }

    pos = 0;
    while (src_len--) {
        if (offset >= dst_size) {
            break;
        }
        
        ch = *p_src++;
        if (ch == '\r' || ch == '\n') {
            continue;
        }
        ch = base64_decode_byte(ch);
        if (ch >= 64) {
            break;
        }
        switch (pos) {
            case 0:
                high_bits = ch << 2;
                break;
            case 1:
                p_dst[offset++] = high_bits | (ch >> 4);
                high_bits = ch << 4;
                break;
            case 2:
                p_dst[offset++] = high_bits | (ch >> 2);
                high_bits = ch << 6;
                break;
            default:
                p_dst[offset++] = high_bits | ch;
                break;
        }
        pos = (pos + 1) & 0x03;
    }
    return (offset);
}

// @brief: base64 编码，将数据编成可视字符
// @return: 成功返回编码后的数据长度，失败返回-1
int htd_base64_encode(const uint8_t *p_src, uint32_t src_len, uint8_t* p_dst, int dst_size)
{
    int offset = 0;
    int nfrom;
    uint8_t ch;
    uint8_t high_bits = 0;

    if (NULL == p_src || NULL == p_dst || dst_size <= 0) {
        return -1;
    }

    nfrom = 0;
    while (src_len--) {
        if (offset >= dst_size) {
            break;
        }

        ch = *p_src++;
        switch (nfrom) {
            case 0: {
                p_dst[offset++] = base64_table[ch >> 2];
                high_bits = (ch << 4) & 0x30;
                nfrom = 1;
                break;
            }
            case 1: {
                p_dst[offset++] = base64_table[high_bits | (ch >> 4)];
                high_bits = (ch << 2) & 0x3c;
                nfrom = 2;
                break;
            }
            default: {
                p_dst[offset++] = base64_table[high_bits | (ch >> 6)];
                if (offset < dst_size) {
                    p_dst[offset++] = base64_table[ch & 0x3f];
                }
                nfrom = 0;
                break;
            }
        }
    }

    if (nfrom && offset < dst_size) {
        p_dst[offset++] = base64_table[high_bits];
        nfrom = 3 - nfrom;
        if (offset+nfrom <= dst_size) {
            htd_memset(p_dst+offset, '=', (unsigned long)nfrom);
            offset += nfrom;
        }
    }
    p_dst[offset++] = 0;
    return (offset);
}




//////////////////////////////////////////////////////////////////////
#if 0
int main()
{
    uint8_t buff[] = "123456a0bcdefghijklmn";
    uint8_t dst[128];
    uint8_t src[128];
    int size;
    int  i;

    size = base64_encode(buff, strlen(buff), dst, 128);
    for (i=0; i<size; i++) {
        printf("%c", dst[i]);
    }
    printf("\r\n");

    size = base64_decode(dst, size+3, src, 128);
    for (i=0; i<size; i++) {
        printf("%c", src[i]);
    }
    printf("\r\n");
    return 0;
}
#endif




