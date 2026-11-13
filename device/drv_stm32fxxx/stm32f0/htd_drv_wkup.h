// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_DRV_WKUP_H_
#define _HTD_DRV_WKUP_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif


// @brief 初始化唤醒定时器
int32_t htd_wkup_timer_init(void);
// @brief 休眠毫秒转ticks数
uint32_t htd_wkup_timer_ms2ticks(uint32_t time_ms);
// @brief 唤醒ticks数转毫秒 
uint32_t htd_wkup_timer_ticks2ms(uint32_t ticks);
// @brief 设置多少个毫秒后唤醒系统, 休眠前调用
uint32_t htd_wkup_timer_ms_set(uint32_t time_ms);
// @brief 获取休眠了多少ticks数，唤醒后调用
uint32_t htd_wkup_timer_sleep_ticks_get(uint32_t set_ticks);


#ifdef __cplusplus
}
#endif
#endif /*_HTD_DRV_WKUP_H_*/

