#define DEBUG_LVL   4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG   "drv.wkup"
#include "htd_drv_common.h"
#include "htd_device.h"
#include "htd_drv_wkup.h"
#include "htd_drv_rtc.h"


//////////////////////////////////////////////////////////////////////
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
    uint32_t now_seconds = htd_rtc_timestamps_get();
    uint32_t now_ticks = drv_rtc_ss_ticks_get();
    uint32_t dst_ticks = now_ticks + htd_wkup_timer_ms2ticks(time_ms);
    uint32_t add_seconds = dst_ticks / ss_freq;
    if (add_seconds == 0) {
        print_warning("now_ticks=%u, dst_ticks=%u", now_ticks, dst_ticks);
        return 0;
    }

    // 提前唤醒
    add_seconds--;
    if (add_seconds == 0 && now_ticks > ss_freq - 100) {
        print_warning("add_seconds=%d, now_ticks=%d", add_seconds, now_ticks);
        // 这里有一定的风险，如果有中断执行时间太长超过100ticks，这里有可能导致配置的闹钟唤醒已经过了
        return 0;
    }
    
    // 闹钟时间
    uint32_t dst_seconds = now_seconds + add_seconds;
    drv_rtc_alarm_enable_set(true, dst_seconds);

    print_debug("now_seconds=%u, add_seconds=%u, ticks=%u", now_seconds, add_seconds, now_ticks);
    now_ticks = (now_seconds % (24*3600)) * ss_freq + now_ticks;
    return now_ticks;
}

// @brief 获取休眠了多少 ticks 数，唤醒后调用
uint32_t htd_wkup_timer_sleep_ticks_get(uint32_t prev_ticks)
{
    uint32_t ss_freq = drv_rtc_ss_freq_get();
    uint32_t ticks = drv_rtc_ss_ticks_get(); 
    uint32_t now_seconds = htd_rtc_timestamps_get();
    uint32_t now_ticks = (now_seconds % (24*3600)) * ss_freq + ticks;
    
    // 计算耗时
    uint32_t diff_ticks, max_ticks;
    if (prev_ticks <= now_ticks) {
        diff_ticks = now_ticks - prev_ticks;
    } else {
        max_ticks = 24 * 3600 * ss_freq;
        diff_ticks = (max_ticks - prev_ticks) + now_ticks;
    }
    print_debug("now_seconds=%u->%u, ticks=%u->%u, diff_ticks=%u, ticks=%d, m_alarm_int_count=%u", \
                 now_seconds, htd_rtc_timestamps_get(), prev_ticks, now_ticks, diff_ticks, ticks, m_alarm_int_count);
    return diff_ticks;
}

// @brief RTC 闹钟中断
void RTC_Alarm_IRQHandler(void)
{
    m_alarm_int_count++;
    if (LL_RTC_IsActiveFlag_ALR(RTC)) {
        LL_RTC_ClearFlag_ALR(RTC);
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
    drv_rtc_alarm_enable_set(false, 0);
    return 0;
}


