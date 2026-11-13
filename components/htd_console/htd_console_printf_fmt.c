// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL   4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG   "fmt"
#include "htd_console_printf_fmt.h"


#if !defined(HTD_TASK_ENABLE)
//////////////////////////////////////////////////////////////////////
#define HTD_FMT_FLAG_LEFT_JUSTIFY       (1u << 0)
#define HTD_FMT_FLAG_PAD_ZERO           (1u << 1)
#define HTD_FMT_FLAG_PRINT_SIGN         (1u << 2)

#define HTD_FMT_DOUBLE_DEF_PRECISION    6
#define HTD_FMT_DOUBLE_SIGN_POSITION    63U
#define HTD_FMT_DOUBLE_SIGN_MASK        1ULL
#define HTD_FMT_DOUBLE_SIGN             (HTD_FMT_DOUBLE_SIGN_MASK << HTD_FMT_DOUBLE_SIGN_POSITION)
#define HTD_FMT_DOUBLE_EXP_POSITION     52U
#define HTD_FMT_DOUBLE_EXP_MASK         0x7FFULL
#define HTD_FMT_DOUBLE_EXP              (HTD_FMT_DOUBLE_EXP_MASK << HTD_FMT_DOUBLE_EXP_POSITION)
#define HTD_FMT_DOUBLE_MANT_POSITION    0U
#define HTD_FMT_DOUBLE_MANT_MASK        0xFFFFFFFFFFFFF
#define HTD_FMT_DOUBLE_MANT             (HTD_FMT_DOUBLE_MANT_MASK << HTD_FMT_DOUBLE_MANT_POSITION)

#define HTD_FMT_DOUBLE_SIGN_GET(v)      (!!((v) & HTD_FMT_DOUBLE_SIGN))
#define HTD_FMT_DOUBLE_EXP_GET(v)       (((v) & HTD_FMT_DOUBLE_EXP) >> HTD_FMT_DOUBLE_EXP_POSITION)
#define HTD_FMT_DOUBLE_MANT_GET(v)      (((v) & HTD_FMT_DOUBLE_MANT) >> HTD_FMT_DOUBLE_MANT_POSITION)
#define HTD_FMT_REQ_SIGN_SPACE(s, f)    ((s) | (!!((f) & HTD_FMT_FLAG_PRINT_SIGN)))

#define HIGH_32(v)                      ((v) >> 32)
#define LOW_32(v)                       (((1ULL << 32) - 1) & v)


// @brief 输出字符
static void htd_fprintf_buffer_flush(htd_printf_ctx_t * const p_ctx)
{
    if (p_ctx->io_buffer_cnt) {
        p_ctx->fwrite(p_ctx->p_user_ctx, p_ctx->p_io_buffer, p_ctx->io_buffer_cnt);
        p_ctx->out_buffer_cnt += p_ctx->io_buffer_cnt;
        p_ctx->io_buffer_cnt = 0;
    }
}

// @brief 添加数据
static void buffer_add(htd_printf_ctx_t * const p_ctx, char c)
{   
    if (p_ctx->fwrite) {
        p_ctx->p_io_buffer[p_ctx->io_buffer_cnt++] = c;
        if (p_ctx->io_buffer_cnt >= p_ctx->io_buffer_size) {
            htd_fprintf_buffer_flush(p_ctx);
        }
    } else {
        if (p_ctx->io_buffer_cnt < p_ctx->io_buffer_size) {
            p_ctx->p_io_buffer[p_ctx->io_buffer_cnt++] = c;
        }
    }
}

// @brief 格式化字符串
static void string_print(htd_printf_ctx_t * const p_ctx, char const * p_str, uint32_t field_width, uint32_t format_flags)
{
    uint32_t width = 0;
    char c;
    if ((format_flags & HTD_FMT_FLAG_LEFT_JUSTIFY) == HTD_FMT_FLAG_LEFT_JUSTIFY) {
        while ((c = *p_str) != '\0') {
            p_str++;
            width++;
            buffer_add(p_ctx, c);
        }
        while ((field_width > width) && (field_width > 0)) {
            field_width--;
            buffer_add(p_ctx, ' ');
        }
    } else {
        if (p_str != 0) {
            width = strlen(p_str);
        }
        while ((field_width > width) && (field_width > 0)) {
            field_width--;
            buffer_add(p_ctx, ' ');
        }
        while ((c = *p_str) != '\0') {
            p_str++;
            width++;
            buffer_add(p_ctx, c);
        }
    }
}

