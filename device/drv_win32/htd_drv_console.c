// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#include "htd_device.h"
#if defined(_WIN32)
    #include <mmsystem.h>
    #include <conio.h>
#endif


typedef struct {
    htd_serial_handle_t* who;
}console_object_t;
static console_object_t m_console_object;
static void* m_console_thread = NULL;


// @brief 控制台获取按键
static void console_thread_for_key_get(void* param)
{
    /*
    * left  key(��)�� 0xe04b
    * up    key(��)�� 0xe048
    * right key(��)�� 0xe04d
    * down  key(��)�� 0xe050
    */
    unsigned char key;
    htd_serial_handle_t* p_handle = (htd_serial_handle_t*)param;

    uint8_t buff[8];
    uint32_t buff_len;
    for (;;) {
        buff_len = 0;
        memset(buff, 0, sizeof(buff));
        key = getch();
        if (key == 0xE0) {
            key = getch();
            if (key == 0x48) {          //up key , 0x1b 0x5b 0x41
                buff[buff_len++] = 0x1b;
                buff[buff_len++] = 0x5b;
                buff[buff_len++] = 0x41;
            } else if (key == 0x50) {   //0x1b 0x5b 0x42
                buff[buff_len++] = 0x1b;
                buff[buff_len++] = 0x5b;
                buff[buff_len++] = 0x42;
            } else if (key == 0x4b) {   //<- 0x1b 0x5b 0x44
                buff[buff_len++] = 0x1b;
                buff[buff_len++] = 0x5b;
                buff[buff_len++] = 0x44;
            } else if (key == 0x4d) {   //<- 0x1b 0x5b 0x43
                buff[buff_len++] = 0x1b;
                buff[buff_len++] = 0x5b;
                buff[buff_len++] = 0x43;
            }
            p_handle->p_user_config->evt_notify_func(p_handle, HTD_SERIAL_EVT_RX, buff, buff_len, false);
            continue;
        }

        buff[buff_len++] = key;
        p_handle->p_user_config->evt_notify_func(p_handle, HTD_SERIAL_EVT_RX, buff, buff_len, false);
    }
} 

// @brief 打开
static int32_t serial_win32_open(htd_serial_handle_t* p_handle)
{
    if (m_console_thread == NULL) {
        m_console_thread = htd_os_sch_init("console", 4, HTD_TASK_THREAD_PRIORITY, HTD_TASK_THREAD_STACK_SIZE);
        ASSERT(m_console_thread);
        // 调用读取按键内容
        htd_os_load_function(m_console_thread, console_thread_for_key_get, p_handle);
    }
    return 0;
}

// @brief 写入
static int32_t serial_win32_write(htd_serial_handle_t* p_handle, const uint8_t* p_data, uint32_t data_len)
{
    if (p_data && data_len) {
        fwrite(p_data, 1, data_len, stdout);
        fflush(stdout);
        return data_len;
    }
    return 0;
}

// @brief 控制句柄
static int32_t serial_win32_ctrl_handle(void* handle, uint32_t cmd, void* p_param, uint32_t param_len)
{
    htd_serial_handle_t* p_handle = (htd_serial_handle_t*)handle;
    if (cmd == HTD_SERIAL_CMD_OPEN) {
        return serial_win32_open(p_handle);
    }
    if (cmd == HTD_SERIAL_CMD_READ) {
        return 0;
    }
    if (cmd == HTD_SERIAL_CMD_WRITE) {
        htd_serial_xfer_t* p_xfer = (htd_serial_xfer_t*)p_param;
        return serial_win32_write(p_handle, p_xfer->p_data, p_xfer->data_len);
    }
    return 0;
}
HTD_DEV_REGISTER(console, HTD_SERIAL_DEV_IDENTIFY, serial_win32_ctrl_handle, &m_console_object);


