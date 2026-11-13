#define DEBUG_LVL       1       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG       "dev.serial"
#include "htd_dev_serial.h"
#include "htd_components.h"


//////////////////////////////////////////////////////////////////////
// @brief 打开, p_user_config 必须为static const
// @param rx_notify_handle 接收数据通告函数，如果p_data为空，调用 htd_serial_read 读取数据
htd_serial_handle_t* htd_serial_open(const char* p_name, const htd_serial_param_t* p_user_config)
{
    ASSERT(p_user_config);

    // 判断设备是否存在
    const htd_dev_t* p_dev = htd_dev_find(p_name, HTD_SERIAL_DEV_IDENTIFY);
    if (p_dev == NULL) {
        print_warning("%s open failed.", p_name);
        return NULL;
    }
    ASSERT(p_dev->ctrl_handle);

    // 创建句柄
    htd_serial_handle_t* p_handle = (htd_serial_handle_t*)htd_malloc(sizeof(htd_serial_handle_t));
    ASSERT(p_handle);
    htd_memset(p_handle, 0, sizeof(htd_serial_handle_t));
    p_handle->p_user_config = p_user_config;
    p_handle->p_dev = p_dev;
    
    // 打开串口
    int32_t ret = p_dev->ctrl_handle(p_handle, HTD_SERIAL_CMD_OPEN, NULL, 0);
    if (ret != 0) {
        htd_free(p_handle);
        print_warning("serial[%s] open failed. ret=%d", p_name, ret);
        return NULL;
    }
    print_debug("serial[%s] open success.", p_name);
    print_debug("wfifo_buffer_len=%d, rfifo_buffer_len=%d", p_user_config->wfifo_buffer_len, p_user_config->rfifo_buffer_len);
    return p_handle;
}

// @brief 关闭, 引脚默认会配置为模拟输入
int32_t htd_serial_close(htd_serial_handle_t* p_handle)
{
    if (p_handle == NULL) {
        return 0;
    }
    // 关闭
    if (p_handle->p_dev->ctrl_handle) {
        p_handle->p_dev->ctrl_handle(p_handle, HTD_SERIAL_CMD_CLOSE, NULL, 0);
        // 引脚设置为高阻态(保持引脚处于低电平状态)
        if (p_handle->p_user_config->pin_rx != PIN_NUM_INVALID) {
            htd_pin_input_analog_set(p_handle->p_user_config->pin_rx);
        }
        if (p_handle->p_user_config->pin_tx != PIN_NUM_INVALID) {
            htd_pin_input_analog_set(p_handle->p_user_config->pin_tx);
        }
    }
    // 销毁
    htd_free(p_handle);
    return 0;
}

// @brief 读数据
int32_t htd_serial_read(htd_serial_handle_t* p_handle, uint8_t* p_data, int32_t data_len, uint32_t timeout_ms)
{
    ASSERT(p_handle);
    htd_serial_xfer_t xfer;
    xfer.p_data = p_data;
    xfer.data_len = data_len;
    xfer.timeout_ms = timeout_ms;
    return p_handle->p_dev->ctrl_handle(p_handle, HTD_SERIAL_CMD_READ, &xfer, sizeof(htd_serial_xfer_t));
}

// @brief 写数据
int32_t htd_serial_write(htd_serial_handle_t* p_handle, const uint8_t* p_data, int32_t data_len)
{
    ASSERT(p_handle);
    htd_serial_xfer_t xfer;
    xfer.p_data = (uint8_t*)p_data;
    xfer.data_len = data_len;
    xfer.timeout_ms = 0;
    return p_handle->p_dev->ctrl_handle(p_handle, HTD_SERIAL_CMD_WRITE, &xfer, sizeof(htd_serial_xfer_t));
}

// @brief 控制
int32_t htd_serial_ctrl(htd_serial_handle_t* p_handle, htd_serial_cmd_t cmd, void* p_param, uint32_t param_len)
{
    ASSERT(p_handle);
    return p_handle->p_dev->ctrl_handle(p_handle, cmd, p_param, param_len);
}