// @brief 打印无符号整形数据
static void unsigned_print(htd_printf_ctx_t * const p_ctx, uint32_t v, uint32_t base, uint32_t num_digits, uint32_t field_width, uint32_t format_flags)
{
    static const char int_ch_table[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
    uint32_t div;
    uint32_t value;
    uint32_t width;
    char c;

    // 计算数值长度
    value = v;
    width = 1u;
    while (value >= base) {
        value = (value / base);
        width++;
    }
    if (num_digits > width) {
        width = num_digits;
    }

    // 打印前导字符
    if ((format_flags & HTD_FMT_FLAG_LEFT_JUSTIFY) == 0u) {
        if (field_width != 0u) {
            if (((format_flags & HTD_FMT_FLAG_PAD_ZERO) == HTD_FMT_FLAG_PAD_ZERO) && (num_digits == 0u)) {
                c = '0';
            } else {
                c = ' ';
            }
            while ((field_width != 0u) && (width < field_width)) {
                field_width--;
                buffer_add(p_ctx, c);
            }
        }
    }

    // 计算数值
    value = 1;
    while (1) {
        if (num_digits > 1u) {
            num_digits--;
        } else {
            div = v / value;
            if (div < base) {
                break;
            }
        }
        value *= base;
    }

    // 输出数值
    do {
        div = v / value;
        v -= div * value;
        buffer_add(p_ctx, int_ch_table[div]);
        value /= base;
    } while (value);

    // 补充空格
    if ((format_flags & HTD_FMT_FLAG_LEFT_JUSTIFY) == HTD_FMT_FLAG_LEFT_JUSTIFY) {
        if (field_width != 0u) {
            while ((field_width != 0u) && (width < field_width)) {
                field_width--;
                buffer_add(p_ctx, ' ');
            }
        }
    }
}

// @brief 打印带符号的整形数据
static void int_print(htd_printf_ctx_t * const p_ctx, int32_t v, uint32_t base, uint32_t num_digits, uint32_t field_width, uint32_t format_flags)
{
    // 计算实际长度
    uint32_t width = 1;
    int32_t number = (v < 0) ? -v : v;
    while (number >= (int32_t)base) {
        number = (number / (int32_t)base);
        width++;
    }
    if (num_digits > width) {
        width = num_digits;
    }
    if ((field_width > 0u) && ((v < 0) || ((format_flags & HTD_FMT_FLAG_PRINT_SIGN) == HTD_FMT_FLAG_PRINT_SIGN))) {
        field_width--;
    }

    // 打印前导字符
    if ((((format_flags & HTD_FMT_FLAG_PAD_ZERO) == 0u) || (num_digits != 0u)) && ((format_flags & HTD_FMT_FLAG_LEFT_JUSTIFY) == 0u)) {
        if (field_width != 0u) {
            while ((field_width != 0u) && (width < field_width)) {
                field_width--;
                buffer_add(p_ctx, ' ');
            }
        }
    }

    // 打印负号
    if (v < 0) {
        v = -v;
        buffer_add(p_ctx, '-');
    } else {
        if ((format_flags & HTD_FMT_FLAG_PRINT_SIGN) == HTD_FMT_FLAG_PRINT_SIGN) {
            buffer_add(p_ctx, '+');
        }
    }
    
    // 填充0
    if (((format_flags & HTD_FMT_FLAG_PAD_ZERO) == HTD_FMT_FLAG_PAD_ZERO) &&
        ((format_flags & HTD_FMT_FLAG_LEFT_JUSTIFY) == 0u) && (num_digits == 0u)) {
        if (field_width != 0u) {
            while ((field_width != 0u) && (width < field_width)) {
                field_width--;
                buffer_add(p_ctx, '0');
            }
        }
    }
    //打印无符号整形数据
    unsigned_print(p_ctx, (uint32_t)v, base, num_digits, field_width, format_flags);
}

// @brief 填充空格
static void fill_space(htd_printf_ctx_t * const p_ctx, uint8_t len, bool zeros)
{
    for (; len > 0; len--) {
        if (zeros) {
            buffer_add(p_ctx, '0');
        } else {
            buffer_add(p_ctx, ' ');
        }
    }
}

// @brief 格式化浮点数
static void float_print(htd_printf_ctx_t * const p_ctx, double v, uint32_t digits, uint32_t width, uint32_t format, bool uppercase)
{
    bool sign, transform = false;
    uint64_t num, mant, lead, low, base, res, carry, x, s0, s1, s2, s3, fr;
    int32_t exp;
    uint8_t highest, offset, lead_len = 0, skipped = 0;
    uint8_t precision = digits ? digits + 1 : HTD_FMT_DOUBLE_DEF_PRECISION + 1;
    /* Default digits should be -1, because 0 could be a requirement, not the default.
     * This should be changed for the whole library.
     */

    if ((v > 0.0) && (v < 1.0)) {
        v += 1.0;
        transform = true;
    } else if ((v > -1.0) && (v < 0.0)) {
        v -= 1.0;
        transform = true;
    }

    MEMCPY(&num, &v, sizeof(num));
    sign = HTD_FMT_DOUBLE_SIGN_GET(num);
    exp = HTD_FMT_DOUBLE_EXP_GET(num);
    mant = HTD_FMT_DOUBLE_MANT_GET(num);

    /* Special cases */
    if (exp == HTD_FMT_DOUBLE_EXP_MASK) {
        if (width && (!(format & HTD_FMT_FLAG_LEFT_JUSTIFY))) {
            fill_space(p_ctx, width - 3 - HTD_FMT_REQ_SIGN_SPACE(sign, format), false);
        }

        if (sign) {
            buffer_add(p_ctx, '-');
        } else if (format & HTD_FMT_FLAG_PRINT_SIGN) {
            buffer_add(p_ctx, '+');
        }

        if (mant != 0) {
            if (uppercase) {
                buffer_add(p_ctx, 'N');
                buffer_add(p_ctx, 'A');
                buffer_add(p_ctx, 'N');
            } else {
                buffer_add(p_ctx, 'n');
                buffer_add(p_ctx, 'a');
                buffer_add(p_ctx, 'n');
            }
        } else {
            if(uppercase) {
                buffer_add(p_ctx, 'I');
                buffer_add(p_ctx, 'N');
                buffer_add(p_ctx, 'F');
            } else {
                buffer_add(p_ctx, 'i');
                buffer_add(p_ctx, 'n');
                buffer_add(p_ctx, 'f');
            }
        }
        return;
    }

    /* Add leading 1 to mantissa (except 0.0) */
    if ((mant != 0) || (exp != 0)) {
        mant |= (1ULL << 52);
    }
    /* Convert the exponent */
    exp = exp - 1023;

    /* Whole numbers */
    offset  = 52 - exp;
    if (offset > 64) {
        return;
    }

    lead = (mant >> (offset));

    /* Fraction */
    low = mant & (~(lead << offset));
    while (((low & 0x1) == 0) && low > 0) {
        low = low >> 1U;
        skipped++;
    }

    highest = (offset - skipped);
    base = 1;
    for(uint8_t i = 0; i < precision; i++) {
        base *= 10;
    }

    /* Handle multiplication with possible overflow */
    x = LOW_32(low) * LOW_32(base);
    s0 = LOW_32(x);

    x = HIGH_32(low) * LOW_32(base) + HIGH_32(x);
    s1 = LOW_32(x);
    s2 = HIGH_32(x);

    x = s1 + LOW_32(low) * HIGH_32(base);
    s1 = LOW_32(x);

    x = s2 + HIGH_32(low) * HIGH_32(base) + HIGH_32(x);
    s2 = LOW_32(x);
    s3 = HIGH_32(x);

    res = s1 << 32 | s0;
    carry = s3 << 32 | s2;

    /* Divide and combine */
    carry = carry << (64 - highest);
    res = res >> highest;
    fr = res | carry;

    /* Roundup */
    if (fr%10 >= 5) {
        fr /= 10;
        fr++;
    } else {
        fr /= 10;
    }
    precision--;

    if (transform && (lead == 1)) {
        lead = 0;
    }

    /* Maximum precision handled by int_print() is 10 */
    if (precision > 10) {
        for (uint8_t delta = precision - 10; delta > 0; delta--) {
            fr /= 10;
        }
        precision = 10;
    }

    res = lead;
    while (res > 0) {
        res /= 10;
        lead_len++;
    }

    if ((lead == 0) && (fr == 0)) {
        lead_len = 1;
    }
    if (lead_len == 0) {
        lead_len = 1;
    }

    if (width && (!(format & HTD_FMT_FLAG_LEFT_JUSTIFY))) {
        int32_t space = width - lead_len - precision - HTD_FMT_REQ_SIGN_SPACE(sign, format) - 1;
        if (space > 0) {
            fill_space(p_ctx, space, format & HTD_FMT_FLAG_PAD_ZERO);
        }
    }

    if (sign) {
        buffer_add(p_ctx, '-');
    } else if (format & HTD_FMT_FLAG_PRINT_SIGN) {
        buffer_add(p_ctx, '+');
    }

    int_print(p_ctx, lead, 10u, 0, 0,0);
    buffer_add(p_ctx, '.');
    int_print(p_ctx, fr, 10u, precision, 0, 0);

    // 处理左对齐
    if (width && (format & HTD_FMT_FLAG_LEFT_JUSTIFY)) {
        int32_t space = width - lead_len - precision - HTD_FMT_REQ_SIGN_SPACE(sign, format) - 1;
        if (space > 0) {
            fill_space(p_ctx, space, false);
        }
    }
}

// @brief 获取拼接格式
static uint32_t fprintf_fmt_flags_get(char const *p_fmt, uint32_t* format_flags)
{
    char c;
    char const * p_start = p_fmt;
    uint32_t flag = 0;
    
    for (;;) {
        c = *p_fmt;
        switch (c) {
            case '-': {
                flag |= HTD_FMT_FLAG_LEFT_JUSTIFY;
                break;
            } 
            case '0': {
                flag |= HTD_FMT_FLAG_PAD_ZERO;
                break;
            }
            case '+': {
                flag |= HTD_FMT_FLAG_PRINT_SIGN;
                break;
            }
            default: {
                c = 0;
                break;
            }
        }
        if (c == 0) {
            break;
        }
        p_fmt++;
    } 
    *format_flags = flag;
    return p_fmt - p_start; 
}

// @brief 获取格式化宽度
static uint32_t fprintf_field_width_get(char const *p_fmt, uint32_t* field_width, va_list *p_args)
{
    uint32_t width = 0u;
    char c = *p_fmt;
    char const * p_start = p_fmt;
    do {
        if (c == '*') {
            /*lint -save -e64 -e56*/
            width += va_arg(*p_args, unsigned);
            p_fmt++;
            break;
        }
        c = *p_fmt;
        if ((c < '0') || (c > '9')) {
            break;
        }
        p_fmt++;
        width = (width * 10u) + (c - '0');
    } while (1);
    *field_width = width;
    return p_fmt - p_start;
}


// @brief 
static uint32_t fprintf_num_digits_get(char const *p_fmt, uint32_t* num_digits)
{
    uint32_t val = 0u;
    char c = *p_fmt;
    char const * p_start = p_fmt;
    if (c == '.') {
        p_fmt++;
        for (;;) {
            c = *p_fmt;
            if ((c < '0') || (c > '9')) {
                break;
            }
            p_fmt++;
            val = val * 10u + (c - '0');
        } 
    }
    *num_digits = val;
    return p_fmt - p_start;
}

// @brief 格式打印
static void htd_fprintf_fmt(htd_printf_ctx_t * const p_ctx, char const *p_fmt, va_list *p_args)
{
    ASSERT(p_ctx != NULL);
    if (p_fmt == NULL) {
        return;
    }

    char c;
    int32_t v;
    uint32_t num_digits;
    uint32_t format_flags;
    uint32_t field_width;

    while (*p_fmt != '\0') {
        c = *p_fmt++;
        if (c == 0u) {
            break;
        } 

        // 处理非字符 '%'
        if (c != '%') {
            buffer_add(p_ctx, c);
            continue;
        }
        
        // 提取后续格式
        p_fmt += fprintf_fmt_flags_get(p_fmt, &format_flags);
        // 格式长度
        p_fmt += fprintf_field_width_get(p_fmt, &field_width, p_args);
        // 数值
        p_fmt += fprintf_num_digits_get(p_fmt, &num_digits);
        
        //
        // Filter out length modifier
        //
        c = *p_fmt;
        for (;;) {
            if ((c == 'l') || (c == 'h')) {
                p_fmt++;
                c = *p_fmt;
            } else {
                break;
            }
        }
        
        /*lint -save -e64*/
        switch (c)  {
            case 'c': {
                char c0;
                v = va_arg(*p_args, int32_t);
                c0 = (char)v;
                buffer_add(p_ctx, c0);
                break;
            }
            case 'd':
            case 'i': {
                v = va_arg(*p_args, int32_t);
                int_print(p_ctx, v, 10u, num_digits, field_width, format_flags);
                break;
            }
            case 'u': {
                v = va_arg(*p_args, int32_t);
                unsigned_print(p_ctx, (uint32_t)v, 10u, num_digits, field_width, format_flags);
                break;
            }
            case 'x':
            case 'X':{
                v = va_arg(*p_args, int32_t);
                unsigned_print(p_ctx, (uint32_t)v, 16u, num_digits, field_width, format_flags);
                break;
            }
            case 's': {
                char const * p_s = va_arg(*p_args, const char *);
                string_print(p_ctx, p_s, field_width, format_flags);
                break;
            }
            case 'p': {
                v = va_arg(*p_args, int32_t);
                buffer_add(p_ctx, '0');
                buffer_add(p_ctx, 'x');
                unsigned_print(p_ctx, (uint32_t)v, 16u, 8u, 8u, 0);
                break;
            }
            case '%': {
                buffer_add(p_ctx, '%');
                break;
            }
            case 'f':
            case 'F': {
                double dbl = va_arg(*p_args, double);
                float_print(p_ctx, dbl, num_digits, field_width, format_flags, c == 'F');
                break;
            }
            default: {
                break;
            }
        }
        p_fmt++;
    }

    if (p_ctx->fwrite) {
        htd_fprintf_buffer_flush(p_ctx);
    } else {
        if (p_ctx->io_buffer_cnt < p_ctx->io_buffer_size) {
            p_ctx->p_io_buffer[p_ctx->io_buffer_cnt] = 0;
        }
    }
}

// @brief 格式化输出字符串, 返回输出的数据长度
uint32_t htd_fprintf(htd_printf_ctx_t* const p_ctx, char const *p_fmt, ...)
{
    ASSERT(p_ctx);
    ASSERT(p_ctx->fwrite);
    ASSERT(p_ctx->p_io_buffer);
    ASSERT(p_ctx->io_buffer_size);
    if (p_fmt == NULL) {
        return 0;
    }

    va_list args = {0};
    va_start(args, p_fmt);
    htd_fprintf_fmt(p_ctx, p_fmt, &args);
    va_end(args);
    return p_ctx->out_buffer_cnt;
}

// @brief 格式化输出字符串, 返回输出的数据长度
uint32_t htd_vfprintf(htd_printf_ctx_t* const p_ctx, char const *p_fmt, va_list *p_args)
{
    ASSERT(p_ctx);
    ASSERT(p_ctx->fwrite);
    ASSERT(p_ctx->p_io_buffer);
    ASSERT(p_ctx->io_buffer_size);
    htd_fprintf_fmt(p_ctx, p_fmt, p_args);
    return p_ctx->out_buffer_cnt;
}

// @brief 格式化数据htd_fprintf_ctx_t
uint32_t htd_snprintf(char* p_data, uint32_t data_size, char const * p_fmt, ...)
{
    if (p_fmt == NULL || p_data == NULL || data_size == 0) {
        return 0;
    }

    htd_printf_ctx_t printf_ctx;
    MEMSET(&printf_ctx, 0, sizeof(htd_printf_ctx_t));
    printf_ctx.p_io_buffer = p_data;
    printf_ctx.io_buffer_size = data_size;

    va_list args = {0};
    va_start(args, p_fmt);
    htd_fprintf_fmt(&printf_ctx, p_fmt, &args);
    va_end(args);
    return printf_ctx.io_buffer_cnt;
}

// @brief 格式化输出
uint32_t htd_vsnprintf(char* p_data, uint32_t data_size, char const * p_fmt, va_list *p_args)
{
    if (p_fmt == NULL || p_data == NULL || data_size == 0) {
        return 0;
    }

    htd_printf_ctx_t printf_ctx;
    MEMSET(&printf_ctx, 0, sizeof(htd_printf_ctx_t));
    printf_ctx.p_io_buffer = p_data;
    printf_ctx.io_buffer_size = data_size;
    
    htd_fprintf_fmt(&printf_ctx, p_fmt, p_args);
    return printf_ctx.io_buffer_cnt;
}
#endif /*!defined(HTD_TASK_ENABLE)*/

