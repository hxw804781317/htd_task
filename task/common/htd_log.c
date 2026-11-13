// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#include "htd_task.h"
#include "htd_log.h"

// @brief 打印字符
static void htd_print_buff(const uint8_t* p_buff, int32_t buff_len)
{
    htd_print("\t");
    while (buff_len--) {
        htd_print("%c", *p_buff++);
    }
    htd_print("\r\n");
}

// @brief 打印 16 进制
void htd_print_hex(const char* p_tag, const uint8_t* p_buff, uint16_t buff_len)
{
    uint8_t line; 
    uint8_t data[128];
    uint8_t data_len;

    line = 32; 
    if (p_tag) {
        htd_print("%s:\r\n", p_tag);
    }
    while (buff_len) {
        if (buff_len < line) {
            line = buff_len;
        }
        data_len = htd_base16_encode(p_buff, line, data, sizeof(data));
        data[data_len] = 0;
        htd_print_buff(data, data_len);
        buff_len -= line;
        p_buff += line;
    }
    htd_print("\r\n");
}

// @brief 内存数据显示
void htd_hexdump(const uint8_t *p_data, uint32_t data_len)
{
    #define LINE_BUFFER_LENGTH (4 + 2 + (3 * 8) + 2 + (3 * 8) + 2 + 8 + 1 + 8 + 1)
    char* p_line;
    char line_buffer[LINE_BUFFER_LENGTH];
    uint32_t offset = 0;
    uint32_t i;

    while (offset < data_len) {
        p_line = line_buffer;
        // 打印行数据偏移量
        p_line += htd_snprintf(p_line, LINE_BUFFER_LENGTH, "%04x  ", offset);
        
        // 打印16进制数据
        const uint32_t num_line_bytes = ((data_len - offset) > 16) ? 16 : (data_len - offset);
        for (i = 0; i < num_line_bytes; ++i) {
            if (i == 8) {
                *p_line++ = ' ';
            }
            p_line += htd_base16_encode(&p_data[offset + i], 1, (uint8_t*)p_line, 2);
            *p_line++ = ' ';
        }
        
        // 补充对齐
        uint32_t required_padding = 2;
        if (num_line_bytes < 16) {
            required_padding += (16 - num_line_bytes) * 3;
            if (num_line_bytes <= 8) {
                required_padding += 1;
            }
        }
        htd_memset(p_line, ' ', required_padding);
        p_line += required_padding;

        // 打印 ASCII 字节
        for (i = 0; i < num_line_bytes; ++i) {
            if (i == 8) {
                *p_line++ = ' ';
            }
            char c = p_data[offset + i];
            if (c < 32 || c > 126 || c == '`') { // ` is used for log hash string delimiting
                c = '.';
            }
            *p_line++ = c;
        }
        offset += 16;

        // 打印输出
        *p_line++ = 0;
        htd_print("%s\r\n", line_buffer);
    }
}


#ifdef LIB_SEGGER_RTT_ENABLE
//////////////////////////////////////////////////////////////////////
#include "SEGGER_RTT.h"

// @brief SEGGER-RTT 输出
static void segger_rtt_write_handle(void const * p_user_ctx, char const * p_str, size_t length)
{
    SEGGER_RTT_Write(0, p_str, length);
}

// @brief segger-rtt 打印输出
int32_t htd_segger_rtt_print(const char *fmt, ...)
{
    uint32_t buff_len;
    char buff[64];

    htd_printf_ctx_t ctx;
    ctx.p_user_ctx = NULL;
    ctx.fwrite = segger_rtt_write_handle;
    ctx.p_io_buffer = buff;
    ctx.io_buffer_size = sizeof(buff);
    ctx.io_buffer_cnt = 0;

    va_list args;
    va_start(args, fmt);
    buff_len = htd_vfprintf(&ctx, fmt, &args);
    va_end(args);
    return buff_len;
}
#endif /*LIB_SEGGER_RTT_ENABLE*/


