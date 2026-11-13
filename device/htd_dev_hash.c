// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL           2       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG           "dev.hash"
#include "htd_device.h"
#include "htd_dev_hash.h"


#if 1
//////////////////////////////////////////////////////////////////////
typedef struct {
    uint32_t magic1;
    uint32_t magic2;
}htd_sha1_info_t;
#define HTD_SHA1_MAGIC_VAL        0xAA313131

// @brief 初始化(WEAK 函数, 硬件实现)
__WEAK int32_t htd_dev_sha1_init(htd_sha1_ctx_t *p_ctx)
{
    return ERROR_CODE_FAILED;
}
// @brief 更新 sha1 (WEAK 函数, 硬件实现)
__WEAK void htd_dev_sha1_update(htd_sha1_ctx_t *p_ctx, const uint8_t* p_data, uint32_t data_len)
{
}   
// @brief 获取 sha1 结果(WEAK 函数, 硬件实现)
__WEAK void htd_dev_sha1_final(htd_sha1_ctx_t *p_ctx, uint8_t output[20])
{
}

// @brief 初始化 sha1; 优先使用硬件，如果没有硬件或者硬件正在运行，自动使用软件
void htd_sha1_init(htd_sha1_ctx_t *p_ctx)
{
    ASSERT(p_ctx);
    if (htd_dev_sha1_init(p_ctx) == ERROR_CODE_SUCCESS) {
        // 使用硬件进行处理
        htd_sha1_info_t* p_info = (htd_sha1_info_t*)p_ctx;
        p_info->magic1 = HTD_SHA1_MAGIC_VAL;
        p_info->magic2 = HTD_SHA1_MAGIC_VAL;
        return ;
    }

    // 使用软件进行处理
    htd_sfw_sha1_starts(p_ctx);
}
// @brief 更新 sha1
void htd_sha1_update(htd_sha1_ctx_t *p_ctx, const uint8_t* p_data, uint32_t data_len)
{
    ASSERT(p_ctx);
    htd_sha1_info_t* p_info = (htd_sha1_info_t*)p_ctx;
    if (p_info->magic1 == HTD_SHA1_MAGIC_VAL && p_info->magic2 == HTD_SHA1_MAGIC_VAL) {
        // 使用硬件进行处理
        htd_dev_sha1_update(p_ctx, p_data, data_len);
        return;
    }
    // 使用软件进行处理
    htd_sfw_sha1_update(p_ctx, p_data, data_len);
}
// @brief 获取 sha1 结果
void htd_sha1_final(htd_sha1_ctx_t *p_ctx, uint8_t output[20])
{
    ASSERT(p_ctx);
    htd_sha1_info_t* p_info = (htd_sha1_info_t*)p_ctx;
    if (p_info->magic1 == HTD_SHA1_MAGIC_VAL && p_info->magic2 == HTD_SHA1_MAGIC_VAL) {
        // 使用硬件进行处理
        htd_dev_sha1_final(p_ctx, output);
        return;
    }
    // 使用软件进行处理
    htd_sfw_sha1_finish(p_ctx, output);
}
// @brief sha1
void htd_sha1(const uint8_t* p_data, uint32_t data_len, uint8_t output[20])
{
    htd_sha1_ctx_t sha1_ctx;
    htd_sha1_init(&sha1_ctx);
    htd_sha1_update(&sha1_ctx, p_data, data_len);
    htd_sha1_final(&sha1_ctx, output);
}
// @brief sha1 加密计算
void htd_sha1_hmac(const uint8_t *p_key, int key_len, const uint8_t *p_data, int data_len, uint8_t out[20]) 
{
    htd_sha1_ctx_t ctx;
    uint8_t temp[20];
    uint8_t key2[20];
    uint8_t buff[64];
    int i;

    if (key_len > sizeof(buff)) {
        htd_sha1_init(&ctx);
        htd_sha1_update(&ctx, p_key, key_len);
        htd_sha1_final(&ctx, key2);
        p_key = key2;
        key_len = sizeof(key2);
    }

    for (i=0; i<key_len; ++i) {
        buff[i] = p_key[i] ^ 0x36;
    }
    for (i=key_len; i<sizeof(buff); ++i) {
        buff[i] = 0 ^ 0x36;
    }
    
    htd_sha1_init(&ctx);
    htd_sha1_update(&ctx, buff, sizeof(buff));
    htd_sha1_update(&ctx, p_data, data_len);
    htd_sha1_final(&ctx, temp);

    for (i=0; i<key_len; ++i) {
        buff[i] = p_key[i] ^ 0x5c;
    }
    for (i=key_len; i<sizeof(buff); ++i) {
        buff[i] = 0 ^ 0x5c;
    }
    htd_sha1_init(&ctx);
    htd_sha1_update(&ctx, buff, sizeof(buff));
    htd_sha1_update(&ctx, temp, sizeof(temp));
    htd_sha1_final(&ctx, out);
}




