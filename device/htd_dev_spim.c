// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL       4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG       "dev.spim"
#include "htd_device.h"
#include "htd_dev_spim.h"


// @brief 打开 SPIM
spim_handle_t* htd_spim_open(const char* p_name, const spim_user_config_t* p_user_config)
{
    const htd_dev_t* p_dev = htd_dev_find(p_name, HTD_SPIM_DEV_IDENTIFY);
    if (p_dev == NULL) {
        return NULL;
    }
    
    // 创建新的设备
    spim_handle_t* p_handle = htd_malloc(sizeof(spim_handle_t));
    ASSERT(p_handle);
    htd_memset(p_handle, 0, sizeof(spim_handle_t));
    p_handle->p_dev = p_dev;
    p_handle->p_user_config = p_user_config;
    
    // 打开驱动设备
    int32_t ret = p_dev->ctrl_handle(p_handle, HTD_SPIM_CMD_OPEN, NULL, 0);
    if (ret != 0) {
        htd_free(p_handle);
        return NULL;
    }
    return p_handle;
}

// @brief 关闭SPI, cs引脚默认拉高(外部设备如果掉电，需要将cs引脚配置为拉低); SCK和MOSI配置为拉低，MISO配置为模拟输入
int32_t htd_spim_close(spim_handle_t* p_handle)
{
    if (p_handle) {
        const htd_dev_t* p_dev = p_handle->p_dev;
        if (p_dev && p_dev->ctrl_handle) {
            if (ERROR_CODE_SUCCESS == p_dev->ctrl_handle(p_handle, HTD_SPIM_CMD_CLOSE, NULL, 0)) {
                // 去初始化引脚(保持引脚的低电平)
                if (PIN_NUM_INVALID != p_handle->p_user_config->pin_miso) {
                    htd_pin_input_analog_set(p_handle->p_user_config->pin_miso);
                }
                if (PIN_NUM_INVALID != p_handle->p_user_config->pin_sck) {
                    htd_pin_output_pp_set(p_handle->p_user_config->pin_sck);
                    htd_pin_write(p_handle->p_user_config->pin_sck, 0);
                }
                if (PIN_NUM_INVALID != p_handle->p_user_config->pin_mosi) {
                    htd_pin_output_pp_set(p_handle->p_user_config->pin_mosi);
                    htd_pin_write(p_handle->p_user_config->pin_mosi, 0);
                }
            }
        }
        htd_free(p_handle);
    }
    return 0;
}

// @brief 多个写读数据操作
int32_t htd_spim_xfer(spim_handle_t* p_handle, spim_msg_t* p_msg, uint32_t msg_count)
{
    ASSERT(p_handle);
    ASSERT(p_handle->p_dev);

    int32_t ret = 0;
    if (p_handle->p_dev->ctrl_handle) {
        ret = p_handle->p_dev->ctrl_handle(p_handle, HTD_SPIM_CMD_XFER, (void*)p_msg, msg_count);
    }
    return ret;
}

// @brief 通信交互(收发都是阻塞操作)
int32_t htd_spim_transfer(spim_handle_t* p_handle, const uint8_t* tx, uint16_t tx_len, uint8_t* rx, uint16_t rx_len)
{
    int32_t ret_len;
    uint32_t msg_count = 0;
    spim_msg_t msg[2];
    if (tx && tx_len) {
        msg[msg_count].flags = SPIM_MSG_FLAG_TX|SPIM_MSG_FLAG_BLOCK;
        msg[msg_count].buff = (uint8_t*)tx;
        msg[msg_count].len = tx_len;
        msg_count++;
        ret_len = tx_len;
    }
    if (rx && rx_len) {
        msg[msg_count].flags = SPIM_MSG_FLAG_RX|SPIM_MSG_FLAG_BLOCK;
        msg[msg_count].buff = rx;
        msg[msg_count].len = rx_len;
        msg_count++;
        ret_len = rx_len;
    }
    if (msg_count == 0) {
        return 0;
    }
    if (msg_count == htd_spim_xfer(p_handle, msg, msg_count)) {
        return ret_len;
    }
    return 0;
}


