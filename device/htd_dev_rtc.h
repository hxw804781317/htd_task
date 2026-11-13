// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef __HTD_DEV_RTC_H__
#define __HTD_DEV_RTC_H__

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
// @brief 当前时获取
uint8_t htd_rtc_hour_get(void);
// @brief 当前分获取
uint8_t htd_rtc_minute_get(void);
// @brief 当前秒获取
uint8_t htd_rtc_second_get(void);
// @brief 获取当天的年
uint16_t htd_rtc_year_get(void);
// @brief 获取当天的月
uint8_t htd_rtc_month_get(void);
// @brief 获取当天的日期
uint8_t htd_rtc_day_get(void);
// @brief 获取当天的星期
uint8_t htd_rtc_week_get(void);
// @brief 设置时区
void htd_rtc_timezone_set(int32_t seconds);
// @brief 获取时区
int32_t htd_rtc_timezone_get(void);



//////////////////////////////////////////////////////////////////////
// @brief 初始化 RTC(WEEK函数)
// @param start_timestamps 起始时间戳，如果为0xffffffff, 则为默认时间2020-1-1 0:0:0
int32_t htd_rtc_init(uint32_t start_timestamps);
// @brief 同步RTC寄存器(休眠唤醒后需要调用一次)，WEEK函数
void htd_rtc_register_sync_wait(void);
// @brief 设置时间(年月日时分秒)，WEEK函数
int32_t htd_rtc_time_set(htd_time_t* p_time);
// @brief 获取当前的时间(年月日时分秒)，WEEK函数
htd_time_t htd_rtc_time_get(void);
// @brief 获取当前的时间戳，WEEK函数
uint32_t htd_rtc_timestamps_get(void);
// @brief 备份区数据(WEEK函数)
// @param bkp_idx 备份区序号
// @return 返回获取到的数据长度
int32_t htd_rtc_bkp_data_get(uint8_t bkp_idx, uint8_t* p_data, uint32_t data_len);
// @brief 设置备份区数据(WEEK函数)
// @param bkp_idx 备份区序号
// @return 返回设置的数据长度
int32_t htd_rtc_bkp_data_set(uint8_t bkp_idx, const uint8_t* p_data, uint32_t data_len);



//////////////////////////////////////////////////////////////////////
// @brief 本地时间戳
uint32_t htd_rtc_local_timestamps_get(void);
// @brief 本地时间戳转 UTC0 时间戳
uint32_t htd_rtc_localtimestamps2utc0timestamps(uint32_t localtimestamps);
// @brief UTC0 时间戳转本地时间戳
uint32_t htd_rtc_utc0timestamps2localtimestamps(uint32_t utc0timestamps);

// @brief 获取昨天的年
uint16_t htd_rtc_yesterday_year_get(void);
// @brief 获取昨天的月
uint8_t htd_rtc_yesterday_month_get(void);
// @brief 获取昨天的日期
uint8_t htd_rtc_yesterday_day_get(void);

// @brief 显示当前rtc信息
void htd_rtc_info_show(void);


#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*__HTD_DEV_RTC_H__*/