//////////////////////////////////////////////////////////////////////
typedef struct {
    uint32_t magic1;
    uint32_t magic2;
}htd_sha256_info_t;
#define HTD_SHA256_MAGIC_VAL        0xAA323536

// @brief 初始化(WEAK 函数, 硬件实现)
__WEAK int32_t htd_dev_sha256_init(htd_sha256_ctx_t *p_ctx)
{
    return ERROR_CODE_FAILED;
}
// @brief 更新 sha256 (WEAK 函数, 硬件实现)
__WEAK void htd_dev_sha256_update(htd_sha256_ctx_t *p_ctx, const uint8_t* p_data, uint32_t data_len)
{
}
// @brief 获取 sha256 结果(WEAK 函数, 硬件实现)
__WEAK void htd_dev_sha256_final(htd_sha256_ctx_t *p_ctx, uint8_t output[32])
{
}

// @brief 初始化 sha256; 优先使用硬件，如果没有硬件或者硬件正在运行，自动使用软件
void htd_sha256_init(htd_sha256_ctx_t *p_ctx)
{
    ASSERT(p_ctx);
    if (htd_dev_sha256_init(p_ctx) == ERROR_CODE_SUCCESS) {
        // 使用硬件进行处理
        htd_sha256_info_t* p_info = (htd_sha256_info_t*)p_ctx;
        p_info->magic1 = HTD_SHA256_MAGIC_VAL;
        p_info->magic2 = HTD_SHA256_MAGIC_VAL;
        return ;
    }

    // 使用软件进行处理
    htd_sfw_sha256_init(p_ctx);
}
// @brief 更新 sha256
void htd_sha256_update(htd_sha256_ctx_t *p_ctx, const uint8_t* p_data, uint32_t data_len)
{
    ASSERT(p_ctx);
    htd_sha256_info_t* p_info = (htd_sha256_info_t*)p_ctx;
    if (p_info->magic1 == HTD_SHA256_MAGIC_VAL && p_info->magic2 == HTD_SHA256_MAGIC_VAL) {
        // 使用硬件进行处理
        htd_dev_sha256_update(p_ctx, p_data, data_len);
        return;
    }
    // 使用软件进行处理
    htd_sfw_sha256_update(p_ctx, p_data, data_len);
}
// @brief 获取 sha256 结果
void htd_sha256_final(htd_sha256_ctx_t *p_ctx, uint8_t output[32])
{
    ASSERT(p_ctx);
    htd_sha256_info_t* p_info = (htd_sha256_info_t*)p_ctx;
    if (p_info->magic1 == HTD_SHA256_MAGIC_VAL && p_info->magic2 == HTD_SHA256_MAGIC_VAL) {
        // 使用硬件进行处理
        htd_dev_sha256_final(p_ctx, output);
        return;
    }
    // 使用软件进行处理
    htd_sfw_sha256_final(p_ctx, output);
}
// @brief sha256
void htd_sha256(const uint8_t* p_data, uint32_t data_len, uint8_t output[32])
{
    htd_sha256_ctx_t sha256_ctx;
    htd_sha256_init(&sha256_ctx);
    htd_sha256_update(&sha256_ctx, p_data, data_len);
    htd_sha256_final(&sha256_ctx, output);
}
// @brief sha256 加密计算
void htd_sha256_hmac(const uint8_t *p_key, int key_len, const uint8_t *p_data, int data_len, uint8_t out[32]) 
{
    htd_sha256_ctx_t ctx;
    uint8_t temp[32];
    uint8_t key2[32];
    uint8_t buff[64];
    int i;

    if (key_len > sizeof(buff)) {
        htd_sha256_init(&ctx);
        htd_sha256_update(&ctx, p_key, key_len);
        htd_sha256_final(&ctx, key2);
        p_key = key2;
        key_len = sizeof(key2);
    }

    for (i=0; i<key_len; ++i) {
        buff[i] = p_key[i] ^ 0x36;
    }
    for (i=key_len; i<sizeof(buff); ++i) {
        buff[i] = 0 ^ 0x36;
    }
    
    htd_sha256_init(&ctx);
    htd_sha256_update(&ctx, buff, sizeof(buff));
    htd_sha256_update(&ctx, p_data, data_len);
    htd_sha256_final(&ctx, temp);

    for (i=0; i<key_len; ++i) {
        buff[i] = p_key[i] ^ 0x5c;
    }
    for (i=key_len; i<sizeof(buff); ++i) {
        buff[i] = 0 ^ 0x5c;
    }
    htd_sha256_init(&ctx);
    htd_sha256_update(&ctx, buff, sizeof(buff));
    htd_sha256_update(&ctx, temp, sizeof(temp));
    htd_sha256_final(&ctx, out);
}




