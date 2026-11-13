// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL           4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG           "aes"
#include "htd_device.h"
#include "htd_dev_rng.h"
#include "htd_sfw_aes128.h"


// @brief 加密数据(WEAK函数，芯片驱动可以重定义)
// @param input 必须为16字节
// @param key 必须为16字节
// @param output 必须为16字节
__WEAK int32_t htd_dev_aes128_encrypt(const uint8_t* input, const uint8_t* key, uint8_t *output)
{
    aes128_ecb_encrypt((uint8_t*)input, key, output);
    return 0;
}
// @brief 解密数据(WEAK函数，芯片驱动可以重定义)
// @param input 必须为16字节
// @param key 必须为16字节
// @param output 必须为16字节
__WEAK int32_t htd_dev_aes128_decrypt(const uint8_t* input, const uint8_t* key, uint8_t *output)
{
    aes128_ecb_decrypt((uint8_t*)input, key, output);
    return 0;
}

// @brief 加密数据, zeropadding
// @return 返回加密后的长度
static int aes128_ecb_encrypt_padding(const uint8_t* p_key, uint8_t* p_pad, const uint8_t* p_in, uint16_t in_len, uint8_t* p_out, uint16_t* out_len)
{
    uint8_t* p_start = p_out;
    while (in_len >= 16) {
        htd_dev_aes128_encrypt(p_in, p_key, p_out);
        p_out += 16;
        p_in += 16;
        in_len -= 16;
    }

    if (in_len) {
        htd_memcpy(p_pad, p_in, in_len);
    }
    htd_dev_aes128_encrypt((uint8_t*)p_pad, p_key, p_out);
    p_out += 16;

    if (out_len) {
        *out_len = p_out - p_start;
    }
    return p_out - p_start;
}


// @brief 加密数据, zeropadding
// @return 返回加密后的长度
int htd_aes128_ecb_zeropadding_encrypt(const uint8_t* p_in, uint16_t in_len, const uint8_t* p_key, uint8_t* p_out, uint16_t* out_len)
{
    uint8_t pad[16];
    htd_memset(pad, 0, sizeof(pad));
    return aes128_ecb_encrypt_padding(p_key, pad, p_in, in_len, p_out, out_len);
}

// @brief 解密数据，zeropadding
// @return 返回解密后的长度
int htd_aes128_ecb_zeropadding_decrypt(const uint8_t* p_in, uint16_t in_len, const uint8_t* p_key, uint8_t *p_out)
{
    uint16_t out_len;
    uint8_t* p_end;
    
    if (in_len & 15) {
        return 0;
    }

    out_len = in_len;
    while (in_len) {
        htd_dev_aes128_decrypt(p_in, p_key, p_out);
        p_in += 16;
        p_out += 16;
        in_len -= 16;
    }

    // 从后往前查看是否为0，最多16个
    p_end = p_out - 16;
    while (*p_end && p_end < p_out) {
        p_end++;
    }
    return out_len - (p_out - p_end);
}

// @brief 加密数据 pkc7padding
// @return 返回加密后的长度
int htd_aes128_ecb_pkc7padding_encrypt(const uint8_t* p_in, uint16_t in_len, const uint8_t* p_key, uint8_t* p_out, uint16_t* out_len)
{
    uint8_t pad[16];
    htd_memset(pad, (16 - (in_len & 0x0f)), sizeof(pad));
    return aes128_ecb_encrypt_padding(p_key, pad, p_in, in_len, p_out, out_len);
}

// @brief 解密数据 pkc7padding
// @return 返回解密后的长度
int htd_aes128_ecb_pkc7padding_decrypt(const uint8_t* p_in, uint16_t in_len, const uint8_t* p_key, uint8_t *p_out)
{
    uint16_t out_len = 0;
    if (in_len & 15) {
       return 0;
    }
    while (in_len) {
       htd_dev_aes128_decrypt(p_in, p_key, &p_out[out_len]);
       p_in += 16;
       out_len += 16;
       in_len -= 16;
    }
    //FIXME: 最后一个值必须小于等于16
    out_len -= p_out[out_len - 1];
    return out_len;
}

// @brief 加密数据 pkc5padding
int htd_aes128_ecb_pkc5padding_encrypt(const uint8_t* p_in, uint16_t in_len, const uint8_t* p_key, uint8_t* p_out, uint16_t* out_len)
{
    return htd_aes128_ecb_pkc7padding_encrypt(p_in, in_len, p_key, p_out, out_len);
}
// @brief 解密数据 pkc5padding
int htd_aes128_ecb_pkc5padding_decrypt(const uint8_t* p_in, uint16_t in_len, const uint8_t* p_key, uint8_t *p_out)
{
    return htd_aes128_ecb_pkc7padding_decrypt(p_in, in_len, p_key, p_out);
}

