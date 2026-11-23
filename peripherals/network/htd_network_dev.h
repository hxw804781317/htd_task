#ifndef _HTD_DEV_NETWORK_H_
#define _HTD_DEV_NETWORK_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


// 以太网最大帧长度
#ifndef MAX_FRAMELEN
    #define MAX_FRAMELEN                    1518
#endif

#define NET_DEV_INDICATE_NONE               0
#define NET_DEV_INDICATE_DATA_RECV          1
#define NET_DEV_INDICATE_PHY_LINK           2
typedef uint32_t net_dev_indicate_t;
typedef void (*net_dev_indicate_handle_t)(net_dev_indicate_t indicate, void* p_data, uint32_t data_len);

#define NET_DEV_CMD_NONE                    0
#define NET_DEV_CMD_INDICATE_HANDLE_SET     1
#define NET_DEV_CMD_CLOSE                   2
#define NET_DEV_CMD_LED1_ONOFF              3
#define NET_DEV_CMD_MAC_ADDR_GET            4
#define NET_DEV_CMD_DUMP                    5
typedef uint32_t net_dev_cmd_t;

typedef struct {
    const char* p_name;
    int32_t (*write_handle)(const uint8_t* p_buff, uint32_t buff_len);
    int32_t (*read_handle)(uint8_t* p_buff, uint32_t buff_len);
    int32_t (*ctrl_handle)(net_dev_cmd_t cmd, void* p_param, uint32_t param_len);
    int32_t (*open_handle)(const char* p_macaddr);
} dev_network_t;


// @brief 函数注册
int32_t htd_network_dev_register(const dev_network_t* p_dev);

// @brief 初始化
// @param p_macaddr 配置设备的mac地址
int32_t htd_network_dev_open(const char* p_macaddr);
// @brief 去初始化(设备进入低功耗状态)
int32_t htd_network_dev_close(void);

// @brief 读数据
int32_t htd_network_dev_read(uint8_t* p_data, uint32_t data_len);
// @brief 写数据
int32_t htd_network_dev_write(const uint8_t* p_data, uint32_t data_len);

// @brief 获取设备名字
const char* htd_network_dev_name_get(char* p_name);
// @brief 芯片控制
int32_t htd_network_dev_ctrl(net_dev_cmd_t cmd, void* p_param, uint32_t param_len);


#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_DEV_NETWORK_H_*/

