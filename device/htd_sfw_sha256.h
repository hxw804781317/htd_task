#ifndef _HTD_SFW_SHA256_H_
#define _HTD_SFW_SHA256_H_

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stddef.h>


#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

typedef struct {
    uint8_t data[64];
    uint32_t datalen;
    uint64_t bitlen;
    uint32_t state[8];
} htd_sha256_ctx_t;

// @brief 初始化
void htd_sfw_sha256_init(htd_sha256_ctx_t *p_ctx);
// @brief 更新 sha256 数据
void htd_sfw_sha256_update(htd_sha256_ctx_t *p_ctx, const uint8_t* p_data, uint32_t data_len);
// @brief 获取 sha256 结果
void htd_sfw_sha256_final(htd_sha256_ctx_t *p_ctx, uint8_t output[32]);


#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_SFW_SHA256_H_*/