//////////////////////////////////////////////////////////////////////
#if (defined(HTD_CONSOLE_ENABLE) && (DEBUG_LVL == 4))
static htd_serial_handle_t* m_serial_test_handle = NULL;
static volatile uint32_t m_notify_ms = 0;


// @brief 
static void serial_recv_handle(void* arg)
{
    uint8_t data[128];      // 透传时，这个值越大，一次读取的数据越多，发送次数就越少
    uint32_t data_len;
    htd_serial_handle_t* p_handle = (htd_serial_handle_t*)arg;
    m_notify_ms = 0;

    uint32_t write_len = 0;
    while (1) {
        htd_memset(data, 0, sizeof(data));
        data_len = htd_serial_read(p_handle, data, sizeof(data), 5);
        if (data_len == 0) {
            break;
        }
        print_hex("serial rx", data, data_len);
        htd_serial_write(p_handle, data, data_len);
        write_len += data_len;
    }
    //print_debug("write to fifo. len=%d", write_len);
}

// @brief 中断接收到数据了
static void serial_evt_notify_func(htd_serial_handle_t* p_handle, htd_serial_evt_t evt, const uint8_t* p_data, uint32_t data_len, bool from_irq)
{
    print_debug("evt=%d", evt);
    if (evt == HTD_SERIAL_EVT_RX) {
        if (m_notify_ms == 0) {
            if (htd_task_load_function_fisr(serial_recv_handle, (void*)p_handle)) {
                m_notify_ms = htd_time_ms_get_fisr();
            }
        }
    }
}

// @brief 调试命令
static int serial_cmd(int argc, char **argv)
{
    if (argc < 2) {
        print_error("Usage: %s open/close", argv[0]);
        return 0;
    }
    
    if (0 == memcmp(argv[1], "open", 5)) {
        if (argc != 3) {
            print_error("Usage: %s open [uart1/uart2...]", argv[0]);
            return 0;
        }
        
        const static htd_serial_param_t serial_config = HTD_SERIAL_CONFIG_DEF(PIN_NUM_GET(A, 9), PIN_NUM_GET(A, 10), 1000000, serial_evt_notify_func);
        //const static htd_serial_param_t serial_config = HTD_SERIAL_CONFIG_DEF(PIN_NUM_GET(A, 8), PIN_NUM_GET(A, 6), 1000000, serial_evt_notify_func);
        htd_serial_handle_t* p_handle = htd_serial_open(argv[2], &serial_config);
        if (p_handle == NULL) {
            print_error("open %s failed.", argv[2]);
            return 0;
        }
        m_serial_test_handle = p_handle;
        print_warning("open %s, baudrate=%d success.", argv[2], serial_config.baudrate);
        htd_serial_write(p_handle, (const uint8_t*)"111122223333444455556666\r\n", 26);
        htd_serial_write(p_handle, (const uint8_t*)"111122223333444455556666\r\n", 26);
        htd_serial_write(p_handle, (const uint8_t*)"111122223333444455556666\r\n", 26);
        return 0;
    } 
    
    // 关闭
    if (0 == memcmp(argv[1], "close", 6)) {
        if (m_serial_test_handle) {
            htd_serial_close(m_serial_test_handle);
            m_serial_test_handle = NULL;
        }
        return 0;
    } 
    
    if (0 == memcmp(argv[1], "show", 5)) {
        if (m_serial_test_handle) {
            htd_serial_ctrl(m_serial_test_handle, HTD_SERIAL_CMD_DUMP, NULL, 0);
        }
        return 0;
    } 

    print_error("Usage: %s open/close", argv[0]);
    return 0;
}
CONSOLE_CMD_EXPORT(serial_cmd, serial);
#endif /*(defined(HTD_CONSOLE_ENABLE) && (DEBUG_LVL > 0))*/

