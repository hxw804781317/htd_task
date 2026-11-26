// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_GSENSOR_PROCESS_H_
#define _HTD_GSENSOR_PROCESS_H_

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include "htd_gsensor_dev.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


// 配置读取gsensor周期
#define GSENSOR_READ_DATA_PERIOD            500


typedef void (*gsensor_indicate_handle_t)(gsensor_val_t* p_data, uint32_t data_count);
// @brief 注册gsensor数据通知句柄
void htd_gsensor_indicate_handle_register(gsensor_indicate_handle_t handle);
// @brief 注销gsensor数据通知句柄
void htd_gsensor_indicate_handle_unregister(gsensor_indicate_handle_t handle);


// @brief 初始化
int32_t htd_gsensor_init(void);
// @brief 去初始化
void htd_gsensor_uninit(bool is_power_off);
// @brief 获取gsensor 采样频率
uint32_t htd_gsensor_sample_freq_get(void);
// @brief 设置采样间隔
uint32_t htd_gsensor_sample_interval_set(uint32_t time_ms);


#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_GSENSOR_PROCESS_H_*/

