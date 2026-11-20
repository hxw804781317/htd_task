// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_BOOT_YMODEM_H_
#define _HTD_BOOT_YMODEM_H_

#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "htd_boot_port.h"
#include "htd_boot_config.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


// 控制码，指令
#define YMODEM_CMD_NONE             0x00
#define YMODEM_CMD_SOH              0x01        // 128 字节数据包起始符
#define YMODEM_CMD_STX              0x02        // 1024 字节数据包起始符
#define YMODEM_CMD_EOT              0x04        // 传输结束（End of Transmission）
#define YMODEM_CMD_ACK              0x06        // 确认应答（Acknowledge）
#define YMODEM_CMD_NAK              0x15        // 否定应答（Negative Acknowledge）
#define YMODEM_CMD_CAN              0x18        // 取消传输（Cancel），连发两个即可中止
#define YMODEM_CMD_C                0x43        // 请求使用 CRC-16 校验方式
typedef uint8_t ymodem_cmd_t;

// 状态码
#define YMODEM_STATE_NONE           0
#define YMODEM_STATE_LINK           1
#define YMODEM_STATE_TRANSLATE      2
#define YMODEM_STATE_FINISHING      3
typedef uint8_t ymodem_state_t;

// 事件
#define YMODEM_EVT_NONE             0
#define YMODEM_EVT_START            1
#define YMODEM_EVT_RECV_DATA        2
#define YMODEM_EVT_FINISH           3
typedef uint32_t ymodem_evt_t;

// 错误码
#define YMODEM_ERR_OK               0           // 正确
#define YMODEM_ERR_TIMEOUT          (-0x10)     // 超时
#define YMODEM_ERR_CMD              (-0x11)     // 指令错误
#define YMODEM_ERR_SEQ              (-0x12)     // SEQ 错误
#define YMODEM_ERR_CRC              (-0x13)     // 校验码错误
#define YMODEM_ERR_DATA_SIZE        (-0x14)     // 数据大小
#define YMODEM_ERR_CANCEL           (-0x15)     // 取消传输 Cancel
typedef int32_t ymodem_err_t;

// 读数据
typedef int32_t (*ymodem_serial_read_func_t)(uint8_t* p_data, int32_t data_len, uint32_t timeout_ms);
// 写数据
typedef int32_t (*ymodem_serial_write_func_t)(const uint8_t* p_data, int32_t data_len);
// 通告，处理成功返回0，失败返回错误码
typedef int32_t (*ymodem_evt_notify_t)(ymodem_evt_t evt, const uint8_t* p_param, uint32_t param_len);

// ymodem 结构体
typedef struct {
    ymodem_serial_read_func_t serial_read_handle;
    ymodem_serial_write_func_t serial_write_handle;
    ymodem_evt_notify_t evt_notify_handle;
    uint8_t recv_data[1024+5];
    ymodem_state_t state;
    boot_update_type_t update_type;
    void* p_dev;
} ymodem_ctx_t;


// @brief 创建一个 Ymodem
ymodem_ctx_t* htd_ymodem_create(ymodem_evt_notify_t notify_handle, ymodem_serial_read_func_t serial_read, ymodem_serial_write_func_t serial_write);
// @brief 不断轮训, 返回false代表已经结束，返回true则需要不断调用
bool htd_ymodem_process(ymodem_ctx_t* p_ctx, uint32_t timeout_ms);
// @brief 判断 ymodem 是否已经握手
bool htd_ymodem_is_linked(ymodem_ctx_t* p_ctx);
// @brief 获取文件名字和文件长度
const char* htd_ymodem_fileinfo_get(const uint8_t* p_param, uint32_t param_len, uint32_t* filesize);
// @brief 销毁
int32_t htd_ymodem_destroy(ymodem_ctx_t* p_ctx);


#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif

