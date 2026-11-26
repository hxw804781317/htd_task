// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_GSENSOR_DEV_H_
#define _HTD_GSENSOR_DEV_H_

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


//////////////////////////////////////////////////////////////////////
// 配置采样率
#ifndef HTD_GSENSOR_SAMPLE_FREQ
    #define HTD_GSENSOR_SAMPLE_FREQ         50
#endif 

#pragma pack(1)
// 三轴传感器，只有三组数据
typedef struct {
    int16_t ax;
    int16_t ay;
    int16_t az;
} gsensor_val_t;
#pragma pack()




//////////////////////////////////////////////////////////////////////
#define HTD_GSENSOR_CMD_NONE                0
#define HTD_GSENSOR_CMD_FIFO_DATA_READ      1       // 读取传感器缓冲数据, @gsensor_val_t
typedef uint32_t htd_gsensor_cmd_t;

typedef struct {
    const char* chip_name;
    uint16_t fifo_max_size;
    // 初始化函数，并获取设备的id
    int32_t (*init)(uint8_t* p_chip_id);
    // 去初始化函数，设备进入低功耗状态
    void (*uninit)(bool is_power_off);
    // 控制句柄
    int32_t (*ctrl_handle)(htd_gsensor_cmd_t cmd, void* p_param, uint32_t param_len);
} htd_gsensor_dev_t;


// @brief 函数注册
int32_t htd_gsensor_dev_register(const htd_gsensor_dev_t* p_ops);
// @brief 初始化
int32_t htd_gsensor_dev_init(void);
// @brief 去初始化(设备进入低功耗状态)
void htd_gsensor_dev_uninit(bool is_power_off);
// @brief 获取设备名字
const char* htd_gsensor_dev_name_get(char* p_name);
// @brief 获取缓冲最大的数据个数
uint8_t htd_gsensor_dev_fifo_max_count_get(void);
// @brief 获取gsensor缓冲数据，返回数据长度，count输入数据个数，输出返回数据个数
uint32_t htd_gsensor_dev_fifo_data_get(gsensor_val_t *p_xyz, uint8_t *count);




#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_GSENSOR_DEV_H_*/

