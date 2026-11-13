//#define DEBUG_LVL           4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG           "dev.crc"
#include "htd_common.h"
#include "htd_dev_crc.h"




//////////////////////////////////////////////////////////////////////
// @brief 开始(WEAK 函数, 驱动重定义)
__WEAK int32_t htd_dev_crc8_init(htd_crc8_ctx_t* p_ctx)
{
    ASSERT(p_ctx);
    return -ERROR_CODE_FAILED;
}
// @brief 计算(WEAK 函数, 驱动重定义)
__WEAK uint16_t htd_dev_crc8_update(htd_crc8_ctx_t* p_ctx, const uint8_t* p_data, uint32_t data_len)
{
    return 0;
}
// @brief 结束(WEAK 函数, 驱动重定义)
__WEAK int32_t htd_dev_crc8_final(htd_crc8_ctx_t* p_ctx)
{
    return 0;
}

// @brief 开始crc8; 优先使用硬件，如果没有硬件或者硬件正在运行，自动使用软件
int32_t htd_crc8_init(htd_crc8_ctx_t* p_ctx)
{
    if (htd_dev_crc8_init(p_ctx) == ERROR_CODE_SUCCESS) {
        p_ctx->user_hw_flag = 1;
        return 0;
    }
    p_ctx->user_hw_flag = 0;
    return 0;
}
// @brief 计算
uint16_t htd_crc8_update(htd_crc8_ctx_t* p_ctx, const uint8_t* p_data, uint32_t data_len)
{
    if (p_ctx->user_hw_flag == 0) {
        if (p_ctx->crc8_type == HTD_CRC8_DEF) {
            p_ctx->result = htd_sfw_crc8(p_data, data_len, p_ctx->result);
        } else if (p_ctx->crc8_type == HTD_CRC8_ITU) {
            p_ctx->result = htd_sfw_crc8_itu(p_data, data_len, p_ctx->result);
        } else if (p_ctx->crc8_type == HTD_CRC8_ROHC) {
            p_ctx->result = htd_sfw_crc8_rohc(p_data, data_len, p_ctx->result);
        } else if (p_ctx->crc8_type == HTD_CRC8_MAXIM) {
            p_ctx->result = htd_sfw_crc8_maxim(p_data, data_len, p_ctx->result);
        } else {
            ASSERT(0);
        }
        return p_ctx->result;
    }
    return htd_dev_crc8_update(p_ctx, p_data, data_len);
}
// @brief 结束
int32_t htd_crc8_final(htd_crc8_ctx_t* p_ctx)
{
    if (p_ctx->user_hw_flag == 0) {
        return p_ctx->result;
    }
    return htd_dev_crc8_final(p_ctx);
}   






//////////////////////////////////////////////////////////////////////
// @brief 开始(WEAK 函数, 驱动重定义)
__WEAK int32_t htd_dev_crc16_init(htd_crc16_ctx_t* p_ctx)
{
    ASSERT(p_ctx);
    return -ERROR_CODE_FAILED;
}
// @brief 计算(WEAK 函数, 驱动重定义)
__WEAK uint16_t htd_dev_crc16_update(htd_crc16_ctx_t* p_ctx, const uint8_t* p_data, uint32_t data_len)
{
    return 0;
}
// @brief 结束(WEAK 函数, 驱动重定义)
__WEAK uint16_t htd_dev_crc16_final(htd_crc16_ctx_t* p_ctx)
{
    return 0;
}

// @brief 开始CRC16; 优先使用硬件，如果没有硬件或者硬件正在运行，自动使用软件
int32_t htd_crc16_init(htd_crc16_ctx_t* p_ctx)
{
    if (htd_dev_crc16_init(p_ctx) == ERROR_CODE_SUCCESS) {
        p_ctx->user_hw_flag = 1;
        return 0;
    }
    p_ctx->user_hw_flag = 0;
    return 0;
}
// @brief 计算
uint16_t htd_crc16_update(htd_crc16_ctx_t* p_ctx, const uint8_t* p_data, uint32_t data_len)
{
    if (p_ctx->user_hw_flag == 0) {
        if (p_ctx->crc16_type == HTD_CRC16_CCITT) {
            p_ctx->result = htd_sfw_crc16_ccitt(p_data, data_len, p_ctx->result);
        } else if (p_ctx->crc16_type == HTD_CRC16_CCITT_FALSE) {
            p_ctx->result = htd_sfw_crc16_ccitt_false(p_data, data_len, p_ctx->result);
        } else if (p_ctx->crc16_type == HTD_CRC16_X25) {
            p_ctx->result = htd_sfw_crc16_x25(p_data, data_len, p_ctx->result);
        } else if (p_ctx->crc16_type == HTD_CRC16_XMODEM) {
            p_ctx->result = htd_sfw_crc16_xmodem(p_data, data_len, p_ctx->result);
        } else if (p_ctx->crc16_type == HTD_CRC16_MODBUS) {
            p_ctx->result = htd_sfw_crc16_modbus(p_data, data_len, p_ctx->result);
        } else {
            ASSERT(0);
        }
        return p_ctx->result;
    }
    return htd_dev_crc16_update(p_ctx, p_data, data_len);
}
// @brief 结束
uint16_t htd_crc16_final(htd_crc16_ctx_t* p_ctx)
{
    if (p_ctx->user_hw_flag == 0) {
        return p_ctx->result;
    }
    return htd_dev_crc16_final(p_ctx);
}   




