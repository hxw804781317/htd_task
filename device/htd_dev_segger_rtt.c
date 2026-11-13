// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
//#define DEBUG_LVL       4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG       "drv.rtt"
#include "htd_device.h"
#include "htd_dev_serial.h"


#if defined(LIB_SEGGER_RTT_ENABLE)
//////////////////////////////////////////////////////////////////////
#include "SEGGER_RTT.h"

// @brief 发送数据
static int32_t segger_rtt_write_handle(htd_serial_handle_t* p_handle, const uint8_t* p_data, uint32_t data_len)
{
    if (p_data && data_len) {
        SEGGER_RTT_Write(0, p_data, data_len);
        return data_len;
    }
    return 0;
}

// @brief 读取数据
static int32_t segger_rtt_read_handle(htd_serial_handle_t* p_handle, uint8_t* p_data, uint32_t data_size, uint32_t timeout_ms)
{
    uint32_t retry_count = 2;       // 尝试读取次数
    int32_t read_len = 0;
    do {
        read_len = SEGGER_RTT_Read(0, p_data, data_size);
        if (read_len) {
            break;
        }
        retry_count--;
        if (retry_count == 0) {
            break;
        }
    } while (1);
    return read_len;
}

// @brief 打开
static int32_t segger_rtt_open_handle(htd_serial_handle_t* p_handle)
{
    const char* p_ptr = "segger RTT init finish.\r\n";
    SEGGER_RTT_Write(0, p_ptr, strlen(p_ptr));
    return 0;
}

// @brief 关闭
static int32_t segger_rtt_close_handle(htd_serial_handle_t* p_handle)
{
    return 0;
}

// @brief 定时通知读数据
static void segger_rtt_read_notify_handle(void* arg)
{
    htd_serial_handle_t* p_handle = (htd_serial_handle_t*)arg;
    p_handle->p_user_config->evt_notify_func(p_handle, HTD_SERIAL_EVT_RX, NULL, 0, false);
}

// @brief 等待RTT缓冲区数据发送完毕, 此函数会阻塞CPU，直到数据被读走或超时
static int32_t segger_rtt_tx_flush(uint32_t timeout_ms)
{
    uint32_t buff_len;
    uint32_t start_ms = htd_time_ms_get();
    while (1) {
        buff_len = SEGGER_RTT_WriteBuffLen_Get(0);
        if (buff_len == 0) {
            return 0;
        }
        if (htd_time_ms_diff(htd_time_ms_get(), start_ms) > timeout_ms) {
            //print_warning("tx flush timeout. buff_len=%d", buff_len);
            break;
        }
        htd_delay_us(1);
    }
    return (int32_t)buff_len;
}

// @brief 控制句柄
static int32_t segger_rtt_ctrl_handle(void* handle, htd_serial_cmd_t cmd, void* p_param, uint32_t param_len)
{
    htd_serial_handle_t* p_handle = (htd_serial_handle_t*)handle;
    if (cmd == HTD_SERIAL_CMD_OPEN) {
        return segger_rtt_open_handle(p_handle);
    } else if (cmd == HTD_SERIAL_CMD_CLOSE) {
        return segger_rtt_close_handle(p_handle);
    } else if (cmd == HTD_SERIAL_CMD_READ) {
        htd_serial_xfer_t *p_xfer = (htd_serial_xfer_t*)p_param;
        return segger_rtt_read_handle(p_handle, p_xfer->p_data, p_xfer->data_len, p_xfer->timeout_ms);
    } else if (cmd == HTD_SERIAL_CMD_WRITE) {
        htd_serial_xfer_t *p_xfer = (htd_serial_xfer_t*)p_param;
        return segger_rtt_write_handle(p_handle, p_xfer->p_data, p_xfer->data_len);
    } else if (cmd == HTD_SERIAL_CMD_READ_START) {
        //print_debug("load timer function");
        // 定时读取
        htd_task_load_timer_function(true, 50, segger_rtt_read_notify_handle, (htd_serial_handle_t*)p_param);
        return 0;
    } else if (cmd == HTD_SERIAL_CMD_WRITE_FLUSH) {
        uint32_t timeout_ms = *(uint32_t*)p_param;
        return segger_rtt_tx_flush(timeout_ms);
    } else {
        return (-ERROR_CODE_NOT_SUPPORTED);
    }
}
HTD_DEV_REGISTER(segger_rtt, HTD_SERIAL_DEV_IDENTIFY, segger_rtt_ctrl_handle, NULL);
#endif /*defined(USE_LIB_SEGGER_RTT)*/