//////////////////////////////////////////////////////////////////////
typedef struct {
    uint32_t magic1;
    uint32_t magic2;
}htd_md5_info_t;
#define HTD_MD5_MAGIC_VAL        0xAA355535

// @brief 初始化(WEAK 函数, 硬件实现)
__WEAK int32_t htd_dev_md5_init(htd_md5_ctx_t *p_ctx)
{
    return ERROR_CODE_FAILED;
}
// @brief 更新MD5 (WEAK 函数, 硬件实现)
__WEAK void htd_dev_md5_update(htd_md5_ctx_t *p_ctx, const uint8_t* p_data, uint32_t data_len)
{
}
// @brief 获取MD5 结果(WEAK 函数, 硬件实现)
__WEAK void htd_dev_md5_final(htd_md5_ctx_t *p_ctx, uint8_t output[16])
{
}


// @brief 初始化 md5; 优先使用硬件，如果没有硬件或者硬件正在运行，自动使用软件
void htd_md5_init(htd_md5_ctx_t *p_ctx)
{
    ASSERT(p_ctx);
    if (htd_dev_md5_init(p_ctx) == ERROR_CODE_SUCCESS) {
        // 使用硬件进行处理
        htd_md5_info_t* p_info = (htd_md5_info_t*)p_ctx;
        p_info->magic1 = HTD_MD5_MAGIC_VAL;
        p_info->magic2 = HTD_MD5_MAGIC_VAL;
        return ;
    }

    // 使用软件进行处理
    htd_sfw_md5_starts(p_ctx);
}
// @brief 更新 md5
void htd_md5_update(htd_md5_ctx_t *p_ctx, const uint8_t* p_data, uint32_t data_len)
{
    ASSERT(p_ctx);
    htd_md5_info_t* p_info = (htd_md5_info_t*)p_ctx;
    if (p_info->magic1 == HTD_MD5_MAGIC_VAL && p_info->magic2 == HTD_MD5_MAGIC_VAL) {
        // 使用硬件进行处理
        htd_dev_md5_update(p_ctx, p_data, data_len);
        return;
    }
    // 使用软件进行处理
    htd_sfw_md5_update(p_ctx, p_data, data_len);
}
// @brief 获取 md5 结果
void htd_md5_final(htd_md5_ctx_t *p_ctx, uint8_t output[16])
{
    ASSERT(p_ctx);
    htd_md5_info_t* p_info = (htd_md5_info_t*)p_ctx;
    if (p_info->magic1 == HTD_MD5_MAGIC_VAL && p_info->magic2 == HTD_MD5_MAGIC_VAL) {
        // 使用硬件进行处理
        htd_dev_md5_final(p_ctx, output);
        return;
    }
    // 使用软件进行处理
    htd_sfw_md5_finish(p_ctx, output);
}
// @brief md5
void htd_md5(const uint8_t* p_data, uint32_t data_len, uint8_t output[16])
{
    htd_md5_ctx_t md5_ctx;
    htd_md5_init(&md5_ctx);
    htd_md5_update(&md5_ctx, p_data, data_len);
    htd_md5_final(&md5_ctx, output);
}
// @brief md5 加密计算
void htd_md5_hmac(const uint8_t *p_key, int key_len, const uint8_t *p_data, int data_len, uint8_t out[16]) 
{
    htd_md5_ctx_t ctx;
    uint8_t temp[16];
    uint8_t key2[16];
    uint8_t buff[64];
    int i;

    if (key_len > sizeof(buff)) {
        htd_md5_init(&ctx);
        htd_md5_update(&ctx, p_key, key_len);
        htd_md5_final(&ctx, key2);
        p_key = key2;
        key_len = sizeof(key2);
    }

    for (i=0; i<key_len; ++i) {
        buff[i] = p_key[i] ^ 0x36;
    }
    for (i=key_len; i<sizeof(buff); ++i) {
        buff[i] = 0 ^ 0x36;
    }
    
    htd_md5_init(&ctx);
    htd_md5_update(&ctx, buff, sizeof(buff));
    htd_md5_update(&ctx, p_data, data_len);
    htd_md5_final(&ctx, temp);

    for (i=0; i<key_len; ++i) {
        buff[i] = p_key[i] ^ 0x5c;
    }
    for (i=key_len; i<sizeof(buff); ++i) {
        buff[i] = 0 ^ 0x5c;
    }
    htd_md5_init(&ctx);
    htd_md5_update(&ctx, buff, sizeof(buff));
    htd_md5_update(&ctx, temp, sizeof(temp));
    htd_md5_final(&ctx, out);
}




