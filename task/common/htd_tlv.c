#define DEBUG_LVL   4        // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG   "htd_klv"
#include "htd_tlv.h"
#include "htd_task.h"


// @brief 对标签进行编码，返回编码后的数据长度
static uint32_t htd_tlv_tag_encode(tlv_int_t val, uint8_t* p_code, uint32_t code_size) 
{
    ASSERT(val <= HTD_TLV_KEY_MAX);
    ASSERT(p_code);
    uint32_t int32val = val;
    if (int32val <= 0x7f) {                      // 0xxxxxxx
        ASSERT(code_size);
        p_code[0] = int32val & 0xff;
        return 1;
    } 
    if (int32val <= 0x1fff) {                    // 110xxxxx xxxxxxxx
        ASSERT(code_size > 1);
        p_code[0] = (int32val >> 8) | 0xc0;
        p_code[1] = (int32val & 0xff);
        return 2;
    } 
    if (int32val <= 0xfffff) {                   // 1110xxxx xxxxxxxx xxxxxxxx
        ASSERT(code_size > 2);
        p_code[0] = (int32val >> 16) | 0xe0;
        p_code[1] = (int32val >> 8) & 0xff;
        p_code[2] = (int32val & 0xff);
        return 3;
    } 
    if (int32val <= 0x7ffffff) {                 // 11110xxx xxxxxxxx xxxxxxxx xxxxxxxx
        ASSERT(code_size > 3);
        p_code[0] = (int32val >> 24) | 0xf0;
        p_code[1] = (int32val >> 16) & 0xff;
        p_code[2] = (int32val >> 8) & 0xff;
        p_code[3] = (int32val & 0xff);
        return 4;
    } 
    ASSERT(0);
    return 0;
}

// @brief 对标签进行解码, 返回解码的数据长度
// @param 输出解码后的值
static uint32_t htd_tlv_tag_decode(const uint8_t* p_code, uint32_t code_size, tlv_int_t* p_val) 
{
    ASSERT(p_val);
    uint32_t val;
    uint8_t ch = p_code[0];
    if (ch < 0x80) {                        // 0xxxxxxx
        if (code_size < 1) {
            return 0;
        }
        p_val[0] = ch;
        return 1;
    } else if (ch < 0xC0) { 
        return 0;
    } else if (ch < 0xE0) {                 // 110xxxxx xxxxxxxx
        if (code_size < 2) {
            return 0;
        }
        val = p_code[0] << 8;
        val |= p_code[1];
        val &= 0x1fff;
        p_val[0] = val;
        return 2;
    } else if (ch < 0xF0) {                 // 1110xxxx xxxxxxxx xxxxxxxx
        if (code_size < 3) {
            return 0;
        }
        val = p_code[0] << 16;
        val |= p_code[1] << 8;
        val |= p_code[2];
        val &= 0xfffff;
        p_val[0] = val;
        return 3;
    } else {                                // 11110xxx xxxxxxxx xxxxxxxx xxxxxxxx
        if (code_size < 4) {
            return 0;
        }
        val = p_code[0] << 24;
        val |= p_code[1] << 16;
        val |= p_code[2] << 8;
        val |= p_code[3];
        val &= 0x7ffffff;
        p_val[0] = val;
        return 4;
    } 
}

// @brief 查找某个token; 查找到返回的 token.len 不为0
htd_tlv_token_t htd_tlv_token_find(tlv_int_t find_key, const uint8_t* p_in, uint32_t in_len) 
{
    htd_tlv_token_t dst;
    htd_memset(&dst, 0, sizeof(htd_tlv_token_t));
    if (p_in == NULL) {
        return dst;
    }

    tlv_int_t len;
    tlv_int_t key;
    uint32_t decode_len;
    while (1) {
        decode_len = htd_tlv_tag_decode(p_in, in_len, &key);
        if (decode_len == 0) {
            break;
        }
        p_in += decode_len;
        in_len -= decode_len;

        decode_len = htd_tlv_tag_decode(p_in, in_len, &len);
        if (decode_len == 0) {
            break;
        }
        p_in += decode_len;
        in_len -= decode_len;
        if (in_len < len) {
            print_error("token find failed. in_len=%d, len=%d, key=%d", in_len, len, key);
            break;
        }
        if (key == find_key) {
            dst.key = key;
            dst.len = len;
            dst.p_val = p_in;
            break;
        }
        p_in += len;
        in_len -= len;
    }
    return dst;
}

