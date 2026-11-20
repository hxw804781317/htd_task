// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL       4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG       "ymodem"
#include <stddef.h>
#include <stdbool.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include "htd_boot_ymodem.h"
#include "htd_device.h"

#if defined(HTD_BOOT_SUPPORT_SERIAL_YMODEM)
/* SOH/STX + seq + payload + crc */
#define YMODEM_SOH_PKG_LEN      (128+(1+2+2))
#define YMODEM_STX_PKG_LEN      (1024+(1+2+2))


// @brief 读取命令
static ymodem_cmd_t ymodem_cmd_read(ymodem_ctx_t *p_ctx, uint32_t timeout_ms)
{
    uint8_t cmd = YMODEM_CMD_NONE;
    if (p_ctx->serial_read_handle(&cmd, sizeof(uint8_t), timeout_ms) == 1) {
        return cmd;
    }
    return YMODEM_CMD_NONE;
}

// @brief 读取数据
static uint32_t ymodem_data_read(ymodem_ctx_t *p_ctx, uint8_t* p_data, uint32_t data_len)
{
    int32_t ret;
    uint32_t total_len = 0;
    while (1) {
        ret = p_ctx->serial_read_handle(p_data+total_len, data_len-total_len, 1000);
        if (ret <= 0) {
            break;
        }
        total_len += ret;
        if (total_len >= data_len) {
            return total_len;
        }
    } 
    return total_len;
}

// @brief 发送一个字节
static uint32_t ymodem_write_byte(ymodem_ctx_t *p_ctx, uint8_t cmd)
{
    return p_ctx->serial_write_handle(&cmd, sizeof(uint8_t));
}

// @brief 清空接收缓冲区
static void ymodem_rx_flush(ymodem_ctx_t *p_ctx)
{
    uint8_t data[16];
    uint32_t data_len;
    while (1) {
        data_len = p_ctx->serial_read_handle(data, sizeof(data), 0);
        if (data_len == 0) {
            break;
        }
    }
}

// @brief 发送握手包
static int32_t ymodem_handshake(ymodem_ctx_t* p_ctx, uint32_t timeout_ms)
{
    // 发送握手
    uint32_t data_sz;
    ymodem_write_byte(p_ctx, YMODEM_CMD_C);
    ymodem_cmd_t cmd = ymodem_cmd_read(p_ctx, timeout_ms);
    if (cmd == YMODEM_CMD_SOH) {
        data_sz = YMODEM_SOH_PKG_LEN - 1;
    } else if (cmd == YMODEM_CMD_STX) {
        data_sz = YMODEM_STX_PKG_LEN - 1;
    } else {
        return YMODEM_ERR_TIMEOUT;
    }

    // 读取数据
    uint32_t recv_len = ymodem_data_read(p_ctx, p_ctx->recv_data, data_sz);
    if (recv_len != data_sz) {
        boot_error("data size error.");
        return YMODEM_ERR_DATA_SIZE;
    }
    // 首包：数据检查, 包头和包头反码
    if (p_ctx->recv_data[0] != 0 || p_ctx->recv_data[1] != 0xFF) {
        boot_error("seq error. %02x,%02x", p_ctx->recv_data[0], p_ctx->recv_data[1]);
        return -YMODEM_ERR_SEQ;
    }
    uint16_t recv_crc = (uint16_t)(p_ctx->recv_data[data_sz-2] << 8) | p_ctx->recv_data[data_sz-1];
    if (recv_crc != htd_ymodem_crc16(&p_ctx->recv_data[2], data_sz-4)) {
        boot_error("handshake crc error. 0x%04x, data_sz=%d", recv_crc, data_sz);
        return YMODEM_ERR_CRC;
    }

    // 通知应用
    if (p_ctx->evt_notify_handle) {
        boot_debug("seq: 0x%02x", p_ctx->recv_data[0]);
        if (p_ctx->evt_notify_handle(YMODEM_EVT_START, &p_ctx->recv_data[2], data_sz-4) != 0) {
            return YMODEM_ERR_CANCEL;
        }
    }
    ymodem_write_byte(p_ctx, YMODEM_CMD_ACK);
    
    // 发送请求文件指令
    p_ctx->state = YMODEM_STATE_LINK;
    ymodem_write_byte(p_ctx, YMODEM_CMD_C);
    return YMODEM_ERR_OK;
}

