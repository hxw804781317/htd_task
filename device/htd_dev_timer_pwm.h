#ifndef _HTD_DEV_TIMER_PWM_H_
#define _HTD_DEV_TIMER_PWM_H_

#include "htd_common.h"
#include "htd_device.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


// 状态
#define HTD_PWM_STATE_NONE              0
#define HTD_PWM_STATE_START             1
#define HTD_PWM_STATE_STOP              2
typedef uint8_t htd_pwm_state_t;

// 模式, HL(先高电平再低电平为一个周期)；LH(先低电平再高电平为一个周期)
#define HTD_PWM_MODE_HL                 0       // 占空比为H
#define HTD_PWM_MODE_LH                 1       // 占空比为L
typedef uint8_t htd_pwm_mode_t;

// 占空比配置参数
typedef struct {
    uint8_t ch_idx;
    uint16_t duty;
}htd_pwm_duty_param_t;

// 占空比数组配置参数
typedef struct {
    uint8_t ch_idx;
    uint8_t duty_bytes;                         // p_duty_array 的位长，1、2、4
    uint16_t duty_array_size;
    uint8_t *p_duty_array;
    uint32_t loop_count;                        // 0xffffffff 代表执行无数遍
}htd_pwm_duty_array_param_t;

// 命令
#define HTD_PWM_CMD_NONE                0
#define HTD_PWM_CMD_OPEN                1
#define HTD_PWM_CMD_CLOSE               2
#define HTD_PWM_CMD_START               3
#define HTD_PWM_CMD_STOP                4
#define HTD_PWM_CMD_DUTY_SET            5       // 设置占空比，@htd_pwm_duty_param_t
#define HTD_PWM_CMD_DUTY_ARRAY_SET      6       // 设置占空比数组，@htd_pwm_duty_array_param_t
#define HTD_PWM_CMD_PERIOD_SET          7       // 设置周期
#define HTD_PWM_CMD_ACTIVE_CHECK        8       // 查看是否正在运行
typedef uint32_t htd_pwm_cmd_t;

// 控制句柄
typedef struct {
    const htd_dev_t* p_dev;
    htd_pwm_mode_t mode;
    uint8_t pwm_channel;
    pin_num_t pin_num[4]; 
    uint32_t freq_hz;
    uint16_t prescaler;
    uint16_t period;
}htd_pwm_handle_t;
#define HTD_PWM_DEV_IDENTIFY            (HTD_DEV_MAGIC_MASK | 0xbcdf)

// 用户配置参数
typedef struct {
    pin_num_t pin_num[4];               // PWM输出引脚，无效填 PIN_NUM_INVALID
    pin_num_t pin_auto;                 // 引脚自适配通道
    htd_pwm_mode_t mode;
    uint16_t prescaler;                 // 设置分频系数
} htd_pwm_user_config_t;



// @brief 打开 PWM
htd_pwm_handle_t* htd_dev_pwm_open(const char* p_name, const htd_pwm_user_config_t *p_user_config);
// @brief 关闭 PWM
int32_t htd_dev_pwm_close(htd_pwm_handle_t* p_handle);


// @brief 获取频率，一秒多少个tick
uint32_t htd_dev_pwm_freq_hz_get(htd_pwm_handle_t* p_handle);
// @brief 设置周期，只能配置一次，且所有通道都未启动前
int32_t htd_dev_pwm_period_set(htd_pwm_handle_t* p_handle, uint16_t period);
// @brief 设置占空比和周期，单位为tick，不能超过一个周期(设置占空比前必须先设置周期)
// @param ch_idx通道序号，0xff 代表所有通道都启动, 当 open 配置了自适配引脚 pin_auto 这个参数无效
// @note  配置之后需要调用 htd_dev_pwm_start 启动，调用该函数时，如果设备正在运行，会自动停止
int32_t htd_dev_pwm_duty_set(htd_pwm_handle_t* p_handle, uint8_t ch_idx, uint16_t duty);


// @brief 设置占空比数组, 配置之后需要调用 htd_dev_pwm_start 启动，调用该函数时，如果设备正在运行，会自动停止
// @param ch_idx 通道号，当 open 配置了自适配引脚 pin_auto 这个参数无效
// @param p_duty_array 占空比数组，单位为 tick, 
// @param duty_array_count 占空比数组长度
// @param loop_count 循环次数，以数组内容执行N次
int32_t htd_dev_pwm_duty16_array_set(htd_pwm_handle_t* p_handle, uint8_t ch_idx, uint16_t* p_duty_array, uint32_t duty_array_count, uint32_t loop_count);
// @brief 设置占空比数组, 配置之后需要调用 htd_dev_pwm_start 启动，调用该函数时，如果设备正在运行，会自动停止
// @param ch_idx 通道号，当 open 配置了自适配引脚 pin_auto 这个参数无效
// @param p_duty_array 占空比数组，单位为 tick, 
// @param duty_array_count 占空比数组长度
// @param loop_count 循环次数，以数组内容执行N次
int32_t htd_dev_pwm_duty8_array_set(htd_pwm_handle_t* p_handle, uint8_t ch_idx, uint8_t* p_duty_array, uint32_t duty_array_count, uint32_t loop_count);


// @brief 启动某个引脚的 PWM
// @param ch_idx_bit 通道序号bit位，0xff 代表所有通道都启动, 当 open 配置了自适配引脚 pin_auto 这个参数无效
int32_t htd_dev_pwm_start(htd_pwm_handle_t* p_handle, uint8_t ch_idx_bit);
// @brief 停止某个引脚的 PWM
// @param ch_idx_bit 通道序号bit位，0xff 代表所有通道都启动, 当 open 配置了自适配引脚 pin_auto 这个参数无效
int32_t htd_dev_pwm_stop(htd_pwm_handle_t* p_handle, uint8_t ch_idx_bit);
// @brief 判断 PWM 是否在运行
// @param ch_idx通道序号，0xff 代表所有通道都启动, 当 open 配置了自适配引脚 pin_auto 这个参数无效
int32_t htd_dev_pwm_is_active(htd_pwm_handle_t* p_handle, uint8_t ch_idx);




#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_DEV_TIMER_PWM_H_*/

