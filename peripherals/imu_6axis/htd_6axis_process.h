#ifndef _HTD_6AXIS_PROCESS_H_
#define _HTD_6AXIS_PROCESS_H_

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include "htd_6axis_dev.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


// 配置读取6轴传感器周期
#define HTD_6AXIS_READ_DATA_PERIOD            500


typedef void (*htd_6axis_indicate_handle_t)(htd_6axis_val_t* p_data, uint32_t data_count);
// @brief 注册6轴传感器数据通知句柄
void htd_6axis_indicate_handle_register(htd_6axis_indicate_handle_t handle);
// @brief 注销6轴传感器数据通知句柄
void htd_6axis_indicate_handle_unregister(htd_6axis_indicate_handle_t handle);


// @brief 初始化
int32_t htd_6axis_init(void);
// @brief 去初始化
void htd_6axis_uninit(bool is_power_off);

// @brief 获取6轴传感器采样频率
uint32_t htd_6axis_sample_freq_get(void);
// @brief 设置采样间隔
uint32_t htd_6axis_sample_interval_set(uint32_t time_ms);


#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_6AXIS_PROCESS_H_*/

