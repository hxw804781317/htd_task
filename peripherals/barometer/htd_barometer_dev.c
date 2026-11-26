// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL           4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG           "barometer.dev"
#include "htd_common.h"
#include "htd_barometer_dev.h"


//////////////////////////////////////////////////////////////////////
static const barometer_dev_t* m_barometer_dev = NULL;


// @brief 函数注册
int32_t htd_barometer_dev_register(const barometer_dev_t* p_dev)
{
    ASSERT(m_barometer_dev == NULL);
    m_barometer_dev = p_dev;
    return 0;
}

// @brief 初始化气压计设备
int32_t htd_barometer_dev_init(void)
{
    ASSERT(m_barometer_dev);
    return m_barometer_dev->init();
}

// @brief 去初始化(设备进入低功耗状态)
void htd_barometer_dev_uninit(bool is_power_off)
{
    ASSERT(m_barometer_dev);
    m_barometer_dev->uninit(is_power_off);
}

// @brief 获取设备名字
const char* htd_barometer_dev_name_get(char* p_name)
{
    ASSERT(m_barometer_dev);
    return m_barometer_dev->chip_name;
}

// @brief 读取温度值和气压值
int32_t htd_barometer_dev_read(uint32_t *pressure_pa)
{
    ASSERT(m_barometer_dev);
    return m_barometer_dev->read_handle(pressure_pa);
}



