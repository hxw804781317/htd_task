// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_DEV_I2CM_H_
#define _HTD_DEV_I2CM_H_

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


/************************************************
i2c从寄存器a读取2个字节数据流程如下
    1、起始信号(S)
    2、发送从设备地址 + 写位
       等待ACK
    3、发送寄存器地址a
       等待ACK
    4、起始信号(S)
    5、发送从设备地址 + 读位
       等待ACK
    6、读取1字节数据
       主机发送 ACK
       读取1字节数据
       主机发送 NACK
    7、停止信号(P)
****************************************************
i2c 往寄存器a写入2个字节数据流程如下
    1、起始信号(S)
    2、发送从设备地址 + 写位（例如 7位地址 <<1 | 0）
       等待从设备ACK
    3、发送寄存器地址a（1字节）
       等待ACK
    4、发送数据(第一个字节)
       等待ACK
       发送数据(第二个字节)
       等待ACK
    5、停止信号(P)
****************************************************
起始信号：SDA 先拉低，SCL 后拉低
结束信号：SCL 先拉高，SDA 再拉高

*****************************************************/


//////////////////////////////////////////////////////////////////////
// 配置, DMA 功能有驱动配置编译时决定，不在应用层控制，这样逻辑更简单
typedef struct {
    pin_num_t pin_sck;
    pin_num_t pin_sda;
    uint8_t MSB:1;                              // 一般为 true
    uint32_t freq_hz;                           // 频率
} i2cm_user_config_t;
#define HTD_I2C_CONFIG_DEFAULT    { \
        .pin_sck = PIN_NUM_INVALID, \
        .pin_sda = PIN_NUM_INVALID, \
        .MSB = 1,                   \
        .freq_hz = 400*1000,        \
    }


// I2C 设备
typedef struct i2cm_dev_s i2cm_dev_t;
typedef struct i2cm_handle_s i2cm_handle_t;
typedef void (*i2c_rx_notify_handle_t)(i2cm_handle_t* p_handle, const uint8_t* p_data, size_t data_len);
struct i2cm_handle_s {
    const i2cm_dev_t *p_dev;
    const i2cm_user_config_t* p_user_config;
};

#define I2CM_MSG_FLAG_TX                (1u << 0)
#define I2CM_MSG_FLAG_RX                (1u << 1)
#define I2CM_MSG_FLAG_BLOCK             (1u << 2)
#define I2CM_MSG_FLAG_ADDR_10BIT        (1u << 3)
typedef uint16_t i2cm_msg_flag_t;
typedef struct {
    uint16_t slave_addr;
    i2cm_msg_flag_t flags;
    uint16_t len;
    uint8_t *buff;
}i2cm_msg_t;

#define HTD_I2CM_DEV_MAGIC               0x9afda53c
struct i2cm_dev_s {
    uint32_t magic;
    const char* p_name;
    void* p_object;
    // 打开
    int32_t (*open_handle)(i2cm_handle_t* p_handle);
    // 关闭
    int32_t (*close_handle)(i2cm_handle_t* p_handle);
    // 收发控制, 返回发送的个数; 返回时数据收发已经结束
    int32_t (*xfer_handle)(i2cm_handle_t* p_handle, i2cm_msg_t* msg, uint32_t msg_count, uint32_t timeout_ms); 
};

// 设备注册
#define HTD_I2CM_SECTION_NAME    htd_dev_i2cm
#define HTD_I2CM_DEV_REGISTER(name, open_func, close_func, xfer_func, object)   \
    HTD_SECTION_ITEM_REGISTER(HTD_I2CM_SECTION_NAME, static const i2cm_dev_t CONCAT_2(dev_i2cm_, name)) = {\
        .magic = HTD_I2CM_DEV_MAGIC,         \
        .p_name = STR(name),                \
        .p_object = object,                 \
        .open_handle = open_func,           \
        .close_handle = close_func,         \
        .xfer_handle = xfer_func,           \
    };



//////////////////////////////////////////////////////////////////////
// @brief 打开I2C
i2cm_handle_t* htd_i2cm_open(const char* p_name, const i2cm_user_config_t* p_user_config);
// @brief 关闭I2C, 引脚默认会配置为模拟输入
int32_t htd_i2cm_close(i2cm_handle_t* p_handle);


// @brief 交互数据(从机地址为7bit有效地址，没有读写位)
int32_t htd_i2cm_xfer(i2cm_handle_t* p_handle, i2cm_msg_t* msg, uint32_t msg_count, uint32_t timeout_ms);
// @brief 写数据(从机地址为7bit有效地址，没有读写位)
int32_t htd_i2cm_write(i2cm_handle_t* p_handle, uint16_t slave_addr, const uint8_t* p_data, uint16_t data_size);
// @brief 读数据(从机地址为7bit有效地址，没有读写位)
int32_t htd_i2cm_read(i2cm_handle_t* p_handle, uint16_t slave_addr, uint8_t* p_data, uint16_t data_size);
// @brief 读寄存器地址为8bit的数据(从机地址为7bit有效地址，没有读写位)
int32_t htd_i2cm_reg8_read(i2cm_handle_t* p_handle, uint16_t slave_addr, uint8_t reg, uint8_t* p_data, uint16_t data_size);
// @brief 写数据(从机地址为7bit有效地址，没有读写位)
int32_t htd_i2cm_reg8_write(i2cm_handle_t* p_handle, uint16_t slave_addr, uint8_t reg, const uint8_t* p_data, uint16_t data_size);


#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_DEV_I2CM_H_*/

