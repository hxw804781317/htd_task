// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL       4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG       "htd.console"
#include "htd_device.h"
#include "htd_console_process.h"


#if defined(HTD_TASK_ENABLE)
//////////////////////////////////////////////////////////////////////
static htd_serial_handle_t* m_console_handle = NULL;
static volatile uint32_t m_rx_int_ms = 0;


// @brief 控制台写数据
static int32_t app_console_write(const uint8_t* p_data, int32_t data_len)
{
    //htd_critical_enter();     // 使用 freeRTOS，中断里面调用打印函数会卡死在这里，因此注释掉
    data_len = htd_serial_write(m_console_handle, p_data, data_len);
    //htd_critical_exit();
    return data_len;
}

#ifdef HTD_CONSOLE_SUPPORT_CMD
// @brief 控制台读数据句柄
static void console_read_data_handle(void* arg)
{
    uint8_t data[32];
    int32_t data_len, i;
    htd_serial_handle_t* p_handle = (htd_serial_handle_t*)arg;

    m_rx_int_ms = 0;
    do {
       data_len = htd_serial_read(p_handle, data, sizeof(data), 0);
       if (data_len == 0) {
            break;
       }
       //print_hex("data:", data, data_len);
       for (i=0; i<data_len; ++i) {
            htd_console_process_loop(data[i]);
       }
       //print_debug("data_len=%d, i=%d", data_len, i);
    } while (1);
}

// @brief 读事件处理, 底层驱动调用
static void console_read_evt_notify(htd_serial_handle_t* p_handle, htd_serial_evt_t evt, const uint8_t* p_data, uint32_t data_len, bool from_irq)
{
    if (evt == HTD_SERIAL_EVT_RX) {
        if (p_data == NULL || data_len == 0) {
            if (m_rx_int_ms == 0) {
                if (from_irq) {
                    if (htd_task_load_function_fisr(console_read_data_handle, p_handle)) {
                        m_rx_int_ms = htd_time_ms_get_fisr();
                    }
                } else {
                    if (htd_task_load_function(console_read_data_handle, p_handle)) {
                        m_rx_int_ms = htd_time_ms_get();
                    }
                }
            }
        } else {
            //htd_console_print("%s", p_data);
            while (data_len--) {
                htd_console_process_loop(*p_data++);
           }
        }
    }
}
#endif /*HTD_CONSOLE_SUPPORT_CMD*/

// @brief 控制台开始接收数据
void app_console_read_start(void)
{
#ifdef HTD_CONSOLE_SUPPORT_CMD
    if (m_console_handle) {
        htd_serial_ctrl(m_console_handle, HTD_SERIAL_CMD_READ_START, m_console_handle, sizeof(htd_serial_handle_t));
    }
#endif
}

// @brief 初始化控制台(打开字符输出)
// @note 内部使用了 htd_malloc，必须在 ram_heap_init 初始化后再调用
int32_t app_console_init(void)
{
#ifdef HTD_CONSOLE_SUPPORT_CMD
    // 配置控制台
    static const htd_serial_param_t serial_config = HTD_SERIAL_CONFIG_DEF(HTD_CONSOLE_SERIAL_TX_PINNUM, HTD_CONSOLE_SERIAL_RX_PINNUM, HTD_CONSOLE_SERIAL_BAUDRATE, console_read_evt_notify);
#else
    static const htd_serial_param_t serial_config = HTD_SERIAL_CONFIG_DEF(HTD_CONSOLE_SERIAL_TX_PINNUM, HTD_CONSOLE_SERIAL_RX_PINNUM, HTD_CONSOLE_SERIAL_BAUDRATE, NULL);
#endif
    m_console_handle = htd_serial_open(HTD_CONSOLE_SERIAL_NAME, &serial_config);
    if (m_console_handle == NULL) {
        ASSERT(0);
        return -1;
    }
    
    // 初始化控制台
    int32_t ret = htd_console_process_init(app_console_write);
    if (ret != 0) {
        app_console_write((const uint8_t*)"htd console process init failed.", 32);
    } else {
        app_console_write((const uint8_t*)"htd console process init success.", 33);
        htd_console_print("htd console init finish: %s\r\n", HTD_CONSOLE_SERIAL_NAME);
    }
    return 0;
}

// @brief 去初始化控制台
int32_t app_console_uninit(void)
{
    if (m_console_handle) {
        htd_serial_close(m_console_handle);
        m_console_handle = NULL;
    }
    return 0;
}

// @brief 刷新
// @return 返回0说明已经刷新完毕，返回大于0，表示还有多长数据要发送
int32_t app_console_tx_flush(uint32_t waite_timeout_ms)
{
    return htd_serial_ctrl(m_console_handle, HTD_SERIAL_CMD_WRITE_FLUSH, &waite_timeout_ms, sizeof(uint32_t));
}
#endif /*HTD_TASK_ENABLE*/

