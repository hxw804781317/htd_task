// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_DEV_PM_H_
#define _HTD_DEV_PM_H_

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


// WFI(Wait For Interrupt), 中断唤醒
// WFE(Wait For Event)，事件唤醒，一种是来自中断事件(需要特殊设置)，一种来自事件寄存器
//     WFE_INT 特指中断事件唤醒，比WFI功耗更低
//     WFE_EVT 特指事件寄存器的唤醒

// ARM 系统的几种休眠模式
// Sleep Mode(睡眠模式):
//     CPU 状态：CPU 时钟停止，进入睡眠状态，
//     外设时钟：外设时钟独立控制，可睡眠或运行
//     系统时钟：系统时钟维持原有状态
//     唤醒源：任何使能的中断都能唤醒
//     唤醒延迟：唤醒时间极短，因为系统时钟无需重新稳定
//     功耗：功耗中等
// Stop Mode(停止模式):
//     CPU 状态：CPU 完全停止，包括PLL、HRC等高频时钟
//     外设时钟：大部分外设也被强制关闭，仅 LVD、RTC、WKTM、CMP、外部中断等指定唤醒源可工作
//     系统时钟：进入 STOP 前，驱动会自动将系统时钟切换到 MRC（中速 RC 振荡器），并备份原时钟配置；唤醒后再恢复
//     唤醒源：有限（EXTINT、RTC 闹钟/周期、WKTM、LVD、CMP、SWDT 等），且需配置使能
//     唤醒延迟：较长（几十微秒到毫秒级），需要等待时钟重新稳定。
//     功耗：极低
// Power Down Mode(关机模式):
//     系统几乎完全断电，功耗极低，唤醒相当于复位重启


// CPU 使用率计算间隔时间，单位毫秒
#define CPU_USAGE_INTER_MS          3000


// 休眠前回调
typedef struct {
    uint16_t magic;
    uint8_t priority;
    const char* p_name;
    void (*before_sleep_func)(void* arg);
    void* before_sleep_func_arg;
    void (*after_sleep_func)(void* arg);
    void* after_sleep_func_arg;
} pm_sleep_cb_intance_t;
#define HTD_PM_SLEEP_MAGIC                  0x32ef
#define HTD_PM_SLEEP_SECTION_NAME           htd_pm_sleep
// 注册休眠前回调函数，_priority 优先级为字符数值大小排列
#define HTD_PM_SLEEP_CB_REGISTER(_name, _priority, _before_func, _before_func_arg, _after_func, _after_func_arg)   \
    HTD_SECTION_ITEM_REGISTER(HTD_PM_SLEEP_SECTION_NAME, static const pm_sleep_cb_intance_t CONCAT_2(htd_pm, _priority)) = {\
        .magic = HTD_PM_SLEEP_MAGIC,                \
        .p_name = STR(_name),                       \
        .priority = _priority,                      \
        .before_sleep_func = _before_func,          \
        .before_sleep_func_arg = _before_func_arg,  \
        .after_sleep_func = _after_func,            \
        .after_sleep_func_arg = _after_func_arg,    \
    };

typedef void (*system_clock_config_handle_t)(void);



// @brief 初始化
int32_t htd_pm_init(system_clock_config_handle_t handle);
// @brief 运行
uint32_t htd_pm_run(uint32_t sleep_ms);
// @brief 获取 CPU 运行百分比
uint8_t htd_pm_cpu_usage(void);
// @brief 获取 CPU 运行毫秒数
uint32_t htd_pm_cpu_time_ms_get(void);
// @brief 重启
void htd_pm_reboot(void);
// @brief 关机
void htd_pm_shutdown(void);



#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_DEV_PM_H_*/

