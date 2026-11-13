// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
//#define DEBUG_LVL   4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG   "drv.wkup"
#include "htd_device.h"
#include "htd_drv_common.h"
#include "htd_drv_wkup.h"
#include "htd_drv_rtc.h"


#if defined(HAL_RTC_MODULE_ENABLED)
//////////////////////////////////////////////////////////////////////
// 使用RTC唤醒, 亚秒计时, 精准
static RTC_HandleTypeDef *m_rtc_ref = NULL;
static uint32_t m_int_count = 0;


// @brief 获取亚秒ticks数
static uint32_t htd_wkup_timer_ticks_get(void)
{
    RTC_TimeTypeDef RTC_TimeStruct = {0};
    HAL_RTC_GetTime(m_rtc_ref, &RTC_TimeStruct, RTC_FORMAT_BIN);
    RTC_DateTypeDef RTC_DateStruct = {0};
    HAL_RTC_GetDate(m_rtc_ref, &RTC_DateStruct, RTC_FORMAT_BIN);
    
    uint32_t seconds = RTC_TimeStruct.Hours * 3600 + RTC_TimeStruct.Minutes * 60 + RTC_TimeStruct.Seconds;
    uint32_t ticks = seconds * drv_rtc_ss_freq_get();
    ticks += drv_rtc_ss_freq_get() - RTC_TimeStruct.SubSeconds;
    //print_debug("seconds=%u, ticks=%u, SubSeconds=%u", seconds, ticks, RTC_TimeStruct.SubSeconds);
    return ticks;
}

// @brief 休眠毫秒转ticks数
uint32_t htd_wkup_timer_ms2ticks(uint32_t time_ms)
{
    return time_ms * drv_rtc_ss_freq_get() / 1000;
}

// @brief 唤醒ticks数转毫秒 
uint32_t htd_wkup_timer_ticks2ms(uint32_t ticks)
{
    return 1000 * ticks / drv_rtc_ss_freq_get();
}

// @brief 设置多少个毫秒后唤醒系统, 休眠前调用
uint32_t htd_wkup_timer_ms_set(uint32_t time_ms)
{
    if (time_ms == 0) {
        return 0;
    }
    
    // 记录当前亚秒时间点
    uint32_t now_ticks = htd_wkup_timer_ticks_get();
    // 配置唤醒
    uint32_t ticks = time_ms * drv_rtc_ss_freq_get() / 1000;
    if (ticks > 0xffff) {
        ticks = 0xffff;
    }
    // 多少个 TICK 后唤醒系统
    __HAL_RTC_WAKEUPTIMER_CLEAR_FLAG(m_rtc_ref, RTC_FLAG_WUTF);
    if (HAL_RTCEx_SetWakeUpTimer_IT(m_rtc_ref, ticks, RTC_WAKEUPCLOCK_RTCCLK_DIV16) != HAL_OK) {
        ASSERT(0);
        return 0;
    }
    print_debug("wkup timer: time_ms=%u, ticks=%u", time_ms, ticks);
    return now_ticks;
}

// @brief 获取休眠了多少ticks数，唤醒后调用
uint32_t htd_wkup_timer_sleep_ticks_get(uint32_t prev_ticks)
{
    // 禁用唤醒
    HAL_RTCEx_DeactivateWakeUpTimer(m_rtc_ref);
    
    // 计算耗时
    uint32_t now_ticks = htd_wkup_timer_ticks_get(); 
    
    uint32_t diff_ticks, max_ticks;
    if (prev_ticks <= now_ticks) {
        diff_ticks = now_ticks - prev_ticks;
    } else {
        max_ticks = 24 * 3600 * drv_rtc_ss_freq_get();
        diff_ticks = max_ticks - prev_ticks + now_ticks;
    }
    print_debug("m_int_count=%d. ticks=%u->%u, diff_ticks=%u", m_int_count, prev_ticks, now_ticks, diff_ticks);
    return diff_ticks;
}

// @brief 唤醒中断函数回调
void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc)
{
    
}

// @brief 唤醒中断函数
void RTC_WKUP_IRQHandler(void) 
{
    m_int_count++;
    HAL_RTCEx_WakeUpTimerIRQHandler(m_rtc_ref);
}

// @brief RTC 中断
void RTC_IRQHandler(void)
{
    RTC_WKUP_IRQHandler();
}

// @brief 初始化唤醒定时器
int32_t htd_wkup_timer_init(void)
{
    m_rtc_ref = drv_rtc_handle_get();
    // 配置唤醒
    __HAL_RTC_WAKEUPTIMER_CLEAR_FLAG(m_rtc_ref, RTC_FLAG_WUTF);
    HAL_RTCEx_DeactivateWakeUpTimer(m_rtc_ref);
    
    HAL_NVIC_SetPriority(RTC_WKUP_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(RTC_WKUP_IRQn);
    return 0;
}

#endif /*defined(HAL_RTC_MODULE_ENABLED)*/

