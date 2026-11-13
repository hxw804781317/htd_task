// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL   4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG   "drv.wkup"
#include "htd_device.h"
#include "htd_drv_common.h"
#include "htd_drv_wkup.h"
#include "htd_drv_rtc.h"


//////////////////////////////////////////////////////////////////////
// 使用RTC闹钟亚秒唤醒, 亚秒计时, 精准
static RTC_HandleTypeDef *m_rtc_ref = NULL;
static uint32_t m_alarm_int_count = 0;


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
    // 计算目标秒数
    uint32_t ss_freq = drv_rtc_ss_freq_get();
    uint32_t now_ticks = drv_rtc_ss_ticks_get();
    uint32_t dst_ticks = now_ticks + htd_wkup_timer_ms2ticks(time_ms);
    uint32_t seconds = (dst_ticks / ss_freq) % (3600 * 2400);
    
    // 闹钟时间
    RTC_AlarmTypeDef sAlarm;
    htd_memset(&sAlarm, 0, sizeof(RTC_AlarmTypeDef));
    sAlarm.AlarmTime.Hours = seconds / 3600;
    sAlarm.AlarmTime.Minutes = (seconds % 3600) / 60;
    sAlarm.AlarmTime.Seconds = seconds % 60;
    sAlarm.Alarm = RTC_ALARM_A;
    sAlarm.AlarmTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    sAlarm.AlarmTime.StoreOperation = RTC_STOREOPERATION_RESET;
    sAlarm.AlarmMask = RTC_ALARMMASK_DATEWEEKDAY;               // 忽略日期/星期
    sAlarm.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;
    sAlarm.AlarmDateWeekDay = 1;                                // 当AlarmMask忽略此字段时，值无效
    // 配置亚秒
    sAlarm.AlarmTime.SubSeconds = ss_freq - (dst_ticks % ss_freq);
    sAlarm.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_NONE;
    // 关闭原有闹钟，重新配置并开启闹钟中断
    if (HAL_RTC_SetAlarm_IT(m_rtc_ref, &sAlarm, RTC_FORMAT_BIN) != HAL_OK) {
        Error_Handler();
    }
    return now_ticks;
}

// @brief 获取休眠了多少ticks数，唤醒后调用
uint32_t htd_wkup_timer_sleep_ticks_get(uint32_t prev_ticks)
{
    uint32_t ss_freq = drv_rtc_ss_freq_get();
    uint32_t now_ticks = drv_rtc_ss_ticks_get(); 

    // 计算耗时
    uint32_t diff_ticks, max_ticks;
    if (prev_ticks <= now_ticks) {
        diff_ticks = now_ticks - prev_ticks;
    } else {
        max_ticks = 24 * 3600 * ss_freq;
        diff_ticks = max_ticks - prev_ticks + now_ticks;
    }
    print_debug("ticks=%u->%u, diff_ticks=%u, m_alarm_int_count=%u", 
                 prev_ticks, now_ticks, diff_ticks, m_alarm_int_count);
    return diff_ticks;
}

// @brief RTC 闹钟中断
void RTC_Alarm_IRQHandler(void)
{
    m_alarm_int_count++;
    if (LL_RTC_IsActiveFlag_ALRA(RTC)) {
        LL_RTC_ClearFlag_ALRA(RTC);
        drv_rtc_alarm_enable_set(false, 0);
    }
    LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_17);
}

// @brief RTC 中断
void RTC_IRQHandler(void)
{
    RTC_Alarm_IRQHandler();
}

// @brief 初始化唤醒定时器
int32_t htd_wkup_timer_init(void)
{
    m_rtc_ref = drv_rtc_handle_get();
    HAL_RTC_DeactivateAlarm(m_rtc_ref, RTC_ALARM_A);
    
    HAL_NVIC_SetPriority(RTC_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(RTC_IRQn);
    return 0;
}

