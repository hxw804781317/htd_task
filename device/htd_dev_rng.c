// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL           1       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG           "dev.rng"
#include "htd_device.h"
#include "htd_dev_rng.h"


//////////////////////////////////////////////////////////////////////
static uint32_t m_rand_seed = 1254125411;


// @brief 获取随机种子
// @return 返回0说明没有获取到对应的随机值
__WEAK uint32_t htd_dev_rng_data_get(void)
{
#ifndef _WIN32
    uint32_t uid_buff[4];
    htd_mcu_id_get((uint8_t*)uid_buff, sizeof(uid_buff));
    uint32_t randval = m_rand_seed;
    for (uint32_t i=0; i<ARRAY_COUNT(uid_buff); ++i) {
        randval = randval * 1103515245 + uid_buff[i];
    }
    return randval;
#else
    return htd_rng_val_get();
#endif
}

// @brief 获取随机值
uint32_t htd_rng_val_get(void)
{
    m_rand_seed = m_rand_seed * 1103515245 + 12345 + htd_time_ms_get();
    return (unsigned)(m_rand_seed);
}

// @brief 获取随机数据
uint32_t htd_rng_data_get(uint8_t* p_data, uint32_t data_len)
{
    uint32_t rng_val;
    uint8_t* p_start = p_data;
    uint32_t copy_len = sizeof(uint32_t);
    while (data_len) {
        rng_val = htd_rng_val_get();
        if (data_len < copy_len) {
            copy_len = data_len;
        }
        htd_memcpy(p_data, &rng_val, copy_len);
        p_data += copy_len;
        data_len -= copy_len;
    }
    return p_data - p_start;
}

// @brief 初始化
int32_t htd_rng_init(void)
{
    uint32_t rndval = htd_dev_rng_data_get();
    print_debug("m_rand_seed=0x%08x", rndval);
    return 0;
}
DEVICE_INIT_FUNC_REGISTER(htd_rng_init);


