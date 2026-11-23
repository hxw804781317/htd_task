#define DEBUG_LVL       4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG       "net.dev"
#include "htd_common.h"
#include "htd_network_dev.h"
#include "htd_device.h"
#include "htd_network_thread.h"


static const dev_network_t* m_network_dev = NULL;


// @brief 函数注册
int32_t htd_network_dev_register(const dev_network_t* p_dev)
{
    ASSERT(p_dev);
    if (NULL == m_network_dev) {
        m_network_dev = p_dev;
    }
    return 0;
}

// @brief 初始化
// @param p_macaddr 配置设备的mac地址
int32_t htd_network_dev_open(const char* p_macaddr)
{
    int32_t ret;
    network_thread_init(32);
    if (m_network_dev && m_network_dev->open_handle) {
        ret = m_network_dev->open_handle(p_macaddr);
        return ret;
    }
    return -1;
}

// @brief 去初始化(设备进入低功耗状态)
int32_t htd_network_dev_close(void)
{
    if (m_network_dev && m_network_dev->ctrl_handle) {
        return m_network_dev->ctrl_handle(NET_DEV_CMD_CLOSE, NULL, 0);
    }
    return -1;
}

// @brief 读数据
int32_t htd_network_dev_read(uint8_t* p_data, uint32_t data_len)
{
    ASSERT(m_network_dev);
    return m_network_dev->read_handle(p_data, data_len);
}

// @brief 写数据
int32_t htd_network_dev_write(const uint8_t* p_data, uint32_t data_len)
{
    ASSERT(m_network_dev);
    return m_network_dev->write_handle(p_data, data_len);
}

// @brief 获取设备名字
const char* htd_network_dev_name_get(char* p_name)
{
    ASSERT(m_network_dev);
    return m_network_dev->p_name;
}

// @brief 芯片控制
int32_t htd_network_dev_ctrl(net_dev_cmd_t cmd, void* p_param, uint32_t param_len)
{
    ASSERT(m_network_dev);
    return m_network_dev->ctrl_handle(cmd, p_param, param_len);
}

// @brief 信息显示
void htd_network_dev_dump(void)
{
    ASSERT(m_network_dev);
    print_debug("name=%s", m_network_dev->p_name);
    m_network_dev->ctrl_handle(NET_DEV_CMD_DUMP, NULL, 0);
}



//////////////////////////////////////////////////////////////////////
#if (defined(HTD_CONSOLE_ENABLE) && (DEBUG_LVL == 4) && 1)
#include "htd_console_process.h"

// @brief 设备调试
static void network_dev_test_handle(void)
{
    // 初始化
    const char mac[6] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
    if (0 != htd_network_dev_open(mac)) {
        print_error("network dev open failed.");
        return ;
    }
    htd_delay_ms(100);

    uint8_t buff[8];
    htd_network_dev_ctrl(NET_DEV_CMD_MAC_ADDR_GET, buff, sizeof(buff));
    print_hex("mac addr", buff, 6);

#if 0
    // 闪烁
    for (uint32_t i=0; i<100; ++i) {
        buff[0] = 1;
        htd_network_dev_ctrl(NET_DEV_CMD_LED1_ONOFF, &buff[0], 1);
        htd_delay_ms(200);
        buff[0] = 0;
        htd_network_dev_ctrl(NET_DEV_CMD_LED1_ONOFF, &buff[0], 1);
        htd_delay_ms(200);
    }
#endif
}

// @brief 调试命令
static int network_dev_cmd(int argc, char **argv)
{
    if (argc < 2) {
        print_error("Usage: %s test/show");
        return 0;
    }
    if (memcmp(argv[1], "test", 5) == 0) {
        network_dev_test_handle();
    } else if (memcmp(argv[1], "show", 5) == 0) {
        htd_network_dev_dump();
    }
    return 0;
}
CONSOLE_CMD_EXPORT(network_dev_cmd, netdev);
#endif /*(defined(HTD_CONSOLE_ENABLE) && (DEBUG_LVL == 4) && 1)*/


