// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_DEV_TIMER_OCC_H_
#define _HTD_DEV_TIMER_OCC_H_

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


//////////////////////////////////////////////////////////////////////
// 状态
#define HTD_OCC_STATE_NONE              0
#define HTD_OCC_STATE_START             1
#define HTD_OCC_STATE_STOP              2
typedef uint8_t htd_occ_state_t;

// 模式, HL(先高电平再低电平为一个周期)；LH(先低电平再高电平为一个周期)
#define HTD_OCC_MODE_HL                 0
#define HTD_OCC_MODE_LH                 1
typedef uint8_t htd_occ_mode_t;

// 命令
#define HTD_OCC_CMD_NONE                0
#define HTD_OCC_CMD_OPEN                1
#define HTD_OCC_CMD_CLOSE               2
#define HTD_OCC_CMD_START               3
#define HTD_OCC_CMD_STOP                4
typedef uint32_t htd_occ_cmd_t;

// 控制句柄
typedef struct {
    const htd_dev_t* p_dev;
    htd_occ_state_t state;
    htd_occ_mode_t mode;
    pin_num_t pin_num; 
    void (*occ_stop_notify_handle)(void* handle);
    
    uint16_t *p_pulse_array;
    uint16_t pulse_array_size;
    uint16_t pulse_array_offset;
    
    uint32_t freq_hz;
    uint16_t prescaler;
    uint8_t last_pin_level;
    uint16_t irq_count;
}htd_occ_handle_t;
#define HTD_OCC_DEV_IDENTIFY            (HTD_DEV_MAGIC_MASK | 0xab3d)

// 用户配置参数
typedef struct {
    pin_num_t pin_num;                  // 脉冲输出引脚
    htd_occ_mode_t mode;
    uint16_t prescaler;                 // 设置分频系数
} htd_occ_user_config_t;



// @brief 打开 OCC(多脉冲生成器)
htd_occ_handle_t* htd_dev_occ_open(const char* p_name, const htd_occ_user_config_t *p_user_config);
// @brief 关闭 OCC(多脉冲生成器)
int32_t htd_dev_occ_close(htd_occ_handle_t* p_handle);


// @brief 获取频率
uint32_t htd_dev_occ_freq_hz_get(htd_occ_handle_t* p_handle);
// @brief 设置脉冲数组, 配置之后需要调用 htd_dev_occ_start 启动，调用该函数时，如果设备正在运行，会自动停止
// @param pluse_data_size p_pluse_array 数据位宽，单位字节
// @param p_pulse_array 脉冲数组，单位为 tick, 
// @param pluse_array_count 脉冲数组大小
int32_t htd_dev_occ_pluse_set(htd_occ_handle_t* p_handle, uint8_t pluse_data_size, uint8_t* p_pluse_array, uint32_t pluse_array_count);


// @brief 启动 OCC(多脉冲生成器)
int32_t htd_dev_occ_start(htd_occ_handle_t* p_handle);
// @brief 停止 OCC(多脉冲生成器)
int32_t htd_dev_occ_stop(htd_occ_handle_t* p_handle);
// @brief 判断 OCC(多脉冲生成器) 是否在运行
int32_t htd_dev_occ_is_active(htd_occ_handle_t* p_handle);



#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_DEV_TIMER_OCC_H_*/

