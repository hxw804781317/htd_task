#ifndef _HTD_DRV_RTC_H_
#define _HTD_DRV_RTC_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif


#define RTC_INIT_FLAG               0xA7BD

// 该值越大，功耗越低, 反之功耗越高，取值范围(0\1\3\7\15\31)；但功耗相差不大
#define RTC_ASYN_PRE_DIV            15

// 是否支持备份区功能，部分芯片HAL有定义
#if defined(SOC_SERIES_STM32F7) || defined(SOC_SERIES_STM32F4) || defined(SOC_SERIES_STM32F1)
    #define RTC_BACKUP_SUPPORT      1
#endif

#if defined(HAL_RTC_MODULE_ENABLED)
// @brief 获取句柄
RTC_HandleTypeDef* drv_rtc_handle_get(void);
#endif

// @brief 获取亚秒频率
uint32_t drv_rtc_ss_freq_get(void);
// @brief 获取当前亚秒
uint32_t drv_rtc_ss_ticks_get(void);
// @brief 使能RTC闹钟
void drv_rtc_alarm_enable_set(bool is_enable, uint32_t tick_seconds);
// @brief 使能RTC唤醒
int32_t drv_rtc_wkup_enable_set(bool is_enable, uint32_t ticks);


#ifdef __cplusplus
}
#endif
#endif /*_HTD_DRV_RTC_H_*/

