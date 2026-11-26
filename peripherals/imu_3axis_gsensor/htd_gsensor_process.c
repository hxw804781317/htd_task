// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL       1       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG       "gsensor"
#include "htd_common.h"
#include "htd_gsensor_dev.h"
#include "htd_gsensor_process.h"
#include "htd_components.h"


//////////////////////////////////////////////////////////////////////
static gsensor_indicate_handle_t m_indicate_handle_table[2];
static uint8_t m_gsensor_init_flag = 0;
static uint32_t m_gsensor_sample_interval_ms = 0;


// @brief 初始化
int32_t htd_gsensor_init(void)
{
    if (m_gsensor_init_flag != 0) {
        return 0;
    }

    memset(m_indicate_handle_table, 0, sizeof(m_indicate_handle_table));
    int32_t ret = htd_gsensor_dev_init();
    // 设置采样间隔
    htd_gsensor_sample_interval_set(GSENSOR_READ_DATA_PERIOD);
    m_gsensor_init_flag = 0x88;
    return ret;
}

// @brief 去初始化
void htd_gsensor_uninit(bool is_power_off)
{
    if (m_gsensor_init_flag == 0) {
        return;
    }
    htd_gsensor_dev_uninit(is_power_off);
}

// @brief 注册gsensor数据通知句柄
void htd_gsensor_indicate_handle_register(gsensor_indicate_handle_t handle)
{
    ASSERT(m_gsensor_init_flag != 0);
    ASSERT(handle);
    uint32_t i;
    for (i=0; i<ARRAY_COUNT(m_indicate_handle_table); ++i) {
        if (m_indicate_handle_table[i] == handle) {
            return;
        }
    }
    for (i=0; i<ARRAY_COUNT(m_indicate_handle_table); ++i) {
        if (m_indicate_handle_table[i] == NULL) {
            m_indicate_handle_table[i] = handle;
            return;
        }
    }
    // 失败，没有内存
    print_error("gsensor indicate handle register failed.");
    ASSERT(0);
}

// @brief 注销gsensor数据通知句柄
void htd_gsensor_indicate_handle_unregister(gsensor_indicate_handle_t handle)
{
    ASSERT(handle);
    for (uint32_t i=0; i<ARRAY_COUNT(m_indicate_handle_table); ++i) {
        if (m_indicate_handle_table[i] == handle) {
            m_indicate_handle_table[i] = NULL;
        }
    }
}

// @brief 获取gsensor 采样频率
uint32_t htd_gsensor_sample_freq_get(void)
{
    return HTD_GSENSOR_SAMPLE_FREQ;
}

// @brief 定时读取 gsensor 数据, 并通告给应用层
static void htd_gsensor_timer_read_handle(void* arg)
{
    uint32_t param = (uint32_t)arg;
    if (param != 0x32220987) {
        htd_task_load_timer_function(true, m_gsensor_sample_interval_ms, htd_gsensor_timer_read_handle, (void*)0x32220987);
        return;
    }

    gsensor_val_t data[32];
    uint8_t data_count = ARRAY_COUNT(data);
    htd_gsensor_dev_fifo_data_get(data, &data_count);
    for (uint32_t i=0; i<ARRAY_COUNT(m_indicate_handle_table); ++i) {
        if (m_indicate_handle_table[i]) {
            m_indicate_handle_table[i](data, data_count);
        }
    }

#if 0
    gsensor_val_t* p_data = data;
    while (data_count--) {
        print_debug("xyz data: %d, %d, %d", p_data->ax, p_data->ay, p_data->az);
        p_data++;
    }
#endif
}

// @brief 设置采样间隔
uint32_t htd_gsensor_sample_interval_set(uint32_t time_ms)
{
    if (time_ms == 0) {
        time_ms = GSENSOR_READ_DATA_PERIOD;
    }
    if (time_ms == GSENSOR_READ_DATA_PERIOD) {
        m_gsensor_sample_interval_ms = time_ms;
        return 0;
    }
    m_gsensor_sample_interval_ms = time_ms;
    htd_task_load_function(htd_gsensor_timer_read_handle, NULL);
    return 0;
}