// @brief 请求数据内容
static int32_t ymodem_request_data(ymodem_ctx_t *p_ctx, uint32_t data_sz, ymodem_cmd_t *cmd)
{
    // seq(2Byte) + data + crc(2Byte)
    const uint32_t recv_size = 2 + data_sz + 2;
    uint32_t recv_len = ymodem_data_read(p_ctx, p_ctx->recv_data, recv_size);
    if (recv_len != recv_size) {
        boot_error("data size error.");
        return YMODEM_ERR_DATA_SIZE;
    }

    if (p_ctx->state == YMODEM_STATE_LINK) {
        if (p_ctx->recv_data[0] == 0x00) {
            return YMODEM_ERR_OK;
        }
    }
    p_ctx->state = YMODEM_STATE_TRANSLATE;

    // 检查数据
    if ((p_ctx->recv_data[0] + p_ctx->recv_data[1]) != 0xFF) {
        boot_error("seq error. %02x,%02x", p_ctx->recv_data[0], p_ctx->recv_data[1]);
        return -YMODEM_ERR_SEQ;
    }
    uint16_t recv_crc = (uint16_t)(p_ctx->recv_data[recv_size-2] << 8) | p_ctx->recv_data[recv_size-1];
    if (recv_crc != htd_ymodem_crc16(&p_ctx->recv_data[2], data_sz)) {
        boot_error("crc error.");
        return YMODEM_ERR_CRC;
    }
    // 通告接收到数据
    if (p_ctx->evt_notify_handle) {
        boot_debug("seq: 0x%02x", p_ctx->recv_data[0]);
        if (0 == p_ctx->evt_notify_handle(YMODEM_EVT_RECV_DATA, &p_ctx->recv_data[2], data_sz)) {
            *cmd = YMODEM_CMD_ACK;
        } else {
            *cmd = YMODEM_CMD_CAN;
        }
    } else {
        *cmd = YMODEM_CMD_ACK;
    }
    return YMODEM_ERR_OK;
}

// @brief 传输数据
static int32_t ymodem_cmd_translate(ymodem_ctx_t *p_ctx, uint32_t timeout_ms)
{
    uint32_t data_sz;
    ymodem_cmd_t cmd = ymodem_cmd_read(p_ctx, timeout_ms);
    if (cmd == YMODEM_CMD_EOT) {            // 传输结束
        p_ctx->state = YMODEM_STATE_FINISHING;
        return YMODEM_ERR_OK;
    } else if (cmd == YMODEM_CMD_SOH) {     // 
        data_sz = 128;
    } else if (cmd == YMODEM_CMD_STX) {     // 
        data_sz = 1024;
    } else {
        boot_error("cmd error.");
        return YMODEM_ERR_CMD;           // 
    }
    
    int32_t err = ymodem_request_data(p_ctx, data_sz, &cmd);
    if (err != YMODEM_ERR_OK) {
        return err;
    }
    if (cmd == YMODEM_CMD_CAN) {
        data_sz = 5;
        while (data_sz--) {
            ymodem_write_byte(p_ctx, YMODEM_CMD_CAN);
        }
        boot_warning("cancel.");
        p_ctx->state = YMODEM_STATE_NONE;
        return YMODEM_ERR_CANCEL;
    }
    
    ymodem_write_byte(p_ctx, YMODEM_CMD_ACK);
    return err;
}

