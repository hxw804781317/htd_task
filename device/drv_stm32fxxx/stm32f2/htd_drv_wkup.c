#define DEBUG_LVL   4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG   "drv.wkup"
#include "htd_drv_common.h"
#include "htd_device.h"
#include "htd_drv_wkup.h"
#include "htd_drv_rtc.h"


//////////////////////////////////////////////////////////////////////
// RTC 唤醒（精准），RTC计时（不精准）
static RTC_HandleTypeDef *m_rtc_ref = NULL;


// @brief 休眠毫秒转ticks数
uint32_t htd_wkup_timer_ms2ticks(uint32_t time_ms)
{
    return time_ms;
}

// @brief 唤醒ticks数转毫秒 
uint32_t htd_wkup_timer_ticks2ms(uint32_t ticks)
{
    return ticks;
}

// @brief 设置多少个毫秒后唤醒系统, 休眠前调用
uint32_t htd_wkup_timer_ms_set(uint32_t time_ms)
{
    uint32_t now_seconds = htd_rtc_timestamps_get();
    now_seconds %= (24*3600);

    // 配置唤醒
    uint32_t ticks = time_ms * drv_rtc_ss_freq_get() / 1000;
    if (ticks > 0xffff) {
        ticks = 0xffff;
    }
    __HAL_RTC_WAKEUPTIMER_CLEAR_FLAG(m_rtc_ref, RTC_FLAG_WUTF);
    if (HAL_RTCEx_SetWakeUpTimer_IT(m_rtc_ref, ticks, RTC_WAKEUPCLOCK_RTCCLK_DIV16) != HAL_OK) {
        ASSERT(0);
        return 0;
    }
    print_debug("wkup timer: time_ms=%u, ticks=%u", time_ms, ticks);
    return now_seconds * 1000;
}

// @brief 获取休眠了多少ticks数，唤醒后调用
uint32_t htd_wkup_timer_sleep_ticks_get(uint32_t prev_ticks)
{
    // 获取配置的唤醒时长
    uint32_t wakeup_counte = HAL_RTCEx_GetWakeUpTimer(m_rtc_ref);
    // 禁用唤醒
    HAL_RTCEx_DeactivateWakeUpTimer(m_rtc_ref);
    // 唤醒时长如果少于1秒，采用唤醒时长作为此次休眠的时长，这里有可能不太准
    uint32_t sleep_time_ms = wakeup_counte * 1000 / drv_rtc_ss_freq_get();
    if (sleep_time_ms < 1000) {
        return sleep_time_ms;
    }
    
    // 计算耗时
    uint32_t now_seconds = htd_rtc_timestamps_get();
    now_seconds %= (24*3600);

    uint32_t ticks;
    prev_ticks /= 1000;
    if (now_seconds < prev_ticks) {
        ticks = 3600 * 2400 + now_seconds - prev_ticks;
    } else {
        ticks = now_seconds - prev_ticks;
    }
    ticks *= 1000;
    if (sleep_time_ms < ticks) {
        ticks = sleep_time_ms;
    }
    return ticks;
}

// @brief 唤醒中断函数回调
void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc)
{
    
}

// @brief 唤醒中断函数
void RTC_WKUP_IRQHandler(void) 
{
    HAL_RTCEx_WakeUpTimerIRQHandler(m_rtc_ref);
}

// @brief 初始化唤醒定时器
int32_t htd_wkup_timer_init(void)
{
    m_rtc_ref = drv_rtc_handle_get();
    // 配置唤醒
    __HAL_RTC_WAKEUPTIMER_CLEAR_FLAG(m_rtc_ref, RTC_FLAG_WUTF);
    HAL_RTCEx_DeactivateWakeUpTimer(m_rtc_ref);
    // 配置唤醒中断优先级
    HAL_NVIC_SetPriority(RTC_WKUP_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(RTC_WKUP_IRQn);
    return 0;
}