// @brief 加密数据 iso10126
// @return 返回加密后的长度
int htd_aes128_ecb_iso10126_encrypt(const uint8_t* p_in, uint16_t in_len, const uint8_t* p_key, uint8_t* p_out, uint16_t* out_len)
{
    uint8_t pad[16];
    htd_rng_data_get(pad, sizeof(pad));
    pad[15] = 16 - (in_len & 0x0f);
    return aes128_ecb_encrypt_padding(p_key, pad, p_in, in_len, p_out, out_len);
}

// @brief 解密数据 iso10126
// @return 返回解密后的长度
int htd_aes128_ecb_iso10126_decrypt(const uint8_t* p_in, uint16_t in_len, const uint8_t* p_key, uint8_t *p_out)
{
    return htd_aes128_ecb_pkc7padding_decrypt(p_in, in_len, p_key, p_out);
}

// @brief 加密数据 ANSIX923
// @return 返回加密后的长度
int htd_aes128_ecb_ansix923_encrypt(const uint8_t* p_in, uint16_t in_len, const uint8_t* p_key, uint8_t* p_out, uint16_t* out_len)
{
    uint8_t pad[16];
    htd_memset(pad, 0, sizeof(pad));
    pad[15] = 16 - (in_len & 0x0f);
    return aes128_ecb_encrypt_padding(p_key, pad, p_in, in_len, p_out, out_len);
}
// @brief 解密数据 ANSIX923
// @return 返回解密后的长度
int htd_aes128_ecb_ansix923_decrypt(const uint8_t* p_in, uint16_t in_len, const uint8_t* p_key, uint8_t *p_out)
{
    return htd_aes128_ecb_pkc7padding_decrypt(p_in, in_len, p_key, p_out);
}





//////////////////////////////////////////////////////////////////////
#if (defined(HTD_CONSOLE_ENABLE) && (DEBUG_LVL > 0) && 0)
#include "htd_console_process.h"

// @brief 调试命令
static int aes_cmd(int argc, char **argv)
{
    if (argc < 2) {
        print_error("Usage: %s zeropadding/pkc7padding/pkc5padding/iso10126/ansix923", argv[0]);
        return 0;
    }

    const uint8_t* p_data = (const uint8_t*)"0123456789";
    const uint8_t* p_key = (const uint8_t*)"abcdefghijklmnop";
    uint8_t out[16];
    uint16_t out_len = sizeof(out);
    uint8_t buff[16];
    uint32_t buff_len;

    print_hex("AES-key:", p_key, 16);
    print_hex("origin", p_data, 10);
    if (memcmp(argv[1], "zeropadding", 12) == 0) {
        htd_aes128_ecb_zeropadding_encrypt(p_data, 10, p_key, out, &out_len);
        print_hex("zeropadding", out, out_len);
        buff_len = htd_aes128_ecb_zeropadding_decrypt(out, out_len, p_key, buff);
        print_hex("data", buff, buff_len);
        return 0;
    }
    if (memcmp(argv[1], "pkc7padding", 12) == 0) {
        htd_aes128_ecb_pkc7padding_encrypt(p_data, 10, p_key, out, &out_len);
        print_hex("pkc7padding", out, out_len);
        buff_len = htd_aes128_ecb_pkc7padding_decrypt(out, out_len, p_key, buff);
        print_hex("data", buff, buff_len);
        return 0;
    }
    if (memcmp(argv[1], "pkc5padding", 12) == 0) {
        htd_aes128_ecb_pkc5padding_encrypt(p_data, 10, p_key, out, &out_len);
        print_hex("pkc5padding", out, out_len);
        buff_len = htd_aes128_ecb_pkc5padding_decrypt(out, out_len, p_key, buff);
        print_hex("data", buff, buff_len);
        return 0;
    }
    if (memcmp(argv[1], "iso10126", 9) == 0) {
        htd_aes128_ecb_iso10126_encrypt(p_data, 10, p_key, out, &out_len);
        print_hex("iso10126", out, out_len);
        buff_len = htd_aes128_ecb_iso10126_decrypt(out, out_len, p_key, buff);
        print_hex("data", buff, buff_len);
        return 0;
    }
    if (memcmp(argv[1], "ansix923", 9) == 0) {
        htd_aes128_ecb_ansix923_encrypt(p_data, 10, p_key, out, &out_len);
        print_hex("ansix923", out, out_len);
        buff_len = htd_aes128_ecb_ansix923_decrypt(out, out_len, p_key, buff);
        print_hex("data", buff, buff_len);
        return 0;
    }
    print_error("Usage: %s zeropadding/pkc7padding/pkc5padding/iso10126/ansix923", argv[0]);
    return 0;
}
CONSOLE_CMD_EXPORT(aes_cmd, aes);
#endif /*(defined(HTD_CONSOLE_ENABLE) && (DEBUG_LVL > 0))*/

