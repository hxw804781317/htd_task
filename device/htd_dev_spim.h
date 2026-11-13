#ifndef _HTD_DEV_SPIM_H_
#define _HTD_DEV_SPIM_H_

#include <stdint.h>
#include <stdbool.h>
#include "htd_device.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


//////////////////////////////////////////////////////////////////////
// 控制命令
#define HTD_SPIM_CMD_NONE               0
#define HTD_SPIM_CMD_OPEN               1               // 
#define HTD_SPIM_CMD_CLOSE              2               // 
#define HTD_SPIM_CMD_XFER               3               // 
typedef uint32_t htd_spim_cmd_t;

// 模式
#define SPIM_MODE_0             0               // CPOL=0, CPHA=0
#define SPIM_MODE_1             1               // CPOL=0, CPHA=1
#define SPIM_MODE_2             2               // CPOL=1, CPHA=0
#define SPIM_MODE_3             3               // CPOL=1, CPHA=1
typedef uint8_t spim_mode_t;

#define SPIM_MSG_FLAG_TX        0x01
#define SPIM_MSG_FLAG_RX        0x02
#define SPIM_MSG_FLAG_BLOCK     0x04
typedef uint16_t spim_msg_flag_t;
typedef struct {
    spim_msg_flag_t flags;
    uint16_t len;
    uint8_t *buff;
} spim_msg_t;

// 交互结束通告，主要用于非阻塞模式; p_data 和 data_len 最后一次 xfer 操作传进来的指针和数据长度
typedef struct spim_handle_s spim_handle_t;
typedef void (*spim_xfer_finish_notify_t)(spim_handle_t* p_handle, const uint8_t* p_data, size_t data_len);

// 配置
typedef struct {
    pin_num_t pin_cs;
    pin_num_t pin_sck;
    pin_num_t pin_mosi;
    pin_num_t pin_miso;
    uint8_t cs_active_level:1;                  // CS引脚激活电平状态，默认0
    uint8_t is_1line:1;                         // 是否为单线
    uint8_t is_bit_order_msb:1;                 // 一般为 true
    uint8_t data_width;                         // 位宽，8/16
    spim_mode_t mode;                           // spi 模式
    uint32_t freq_hz;                           // 频率
    spim_xfer_finish_notify_t xfer_finish;
} spim_user_config_t;
#define HTD_SPI_MASTER_CONFIG_DEFAULT { \
        .pin_cs = PIN_NUM_INVALID,      \
        .pin_sck = PIN_NUM_INVALID,     \
        .pin_mosi = PIN_NUM_INVALID,    \
        .pin_miso = PIN_NUM_INVALID,    \
        .cs_active_level = 0,           \
        .is_1line = 0,                  \
        .is_bit_order_msb = 1,          \
        .data_width = 8,                \
        .mode = SPIM_MODE_0,            \
        .freq_hz = 8*1000*1000,         \
        .xfer_finish = NULL,            \
    }

// SPI 设备
struct spim_handle_s {
    const htd_dev_t *p_dev;
    const spim_user_config_t* p_user_config;
    uint8_t* p_data;
    uint32_t data_len;
    uint32_t data_pos;
};
#define HTD_SPIM_DEV_IDENTIFY           (HTD_DEV_MAGIC_MASK | 0xa567)








//////////////////////////////////////////////////////////////////////
// @brief 打开SPI
spim_handle_t* htd_spim_open(const char* p_name, const spim_user_config_t* p_user_config);
// @brief 关闭SPI; cs引脚默认拉高(外部设备如果掉电，需要将cs引脚配置为拉低); SCK和MOSI配置为拉低，MISO配置为模拟输入
int32_t htd_spim_close(spim_handle_t* p_handle);
// @brief 多个写读数据操作
int32_t htd_spim_xfer(spim_handle_t* p_handle, spim_msg_t* p_msg, uint32_t msg_count);
// @brief 通信交互
int32_t htd_spim_transfer(spim_handle_t* p_handle, const uint8_t* tx, uint16_t tx_len, uint8_t* rx, uint16_t rx_len);





#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_DEV_SPIM_H_*/

