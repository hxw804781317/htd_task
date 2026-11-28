// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL       4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG       "dev.6axis"
#include "htd_device.h"
#include "htd_6axis_dev.h"



//////////////////////////////////////////////////////////////////////
static const htd_6axis_dev_t* m_dev_6axis = NULL;

// @brief 函数注册
int32_t htd_6axis_dev_register(const htd_6axis_dev_t* p_dev)
{
    ASSERT(p_dev);
    if (NULL == m_dev_6axis) {
        m_dev_6axis = p_dev;
    }
    return 0;
}

// @brief 初始化
int32_t htd_6axis_dev_init(void)
{
    if (m_dev_6axis) {
        if (m_dev_6axis->init) {
            uint8_t m_6axis_chip_id = 0xff;
            if (0 != m_dev_6axis->init(&m_6axis_chip_id)) {
                print_error("dev init failed.");
                return -1;
            }
            print_debug("dev init success. chip_id=%d", m_6axis_chip_id);
            return 0;
        }
        return -2;
    }
    return -1;
}

// @brief 去初始化(设备进入低功耗状态)
void htd_6axis_dev_uninit(bool is_power_off)
{
    print_debug("dev uninit");
    if (m_dev_6axis && m_dev_6axis->uninit) {
        m_dev_6axis->uninit(is_power_off);
    }
}

// @brief 获取设备名字
const char* htd_6axis_dev_name_get(char* p_name)
{
    if (m_dev_6axis) {
        if (p_name) {
            strcpy(p_name, m_dev_6axis->chip_name);
        }
        return m_dev_6axis->chip_name;
    }
    return NULL;
}

// @brief 获取缓冲最大的数据个数
uint8_t htd_6axis_dev_fifo_max_count_get(void)
{
    if (m_dev_6axis) {
        return m_dev_6axis->fifo_max_size;
    }
    return 0;
}

// @brief 获取6轴传感器缓冲数据，返回数据长度，count输入数据个数，输出返回数据个数
uint32_t htd_6axis_dev_fifo_data_get(htd_6axis_val_t *p_xyz, uint8_t *count)
{
    int32_t data_len = 0;
    
    ASSERT(p_xyz);
    ASSERT(count);
    if (m_dev_6axis) {
        if (m_dev_6axis->ctrl_handle) {
            data_len = (*count) * sizeof(htd_6axis_val_t);
            data_len = m_dev_6axis->ctrl_handle(HTD_6AXIS_CMD_FIFO_DATA_READ, p_xyz, data_len);
            *count = data_len / sizeof(htd_6axis_val_t);
        } 
        return data_len;
    }
    return 0;
}