//////////////////////////////////////////////////////////////////////
// @brief 开始(WEAK 函数, 驱动重定义)
__WEAK int32_t htd_dev_crc32_init(htd_crc32_ctx_t* p_ctx)
{
    return -ERROR_CODE_FAILED;
}
// @brief 计算(WEAK 函数, 驱动重定义)
__WEAK uint32_t htd_dev_crc32_update(htd_crc32_ctx_t* p_ctx, const uint8_t* p_data, uint32_t data_len)
{
    return 0;
}
// @brief 结束(WEAK 函数, 驱动重定义)
__WEAK int32_t htd_dev_crc32_final(htd_crc32_ctx_t* p_ctx)
{
    ASSERT(p_ctx);
    return 0;
}


// @brief 开始; 优先使用硬件，如果没有硬件或者硬件正在运行，自动使用软件
int32_t htd_crc32_init(htd_crc32_ctx_t* p_ctx)
{
    ASSERT(p_ctx);
    if (htd_dev_crc32_init(p_ctx) == ERROR_CODE_SUCCESS) {
        p_ctx->user_hw_flag = 1;
        return 0;
    }
    p_ctx->user_hw_flag = 0;
    return 0;
}
// @brief 计算
uint32_t htd_crc32_update(htd_crc32_ctx_t* p_ctx, const uint8_t* p_data, uint32_t data_len)
{
    ASSERT(p_ctx);
    if (p_ctx->user_hw_flag == 0) {
        if (p_ctx->crc32_type == HTD_CRC32_GEN) {
            p_ctx->result = htd_sfw_crc32(p_data, data_len, p_ctx->result);
        } else if (p_ctx->crc32_type == HTD_CRC32_MPEG2) {
            p_ctx->result = htd_sfw_crc32_mpeg2(p_data, data_len, p_ctx->result);
        } else {
            ASSERT(0);
        }
        return p_ctx->result;
    }
    return htd_dev_crc32_update(p_ctx, p_data, data_len);
}
// @brief 结束
uint32_t htd_crc32_final(htd_crc32_ctx_t* p_ctx)
{
    ASSERT(p_ctx);
    htd_dev_crc32_final(p_ctx);
    return p_ctx->result;
}






//////////////////////////////////////////////////////////////////////
#if (defined(HTD_CONSOLE_ENABLE) && (DEBUG_LVL > 2))
#include "htd_console_process.h"

// CRC在线网址：https://www.ip33.com/crc.html

// @brief crc8 调试
void crc8_test_handle(void)
{
    uint8_t crc8val;
    htd_crc8_ctx_t ctx;
    uint8_t buff[] = {48, 49, 50, 51, 52, 53, 54, 55, 56, 57};
    
    ctx = m_crc8_ctx_def;
    htd_crc8_init(&ctx);
    htd_crc8_update(&ctx, buff, 10);
    htd_crc8_final(&ctx);
    crc8val = htd_sfw_crc8(buff, 10, 0);
    if (crc8val == ctx.result) {        // 0x45
        print_warning("CRC8 test success. 0x%02x", ctx.result);
    } else {
        print_debug("CRC8 test failed. 0x%02x(0x%02x)", ctx.result, crc8val);
    }

    ctx = m_crc8_ctx_itu_def;
    htd_crc8_init(&ctx);
    htd_crc8_update(&ctx, buff, 10);
    htd_crc8_final(&ctx);
    crc8val = htd_sfw_crc8_itu(buff, 10, 0);
    if (crc8val == ctx.result) {        // 0x10
        print_warning("CRC8/ITU test success. 0x%02x", ctx.result);
    } else {
        print_debug("CRC8/ITU test failed. 0x%02x(0x%02x)", ctx.result, crc8val);
    }

    ctx = m_crc8_ctx_rohc_def;
    htd_crc8_init(&ctx);
    htd_crc8_update(&ctx, buff, 10);
    htd_crc8_final(&ctx);
    crc8val = htd_sfw_crc8_rohc(buff, 10, 0xff);
    if (crc8val == ctx.result) {        // 0xE3
        print_warning("CRC8/ROHC test success. 0x%02x", ctx.result);
    } else {
        print_debug("CRC8/ROHC test failed. 0x%02x(0x%02x)", ctx.result, crc8val);
    }

    ctx = m_crc8_ctx_maxim_def;
    htd_crc8_init(&ctx);
    htd_crc8_update(&ctx, buff, 10);
    htd_crc8_final(&ctx);
    crc8val = htd_sfw_crc8_maxim(buff, 10, 0);
    if (crc8val == ctx.result) {        // 0x75
        print_warning("CRC8/MAXIM test success. 0x%02x", ctx.result);
    } else {
        print_debug("CRC8/MAXIM test failed. 0x%02x(0x%02x)", ctx.result, crc8val);
    }
}