//////////////////////////////////////////////////////////////////////
#if (defined(HTD_CONSOLE_ENABLE) && (DEBUG_LVL == 4) && 1)
#include "htd_console_process.h"

// 在线网址：https://www.jyshare.com/crypto

// @brief sha256 测试
void sha256_test_handle(void)
{
    uint8_t output[32];
    htd_sha256_ctx_t sha256_ctx;
    const uint8_t* p_data = (const uint8_t*)"0123456789";
    uint32_t data_len = strlen((const char*)p_data);

    // 结果：84d89877f0d4041efb6bf91a16f0248f2fd573e6af05c19f96bedb9f882f7882
    htd_dev_sha256_init(&sha256_ctx);
    htd_dev_sha256_update(&sha256_ctx, p_data, data_len);
    htd_dev_sha256_final(&sha256_ctx, output);
    print_warning("sha256 data: %s", p_data);
    print_hex("sha256", output, 32);
}

// @brief sha1 测试
void sha1_test_handle(void)
{
    uint8_t output[20];
    htd_sha1_ctx_t sha1_ctx;
    const uint8_t* p_data = (const uint8_t*)"0123456789";
    uint32_t data_len = strlen((const char*)p_data);

    // 结果：87acec17cd9dcd20a716cc2cf67417b71c8a7016
    htd_dev_sha1_init(&sha1_ctx);
    htd_dev_sha1_update(&sha1_ctx, p_data, data_len);
    htd_dev_sha1_final(&sha1_ctx, output);
    print_warning("sha1 data: %s", p_data);
    print_hex("sha1", output, 20);
}

// @brief md5 测试
void md5_test_handle(void)
{
    uint8_t output[16];
    htd_md5_ctx_t md5_ctx;
    const uint8_t* p_data = (const uint8_t*)"0123456789";
    uint32_t data_len = strlen((const char*)p_data);

    // 结果：781e5e245d69b566979b86e28d23f2c7
    htd_dev_md5_init(&md5_ctx);
    htd_dev_md5_update(&md5_ctx, p_data, data_len);
    htd_dev_md5_final(&md5_ctx, output);
    print_warning("md5 data: %s", p_data);
    print_hex("md5", output, 16);
}


// @brief 调试命令
static int hash_cmd(int argc, char **argv)
{
    if (argc < 2) {
        print_error("Usage: %s sha256/sha1/sha224/md5", argv[0]);
        return 0;
    }

    if (memcmp(argv[1], "sha256", 7) == 0) {
        sha256_test_handle();
    } else if (memcmp(argv[1], "sha1", 5) == 0) {
        sha1_test_handle();
    } else if (memcmp(argv[1], "md5", 4) == 0) {
        md5_test_handle();
    } else if (memcmp(argv[1], "sha224", 7) == 0) {

    } else {
        print_error("Usage: %s sha256/sha1/sha224/md5", argv[0]);
    }
    return 0;
}
CONSOLE_CMD_EXPORT(hash_cmd, hash);
#endif /*(defined(HTD_CONSOLE_ENABLE) && (DEBUG_LVL > 0))*/
#endif 

