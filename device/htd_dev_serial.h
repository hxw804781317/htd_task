// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_DEV_SERIAL_H_
#define _HTD_DEV_SERIAL_H_

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


#define HTD_SERIAL_CMD_NONE             0
#define HTD_SERIAL_CMD_OPEN             1
#define HTD_SERIAL_CMD_CLOSE            2
#define HTD_SERIAL_CMD_READ             3
#define HTD_SERIAL_CMD_WRITE            4
#define HTD_SERIAL_CMD_READ_START       5               // 
#define HTD_SERIAL_CMD_WRITE_FLUSH      6               // 参数为 uint32_t 超时时间，单位毫秒
#define HTD_SERIAL_CMD_DUMP             7
typedef uint32_t htd_serial_cmd_t;

#define HTD_DATA_BITS_5                 5
#define HTD_DATA_BITS_6                 6
#define HTD_DATA_BITS_7                 7
#define HTD_DATA_BITS_8                 8
#define HTD_DATA_BITS_9                 9
#define HTD_DATA_BITS_16                16
typedef uint8_t htd_data_bits_t;

#define HTD_STOP_BITS_1                 0
#define HTD_STOP_BITS_2                 1
#define HTD_STOP_BITS_3                 2
#define HTD_STOP_BITS_4                 3
typedef uint8_t htd_stop_bits_t;

#define HTD_PARI_NONE                   0
#define HTD_PARI_ODD                    1
#define HTD_PARI_EVEN                   2
typedef uint8_t htd_parity_t;

#define HTD_HWFC_NONE                   0 
#define HTD_HWFC_RTS                    1 
#define HTD_HWFC_CTS                    2
#define HTD_HWFC_RTS_CTS                (HTD_HWFC_RTS | HTD_HWFC_CTS)
typedef uint8_t htd_hwfc_t;

// 电平采样，越高抗干扰越强，波特率可配置范围越低；越低抗干扰越差，波特率可配置范围越高
#define HTD_OVER_SAMPLE_8BIT            0
#define HTD_OVER_SAMPLE_16BIT           1
#define HTD_OVER_SAMPLE_32BIT           2
typedef uint8_t htd_oversample_t;

// 事件
#define HTD_SERIAL_EVT_NONE             0
#define HTD_SERIAL_EVT_RX               1       // 通告接收到数据，调用 htd_serial_read 读取数据
typedef uint32_t htd_serial_evt_t;

typedef struct {
    uint8_t* p_data;
    uint32_t data_len;
    uint32_t timeout_ms;
} htd_serial_xfer_t;

// 控制句柄
typedef struct htd_serial_param_s htd_serial_param_t;
typedef struct htd_serial_handle_s {
    const htd_dev_t *p_dev;
    const htd_serial_param_t* p_user_config;
    void* p_cdc;
}htd_serial_handle_t;
#define HTD_SERIAL_DEV_IDENTIFY         (HTD_DEV_MAGIC_MASK | 0xa5ff)
typedef void (*serial_evt_notify_func_t)(htd_serial_handle_t* p_handle, htd_serial_evt_t evt, const uint8_t* p_data, uint32_t data_len, bool from_irq);


// 配置结构体
struct htd_serial_param_s {
    pin_num_t pin_tx;
    pin_num_t pin_rx;
    uint32_t baudrate;                  // 波特率
    htd_data_bits_t data_bits;          // 数据位
    htd_stop_bits_t stop_bits;          // 停止位
    htd_parity_t parity;                // 奇偶校验 HTD_PARITY_ODD
    
    htd_oversample_t oversample;        // 采样
    bool is_bit_order_msb;              // 是否为大端
    htd_hwfc_t hwfc;                    // 硬件流速控制
    
    uint16_t wfifo_buffer_len;          // 写缓冲大小
    uint16_t rfifo_buffer_len;          // 读缓冲大小
    serial_evt_notify_func_t evt_notify_func;
};
#define HTD_SERIAL_CONFIG_DEF(_pin_tx, _pin_rx, _baudrate, _evt_func)       {       \
        .pin_tx = _pin_tx,                      \
        .pin_rx = _pin_rx,                      \
        .baudrate = _baudrate,                  \
        .data_bits = HTD_DATA_BITS_8,           \
        .stop_bits = HTD_STOP_BITS_1,           \
        .parity = HTD_PARI_NONE,                \
        .oversample = HTD_OVER_SAMPLE_16BIT,    \
        .is_bit_order_msb = 0,                  \
        .hwfc = HTD_HWFC_NONE,                  \
        .wfifo_buffer_len = 512,                \
        .rfifo_buffer_len = 512,                \
        .evt_notify_func = _evt_func,           \
    }





// @brief 打开
// @param rx_notify_handle 接收数据通告函数，如果p_data为空，调用 htd_uart_read 读取数据
htd_serial_handle_t* htd_serial_open(const char* p_name, const htd_serial_param_t* p_user_config);
// @brief 关闭, 引脚默认会配置为浮空
int32_t htd_serial_close(htd_serial_handle_t* p_handle);
// @brief 读数据
int32_t htd_serial_read(htd_serial_handle_t* p_handle, uint8_t* p_data, int32_t data_len, uint32_t timeout_ms);
// @brief 写数据
int32_t htd_serial_write(htd_serial_handle_t* p_handle, const uint8_t* p_data, int32_t data_len);
// @brief 控制
int32_t htd_serial_ctrl(htd_serial_handle_t* p_handle, htd_serial_cmd_t cmd, void* p_param, uint32_t param_len);


#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_DEV_SERIAL_H_*/

