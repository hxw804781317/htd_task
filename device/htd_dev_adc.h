// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_DEV_ADC_H_
#define _HTD_DEV_ADC_H_

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
#define ADC_CH_00                               0
#define ADC_CH_01                               1
#define ADC_CH_02                               2
#define ADC_CH_03                               3
#define ADC_CH_04                               4
#define ADC_CH_05                               5
#define ADC_CH_06                               6
#define ADC_CH_07                               7
#define ADC_CH_08                               8
#define ADC_CH_09                               9
#define ADC_CH_10                               10 
#define ADC_CH_11                               11
#define ADC_CH_12                               12
#define ADC_CH_13                               13
#define ADC_CH_14                               14
#define ADC_CH_15                               15
#define ADC_CH_16                               16
#define ADC_CH_17                               17
#define ADC_CH_18                               18
#define ADC_CH_19                               19
#define ADC_CH_20                               20
#define ADC_CH_INTER_VREF                       21      // 内部参考电压
#define ADC_CH_INTER_TEMPER                     22      // 内部温度传感器参考电压
#define ADC_CH_INTER_VBAT                       23      // 内部VBAT参考电压
#define ADC_CH_INTER_DAC_IN                     24      // 内部 DAC 输入，用于监测DAC输出电压是否精准
#define ADC_CH_NONE                             0xff
typedef uint8_t adc_channel_t;

// 采样率, 控制ADC精度
#define HTD_ADC_RESOLUTION_6BITS                6
#define HTD_ADC_RESOLUTION_8BITS                8
#define HTD_ADC_RESOLUTION_10BITS               10
#define HTD_ADC_RESOLUTION_12BITS               12
#define HTD_ADC_RESOLUTION_14BITS               14
#define HTD_ADC_RESOLUTION_16BITS               16
typedef uint8_t htd_adc_resolution_t;

// 采样周期：外部引脚和电容阻抗有关系，阻抗越大需要的时间越久, 时间不够采集到的数据就不准
//           阻抗<1kΩ 建议 3.5~7.5 个周期
//           阻抗1kΩ ~ 10kΩ，建议 13.5~28.5 个周期
//           阻抗>10kΩ, 建议134周期
// 电量检测阻抗和分压电阻关联，分压电阻越大阻抗越大，比如并联的两个电阻为1MΩ和3MΩ，那么阻抗为 (1MΩ*3MΩ)/(1MΩ+3MΩ)=750KΩ
// 这里的周期和 ADC 时钟又有关系，时钟越快周期越短，对应的周期就要配置得多点，
// 如果这里配置到最大，Vref出值还不稳定或者错误，要看看时钟配置是否太快了
#define HTD_ADC_OVERSAMPLE_2X                   2
#define HTD_ADC_OVERSAMPLE_4X                   4
#define HTD_ADC_OVERSAMPLE_8X                   8
#define HTD_ADC_OVERSAMPLE_16X                  16
#define HTD_ADC_OVERSAMPLE_32X                  32
#define HTD_ADC_OVERSAMPLE_64X                  64
#define HTD_ADC_OVERSAMPLE_128X                 128
#define HTD_ADC_OVERSAMPLE_192X                 192
#define HTD_ADC_OVERSAMPLE_256X                 256
#define HTD_ADC_OVERSAMPLE_320X                 320
typedef uint16_t htd_adc_oversample_t;


// ADC模拟看门狗要求ADC一直在后台运行，耗电量较高，
// 同时ADC挂在系统时钟上，系统时钟停止之后，ADC也停止，也起不到监测作用
typedef struct {
    adc_channel_t channel;                      // 配置内部通道时起作用，ADC_CH_INTER_VREF/ADC_CH_INTER_TEMPER/ADC_CH_INTER_VBAT
    pin_num_t pin_num;                          // 配置外部通道时起作用
    htd_adc_resolution_t resolution;            // 采样深度；部分芯片在menuconfig时已经固定了
    htd_adc_oversample_t oversample;            // 采样周期
    bool vref_onoff;                            // 是否使能内部Vref
}htd_adc_user_config_t;

typedef struct {
    uint16_t up_adc;                            // 可以通过 HTD_ADC_CMD_MV2ADC 获取对应电压的adc值
    uint16_t low_adc;                           // 
    void (*awd_notify_handle)(void);            // 模拟看门狗中断通告句柄, NULL表示不开启
} htd_adc_awd_param_t;

// 
typedef struct {
    uint16_t adc_val;
    uint16_t voltage_mv;
    htd_adc_resolution_t resolution;
} htd_adc_voltage_t;

// ADC设备命令
#define HTD_ADC_CMD_NONE                        0
#define HTD_ADC_CMD_OPEN                        1       // @htd_adc_user_config_t
#define HTD_ADC_CMD_CLOSE                       2
#define HTD_ADC_CMD_ENABLE                      3
#define HTD_ADC_CMD_DISABLE                     4
#define HTD_ADC_CMD_AWD_ENABLE                  5       // 配置监测看门狗，@htd_adc_awd_param_t
#define HTD_ADC_CMD_AWD_DISABLE                 6
#define HTD_ADC_CMD_VALTAGE_GET                 7       // 获取ADC值，@htd_adc_voltage_t
#define HTD_ADC_CMD_FAST_VAL_GET                8       // 用于快速获取ADC值
#define HTD_ADC_CMD_MV2ADC                      9       // 毫伏转ADC值，@uint16_t mv
typedef uint32_t htd_adc_cmd_t;

// ADC 控制器
typedef struct adc_handle_s {
    const htd_dev_t* p_dev;
    uint32_t channel;                                   // 驱动对应的通道值
    uint32_t oversample;
    htd_adc_resolution_t resolution;
    uint8_t vref_onoff;
    void (*awd_notify_handle)(void);
}htd_adc_handle_t;
#define HTD_ADC_DEV_IDENTIFY                    (HTD_DEV_MAGIC_MASK | 0xab32)

// @brief 打开
htd_adc_handle_t* htd_adc_open(const char* p_name, const htd_adc_user_config_t* p_user_config);
// @brief 关闭
int32_t htd_adc_close(htd_adc_handle_t* p_handle);


// @brief 使能/禁用ADC
int32_t htd_adc_enable_set(htd_adc_handle_t* p_handle, bool is_enable);
// @brief 使能/禁用模拟看门狗功能
int32_t htd_adc_awd_enable_set(htd_adc_handle_t* p_handle, bool is_enable, htd_adc_awd_param_t* p_param);


// @brief 快速获取ADC值
// @note  调用该接口前需要先使能ADC
int32_t htd_adc_value_fast_get(htd_adc_handle_t* p_handle, uint16_t* p_adc_val, uint32_t count);
// @brief 获取ADC和对应的电压值
// @note  调用该接口前需要先使能ADC
int32_t htd_adc_voltage_get(htd_adc_handle_t* p_handle, htd_adc_voltage_t* p_val, uint32_t count);


#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_DEV_ADC_H_*/