// @brief 数据解析
// @return 返回解析到的token数量
uint32_t htd_tlv_token_parser(htd_tlv_token_t* p_token, uint32_t token_count, const uint8_t* p_in, uint32_t in_len)
{
    tlv_int_t len;
    tlv_int_t key;
    if (p_in == NULL || p_token == NULL) {
        return 0;
    }

    uint32_t decode_len;
    htd_tlv_token_t* p_start = p_token;
    while (token_count) {
        decode_len = htd_tlv_tag_decode(p_in, in_len, &key);
        if (decode_len == 0) {
            break;
        }
        p_in += decode_len;
        in_len -= decode_len;

        decode_len = htd_tlv_tag_decode(p_in, in_len, &len);
        if (decode_len == 0) {
            break;
        }
        p_in += decode_len;
        in_len -= decode_len;
        if (in_len < len) {
            print_error("token parser failed. in_len=%d, len=%d, key=%d", in_len, len, key);
            break;
        }
        
        p_token->key = key;
        p_token->len = len;
        p_token->p_val = (void*)p_in;
        p_token++;
        token_count--;
        p_in += len;
        in_len -= len;
    }
    return p_token - p_start;
}

// @brief 对数据进行序列化
// @param 返回序列化后的数据长度，数据通过p_out带出
uint32_t htd_tlv_token_serialize(const htd_tlv_token_t* p_token, uint32_t token_count, uint8_t* p_out, uint32_t out_size)
{
    if (p_out == NULL || p_token == NULL) {
        return 0;
    }

    uint8_t data[12];
    uint32_t data_len;
    uint8_t* p_start = p_out;
    while (token_count--) {
        data_len = htd_tlv_tag_encode(p_token->key, data, sizeof(data));
        data_len += htd_tlv_tag_encode(p_token->len, &data[data_len], sizeof(data)-data_len);
        if (out_size < data_len + p_token->len) {
            break;
        }
        htd_memcpy(p_out, data, data_len);
        out_size -= data_len;
        p_out += data_len;
        if (p_token->len) {
            ASSERT(p_token->p_val);
            htd_memcpy(p_out, p_token->p_val, p_token->len);
            p_out += p_token->len;
            out_size -= p_token->len;
        }
        p_token++;
    }
    return p_out - p_start;
}

// @brief 数据解析
// @return 返回已经解析的数据长度
uint32_t htd_tlv_token_parser_one(htd_tlv_token_t* p_token, const uint8_t* p_in, uint32_t in_len) 
{
    if (htd_tlv_token_parser(p_token, 1, p_in, in_len)) {
        return p_token->len + sizeof(uint16_t);
    }
    return 0;
}

// @brief 只序列化一个
// @param 返回序列化后的数据长度，数据通过p_out带出
uint32_t htd_tlv_token_serialize_one(const htd_tlv_token_t* p_token, uint8_t* p_out, uint32_t out_size) 
{
    return htd_tlv_token_serialize(p_token, 1, p_out, out_size);
}

// @brief 获取整形值
int32_t htd_tlv_token_intval_get(const htd_tlv_token_t* p_token) 
{
    if (p_token->len == 1) {
        return *(int8_t*)p_token->p_val;
    } else if (p_token->len == 2) {
        return *(int16_t*)p_token->p_val;
    } else if (p_token->len == 4) {
        return *(int32_t*)p_token->p_val;
    } else {
        print_error("unknown token. key=%d, len=%d", p_token->key, p_token->len);
        return 0;
    }
}


