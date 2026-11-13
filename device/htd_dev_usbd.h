// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_DEV_USBD_H_
#define _HTD_DEV_USBD_H_

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


//////////////////////////////////////////////////////////////////////
#define HTD_USBD_TYPE_NONE              0
#define HTD_USBD_TYPE_CDC_ACM           1
#define HTD_USBD_TYPE_CDC_RNDIS         2
#define HTD_USBD_TYPE_CDC_ECM           3
#define HTD_USBD_TYPE_CDC_NCM           4
#define HTD_USBD_TYPE_ADB               5
#define HTD_USBD_TYPE_DFU               6
#define HTD_USBD_TYPE_HID               7
#define HTD_USBD_TYPE_MSC               8
#define HTD_USBD_TYPE_MTP               9
#define HTD_USBD_TYPE_VIDEO             10
#define HTD_USBD_TYPE_AUDIO             11
#define HTD_USBD_TYPE_VENDOR_DISPLAY    12
#define HTD_USBD_TYPE_GAMEPAD           13
typedef uint8_t htd_usbd_type_t;

#define HTD_USBD_EVT_NONE               0           // 
#define HTD_USBD_EVT_RX                 1           // 
#define HTD_USBD_EVT_LINK               2           // 
#define HTD_USBD_EVT_UNLINK             3           // 
#define HTD_USBD_EVT_DFU_START          0x10        // 
#define HTD_USBD_EVT_DFU_END            0x11        // 
#define HTD_USBD_EVT_DFU_RESET          0x12        // 
#define HTD_USBD_EVT_DFU_READ           0x13        // @htd_usbd_dfu_xfer_t
#define HTD_USBD_EVT_DFU_WRITE          0x14        // @htd_usbd_dfu_xfer_t
#define HTD_USBD_EVT_DFU_ERASE          0x15        // @uint32_t
#define HTD_USBD_EVT_MSC_READ           0x20        // @htd_usbd_msc_xfer_t
#define HTD_USBD_EVT_MSC_WRITE          0x21        // @htd_usbd_msc_xfer_t
#define HTD_USBD_EVT_MSC_ERASE          0x22        // 
typedef uint32_t htd_usbd_evt_t;

#ifndef BSP_USBD_MSC_DEV_COUNT
    #define BSP_USBD_MSC_DEV_COUNT      4
#endif 

typedef struct {
    uint8_t* p_data;
    uint32_t data_len;
    uint32_t timeout_ms;
} htd_usbd_xfer_t;

typedef struct {
    uint32_t addr;
    uint8_t* p_data;
    uint32_t data_len;
} htd_usbd_dfu_xfer_t;

typedef struct {
    uint8_t idx;
    uint32_t sector;
    uint8_t* p_data;
    uint32_t sector_count;
} htd_usbd_msc_xfer_t;


// 控制命令
#define HTD_USBD_CMD_NONE               0
#define HTD_USBD_CMD_OPEN               1           // @htd_usbd_cdc_acm_param_t
#define HTD_USBD_CMD_CLOSE              2           // 
#define HTD_USBD_CMD_READ               3           // @htd_usbd_xfer_t
#define HTD_USBD_CMD_WRITE              4           // @htd_usbd_xfer_t
typedef uint32_t htd_usbd_cmd_t;

typedef struct htd_usbd_handle_s htd_usbd_handle_t;
struct htd_usbd_handle_s{
    htd_slist_node_t node;
    const htd_dev_t *p_dev;
    uint8_t ep[3];
    htd_usbd_type_t type;
    uint8_t tx_busy_flag;
    uint8_t interface_num;
    uint16_t config_descriptor_len;
    uint8_t* p_config_descriptor;
    void* param;
    void* interface_info;
    void (*destroy_handle)(htd_usbd_handle_t* p_handle);
    void* serial;
};
#define HTD_USBD_DEV_IDENTIFY           (HTD_DEV_MAGIC_MASK | 0xc547)
typedef int32_t (*usbd_evt_notify_t)(htd_usbd_handle_t* p_handle, htd_usbd_evt_t evt, uint8_t* p_param, uint32_t param_len, bool from_irq);

// CDC-ACM 参数
typedef struct {
    uint32_t baudrate;                  // 波特率
    htd_data_bits_t data_bits;          // 数据位
    htd_stop_bits_t stop_bits;          // 停止位
    htd_parity_t parity;                // 奇偶校验 UART_PARITY_ODD
    uint16_t wfifo_buffer_len;          // 写缓冲大小
    uint16_t rfifo_buffer_len;          // 读缓冲大小
    usbd_evt_notify_t evt_notify_handle;// 通告事件
} htd_usbd_cdc_acm_param_t;

// DFU 参数
typedef struct {
    usbd_evt_notify_t evt_notify_handle;// 通告事件
} htd_usbd_dfu_param_t;

// MSC 参数
typedef struct {
    uint32_t block_count[BSP_USBD_MSC_DEV_COUNT];               // 块数量
    uint32_t block_size[BSP_USBD_MSC_DEV_COUNT];                // 块大小
    usbd_evt_notify_t evt_notify_handle;// 通告事件
} htd_usbd_msc_param_t;


// MTP 参数
typedef struct {
    usbd_evt_notify_t evt_notify_handle;// 通告事件
}htd_usbd_mtp_param_t;




//////////////////////////////////////////////////////////////////////
// @brief 打开
htd_usbd_handle_t* htd_usbd_open(const char* p_name, void* p_param);
// @brief 关闭
int32_t htd_usbd_close(htd_usbd_handle_t *p_handle);
// @brief 发送数据
int32_t htd_usbd_write(htd_usbd_handle_t* p_handle, const uint8_t* p_data, uint32_t data_len, uint32_t timeout_ms);
// @brief 读取数据
int32_t htd_usbd_read(htd_usbd_handle_t* p_handle, uint8_t* p_data, uint32_t data_len, uint32_t timeout_ms);


    
#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_DEV_USBD_H_*/

