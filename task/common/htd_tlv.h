#ifndef __HTD_TLV_H__
#define __HTD_TLV_H__

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


// 根据项目需要配置整形类型
typedef uint16_t tlv_int_t;
#define HTD_TLV_KEY_MAX         0x7fff

typedef struct {
    tlv_int_t len;
    tlv_int_t key;
    const void* p_val;
}htd_tlv_token_t;


// @brief 查找某个token, 查找到返回的 token.len 不为0
htd_tlv_token_t htd_tlv_token_find(tlv_int_t find_key, const uint8_t* p_in, uint32_t in_len);
// @brief 数据解析
// @return 返回解析到的token数量
uint32_t htd_tlv_token_parser(htd_tlv_token_t* p_token, uint32_t token_count, const uint8_t* p_in, uint32_t in_len);
// @brief 对数据进行序列化
// @param 返回序列化后的数据长度，数据通过p_out带出
uint32_t htd_tlv_token_serialize(const htd_tlv_token_t* p_token, uint32_t token_count, uint8_t* p_out, uint32_t out_size);
// @brief 数据解析
// @return 返回已经解析的数据长度
uint32_t htd_tlv_token_parser_one(htd_tlv_token_t* p_token, const uint8_t* p_in, uint32_t in_len);
// @brief 只序列化一个
// @param 返回序列化后的数据长度，数据通过p_out带出
uint32_t htd_tlv_token_serialize_one(const htd_tlv_token_t* p_token, uint8_t* p_out, uint32_t out_size);
// @brief 获取整形值
int32_t htd_tlv_token_intval_get(const htd_tlv_token_t* p_token);


#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*__HTD_TLV_H__*/

