// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL           4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG           "baro.simulate"
#include "htd_barometer_dev.h"


#if defined(HTD_BAROMETER_USING_SIMULATE) && HTD_BAROMETER_USING_SIMULATE
/////////////////////////////////////////////////////////////////////////////////
static uint32_t m_pressure_vref = 1000;
// @brief simulate 初始化
int32_t drv_simulate_init(void)
{
    m_pressure_vref = htd_rng_val_get() % 10000;
    return 0;
}

// @brief si7020 去初始化
void drv_simulate_uninit(bool is_power_off)
{
    
}

// @brief 获取真实的压强值 (Pa, 帕)
int32_t drv_simulate_pressure_get(uint32_t *pressure_pa) 
{
    if (pressure_pa) {
        *pressure_pa = 101325 - m_pressure_vref;
    }
    return 0;
}

// @brief 注册设备
static int32_t drv_barometer_dev_register(void)
{
    static const barometer_dev_t m_barometer_simulate = {
        .chip_name = "BMP180",
        .init = drv_simulate_init,
        .uninit = drv_simulate_uninit,
        .read_handle = drv_simulate_pressure_get,
    };
    return htd_barometer_dev_register(&m_barometer_simulate);
}
DEVICE_INIT_FUNC_REGISTER(drv_barometer_dev_register);
#endif /*defined(HTD_BAROMETER_USING_SIMULATE) && HTD_BAROMETER_USING_SIMULATE*/