// @brief crc16 调试
void crc16_test_handle(void)
{
    htd_crc16_ctx_t ctx;
    uint8_t buff[] = {48, 49, 50, 51, 52, 53, 54, 55, 56, 57};

    // CRC16/CCITT-FALSE
    ctx = m_crc16_ctx_ccitt_false_def;
    htd_crc16_init(&ctx);
    htd_crc16_update(&ctx, buff, 10);
    htd_crc16_final(&ctx);
    if (0x7D61 == ctx.result) {         // 0x7D61
        print_warning("CRC16/CCITT-FALSE test success. 0x%04x", ctx.result);
    } else {
        print_debug("CRC16/CCITT-FALSE test failed. 0x%04x", ctx.result);
    }

    // CRC16/CCITT
    ctx = m_crc16_ctx_ccitt_def;
    htd_crc16_init(&ctx);
    htd_crc16_update(&ctx, buff, 10);
    htd_crc16_final(&ctx);
    if (0x5F6E == ctx.result) {         // 0x5F6E
        print_warning("CRC16/CCITT test success. 0x%04x", ctx.result);
    } else {
        print_error("CRC16/CCITT test failed. 0x%04x", ctx.result);
    }

    // CRC16/X25
    ctx = m_crc16_ctx_x25_def;
    htd_crc16_init(&ctx);
    htd_crc16_update(&ctx, buff, 10);
    htd_crc16_final(&ctx);
    if (0x3C16 == ctx.result) {         // 0x3C16
        print_warning("CRC16/X25 test success. 0x%04x", ctx.result);
    } else {
        print_error("CRC16/X25 test failed. 0x%04x", ctx.result);
    }

    // CRC16/XMODEM
    ctx = m_crc16_ctx_xmodem_def;
    htd_crc16_init(&ctx);
    htd_crc16_update(&ctx, buff, 10);
    htd_crc16_final(&ctx);
    if (0x9C58 == ctx.result) {         // 0x9C58
        print_warning("CRC16/XMODEM test success. 0x%04x", ctx.result);
    } else {
        print_error("CRC16/XMODEM: test failed. 0x%04x", ctx.result);
    }

    // CRC16/MODBUS
    ctx = m_crc16_ctx_modbus_def;
    htd_crc16_init(&ctx);
    htd_crc16_update(&ctx, buff, 10);
    htd_crc16_final(&ctx);
    if (0x434D == ctx.result) {         // 0x434D
        print_warning("CRC16/MODBUS test success. 0x%04x", ctx.result);
    } else {
        print_error("CRC16/MODBUS: test failed. 0x%04x", ctx.result);
    }
}

// @brief CRC32 调试
void crc32_test_handle(void)
{
    htd_crc32_ctx_t ctx;
    uint8_t buff[] = {48, 49, 50, 51, 52, 53, 54, 55, 56, 57};

    ctx = m_crc32_ctx_mpeg2_def;
    htd_crc32_init(&ctx);
    htd_crc32_update(&ctx, buff, 10);
    htd_crc32_final(&ctx);
    if (ctx.result == 0x694F1B1F) {         // 0~9: 0x694F1B1F
        print_warning("CRC32/MPEG-2 test success.");
    } else {
        print_error("CRC32/MPEG-2 test failed. 0x%08x(0x694F1B1F)", ctx.result);
    }
    ctx.result = htd_sfw_crc32_mpeg2(buff, 10, 0xffffffff);
    print_debug("CRC32/MPEG-2 sfw: 0x%08x", ctx.result);
    
    // 测量分段计算
    ctx = m_crc32_ctx_gen_def;
    htd_crc32_init(&ctx);
    htd_crc32_update(&ctx, buff, 10);
    htd_crc32_final(&ctx);
    if (ctx.result == 0xA684C7C6) {         // 0xA684C7C6
        print_warning("CRC32 test success.");
    } else {
        print_error("CRC32 test failed. 0x%08x(0xA684C7C6)", ctx.result);
    }
    ctx.result = htd_sfw_crc32(buff, 10, 0xffffffff);
    print_debug("CRC32 sfw: 0x%08x", ctx.result);
}


// @brief 调试命令
static int crc_cmd(int argc, char **argv)
{
    crc8_test_handle();
    crc16_test_handle();
    crc32_test_handle();
    return 0;
}
CONSOLE_CMD_EXPORT(crc_cmd, crc);
#endif /*(defined(HTD_CONSOLE_ENABLE) && (DEBUG_LVL > 0))*/