// @brief 处理结束指令
static int32_t ymodem_cmd_finishing(ymodem_ctx_t *p_ctx, uint32_t timeout_ms)
{
    // 发送 NAK 指令，通告上位机结束
    ymodem_write_byte(p_ctx, YMODEM_CMD_NAK);
    ymodem_cmd_t cmd = ymodem_cmd_read(p_ctx, timeout_ms);
    if (cmd != YMODEM_CMD_EOT) {
        boot_error("cmd error. cmd=0x%02x", cmd);
        return YMODEM_ERR_CMD;
    }
    ymodem_write_byte(p_ctx, YMODEM_CMD_ACK);

    // 查询是否还有文件继续传送
    uint32_t data_sz;
    ymodem_write_byte(p_ctx, YMODEM_CMD_C);
    cmd = ymodem_cmd_read(p_ctx, timeout_ms);
    if (cmd != YMODEM_CMD_SOH) {
        boot_error("cmd error. cmd=0x%02x", cmd);
        return YMODEM_ERR_CMD;
    }
    
    data_sz = YMODEM_SOH_PKG_LEN - 1;
    uint32_t recv_len = ymodem_data_read(p_ctx, p_ctx->recv_data, data_sz);
    if (recv_len != data_sz) {
        boot_error("data size error. %d->%d", data_sz, recv_len);
        return YMODEM_ERR_DATA_SIZE;
    }
    // 检查数据
    if (p_ctx->recv_data[0] != 0 || p_ctx->recv_data[1] != 0xFF) {
        boot_error("seq error. 0x%02x, 0x%02x", p_ctx->recv_data[0], p_ctx->recv_data[1]);
        return -YMODEM_ERR_SEQ;
    }
    uint16_t recv_crc = (uint16_t)(p_ctx->recv_data[data_sz-2] << 8) | p_ctx->recv_data[data_sz-1];
    if (recv_crc != htd_ymodem_crc16(&p_ctx->recv_data[2], data_sz-4)) {
        boot_error("crc error.");
        return YMODEM_ERR_CRC;
    }
    // 传送下一个文件, 此时为起始帧
    if (p_ctx->recv_data[2] != 0) {
        if (p_ctx->evt_notify_handle) {
            boot_debug("seq: 0x%02x", p_ctx->recv_data[0]);
            if (p_ctx->evt_notify_handle(YMODEM_EVT_START, &p_ctx->recv_data[2], data_sz-4) != 0) {
                return YMODEM_ERR_CANCEL;
            }
        } 
        p_ctx->state = YMODEM_STATE_TRANSLATE;
        return YMODEM_ERR_OK;
    }
    
    p_ctx->state = YMODEM_STATE_NONE; 
    // 发送 ACK，回话结束
    ymodem_write_byte(p_ctx, YMODEM_CMD_ACK);
    return YMODEM_ERR_OK;
}

// @brief 创建一个 Ymodem
ymodem_ctx_t* htd_ymodem_create(ymodem_evt_notify_t notify_handle, ymodem_serial_read_func_t serial_read, ymodem_serial_write_func_t serial_write)
{
    ymodem_ctx_t* p_ctx = htd_malloc(sizeof(ymodem_ctx_t));
    ASSERT(p_ctx);
    
    htd_memset(p_ctx, 0, sizeof(ymodem_ctx_t));
    p_ctx->evt_notify_handle = notify_handle;
    p_ctx->serial_read_handle = serial_read;
    p_ctx->serial_write_handle = serial_write;
    return p_ctx;
}

// @brief 不断轮训, 返回false代表已经结束，返回true则需要不断调用
bool htd_ymodem_process(ymodem_ctx_t* p_ctx, uint32_t timeout_ms)
{
    int32_t err_code;
    if (p_ctx->state == YMODEM_STATE_NONE) {
        err_code = ymodem_handshake(p_ctx, timeout_ms);
        if (err_code == YMODEM_ERR_TIMEOUT || err_code == YMODEM_ERR_OK) {
            return true;
        }
        ymodem_rx_flush(p_ctx);
        return false;
        
    } else if (p_ctx->state == YMODEM_STATE_LINK || p_ctx->state == YMODEM_STATE_TRANSLATE) {
        if (YMODEM_ERR_OK != ymodem_cmd_translate(p_ctx, timeout_ms)) {
            ymodem_rx_flush(p_ctx);
            return false;
        }
        
    } else if (p_ctx->state == YMODEM_STATE_FINISHING) {
        ymodem_cmd_finishing(p_ctx, timeout_ms);
        if (p_ctx->state == YMODEM_STATE_NONE) {
            return false;
        }
        
    } else {
        p_ctx->state = YMODEM_STATE_NONE;
    }
    return true;
}

// @brief 判断 ymodem 是否已经握手
bool htd_ymodem_is_linked(ymodem_ctx_t* p_ctx)
{
    return (p_ctx->state != YMODEM_STATE_NONE);
}

// @brief 获取文件名字和文件长度
const char* htd_ymodem_fileinfo_get(const uint8_t* p_param, uint32_t param_len, uint32_t* filesize)
{
    const char* p_filename = (const char*)p_param;
    uint32_t len = strlen(p_filename);
    p_param += len + 1;
    
    uint32_t size = 0;
    while (p_param[0]) {
        size *= 10;
        size += p_param[0] - '0';
        p_param++;
    }
    if (filesize) {
        *filesize = size;
    }
    return p_filename;
}

// @brief 销毁
int32_t htd_ymodem_destroy(ymodem_ctx_t* p_ctx)
{
    if (p_ctx) {
        htd_free(p_ctx);
    }
    return 0;
}

#endif /*defined(HTD_BOOT_SUPPORT_SERIAL_YMODEM)*/

