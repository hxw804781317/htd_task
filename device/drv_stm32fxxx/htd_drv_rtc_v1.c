#define DEBUG_LVL   4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG   "drv.rtc"
#include "htd_drv_common.h"
#include "htd_device.h"
#include "htd_drv_rtc.h"


#if defined(BSP_USING_RTC) && defined(RTC) && defined(SOC_SERIES_STM32F1) && 1
//////////////////////////////////////////////////////////////////////
// @brief 等待上次写操作完成
__STATIC_INLINE void stm32_rtc_lwoff_wait(void)
{
    while (LL_RTC_IsActiveFlag_RTOF(RTC) == 0) {}
}

// @brief 等待 RTC 寄存器同步（RSF 置位）
void htd_rtc_register_sync_wait(void)
{
    LL_RTC_ClearFlag_RS(RTC);
    while (LL_RTC_IsActiveFlag_RS(RTC) == 0) {}
}

// @brief 获取当前的时间戳
uint32_t htd_rtc_timestamps_get(void)
{
    return LL_RTC_TIME_Get(RTC);
}

// @brief 设置时间(年月日时分秒)
int32_t htd_rtc_time_set(htd_time_t* p_time)
{
    uint32_t timestamps = htd_time2timestamps(p_time);
    LL_RTC_TIME_SetCounter(RTC, timestamps);
    stm32_rtc_lwoff_wait();
    return 0;
}

// @brief 备份区数据; 
// @param bkp_idx 备份区序号
// @return 返回获取到的数据长度
int32_t htd_rtc_bkp_data_get(uint8_t bkp_idx, uint8_t* p_data, uint32_t data_len)
{
    if (bkp_idx + 1 < RTC_BKP_NUMBER) {
        uint16_t data = LL_RTC_BKP_GetRegister(BKP, RTC_BKP_DR1 + bkp_idx + 1);
        if (data_len > sizeof(data)) {
            data_len = sizeof(data);
        }
        htd_memcpy(p_data, &data, data_len);
        return data_len;
    } 
    return 0;
}

// @brief 设置备份区数据
// @param bkp_idx 备份区序号
// @return 返回设置的数据长度
int32_t htd_rtc_bkp_data_set(uint8_t bkp_idx, const uint8_t* p_data, uint32_t data_len)
{
    if (bkp_idx + 1 < RTC_BKP_NUMBER) {
        uint32_t data;
        if (data_len == 1) {
            data = p_data[0];
        } else {
            data = *(uint16_t*)p_data;
            data_len = sizeof(uint16_t);
        }
        LL_RTC_BKP_SetRegister(BKP, RTC_BKP_DR1 + bkp_idx + 1, data);
        return data_len;
    } 
    return 0;
}

// @brief 配置RTC时钟
static uint32_t drv_rtc_clock_config(void)
{
#if defined(BSP_CLOCK_USE_XTAL32768)
    if (LL_RCC_LSE_IsReady() != 1) {
        LL_RCC_LSE_Enable();
        while (LL_RCC_LSE_IsReady() != 1) {}
    }
    LL_RCC_SetRTCClockSource(LL_RCC_RTC_CLKSOURCE_LSE);
    LL_RCC_EnableRTC();
    return LSE_VALUE;
#else
    if (LL_RCC_LSI_IsReady() != 1) {
        LL_RCC_LSI_Enable();
        while (LL_RCC_LSI_IsReady() != 1) {}
    }
    LL_RCC_SetRTCClockSource(LL_RCC_RTC_CLKSOURCE_LSI);
    LL_RCC_EnableRTC();
    return LSI_VALUE;
#endif
}

// @brief 配置 RTC 预分频器，使秒脉冲为 1Hz
static void drv_rtc_prescaler_config(uint32_t rtc_rcc_freq)
{
    LL_RTC_DisableWriteProtection(RTC);
    stm32_rtc_lwoff_wait();
    /* 设置预分频值 = 32768 - 1 = 32767 */
    LL_RTC_SetAsynchPrescaler(RTC, rtc_rcc_freq);
    stm32_rtc_lwoff_wait();
    LL_RTC_EnableWriteProtection(RTC);
}

// @brief 初始化 RTC
int32_t htd_rtc_init(uint32_t start_timestamps)
{
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_BKP);
    LL_PWR_EnableBkUpAccess();

    // 检查是否已初始化
    uint32_t init_flag = LL_RTC_BKP_GetRegister(BKP, LL_RTC_BKP_DR1);
    if (RTC_INIT_FLAG == init_flag) {
        htd_rtc_register_sync_wait();       // 已初始化，只需等待同步即可
        return 0;
    }
    // 写备份域
    LL_RTC_BKP_SetRegister(BKP, LL_RTC_BKP_DR1, RTC_INIT_FLAG);

    /* 配置 LSE 时钟源 */
    uint32_t rtc_rcc_freq = drv_rtc_clock_config();
    // 配置预分频器
    htd_rtc_register_sync_wait();
    drv_rtc_prescaler_config(rtc_rcc_freq - 1);

    // 设置起始时间
    htd_time_t time;
    if (start_timestamps == 0xffffffff) {
        time.year = __YEAR__;
        time.month = __MONTH__;
        time.day = __DAY__;
        time.hour = __HOUR__;
        time.min = __MINUTE__;
        time.second = __SECOND__;
        start_timestamps = htd_time2timestamps(&time);
        time.wday = htd_timestamps2week(start_timestamps);
    } else {
        htd_timestamps2time(start_timestamps, &time);
    }
    print_debug("rtc init. init_flag=0x%04x", init_flag);
    htd_rtc_time_set(&time);
    return 0;
}

// @brief 获取亚秒频率
uint32_t drv_rtc_ss_freq_get(void)
{
#if defined(BSP_CLOCK_USE_XTAL32768)
    return LSE_VALUE;
#else
    return LSI_VALUE;
#endif
}

// @brief 获取当前亚秒
uint32_t drv_rtc_ss_ticks_get(void)
{
    uint32_t div_val = LL_RTC_GetDivider(RTC);
    return drv_rtc_ss_freq_get() - div_val;
}

// @brief 使能RTC闹钟
void drv_rtc_alarm_enable_set(bool is_enable, uint32_t tick_seconds)
{
    // 清除 EXTI17 挂起标志，防止重复触发
    LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_17);
    // 清除RTC闹钟标志
    LL_RTC_ClearFlag_ALR(RTC);
    // 清除 PWR 的唤醒标志
    LL_PWR_ClearFlag_WU();
    
    if (is_enable) {
        LL_EXTI_EnableIT_0_31(LL_EXTI_LINE_17);
        LL_EXTI_EnableRisingTrig_0_31(LL_EXTI_LINE_17);
        // 设置闹钟值
        LL_RTC_ALARM_SetCounter(RTC, tick_seconds);
        // 使能闹钟中断
        LL_RTC_DisableWriteProtection(RTC);
        LL_RTC_EnableIT_ALR(RTC);
        stm32_rtc_lwoff_wait();
        LL_RTC_EnableWriteProtection(RTC);
        // 配置中断
        NVIC_SetPriority(RTC_Alarm_IRQn, 0);
        NVIC_EnableIRQ(RTC_Alarm_IRQn);
        
    } else {
        // 禁用闹钟中断
        LL_RTC_DisableWriteProtection(RTC);
        LL_RTC_DisableIT_ALR(RTC);
        stm32_rtc_lwoff_wait();
        LL_RTC_EnableWriteProtection(RTC);
        // 禁用 EXTI Line 17
        LL_EXTI_DisableIT_0_31(LL_EXTI_LINE_17);
        LL_EXTI_DisableRisingTrig_0_31(LL_EXTI_LINE_17);
        // 禁用 NVIC
        NVIC_DisableIRQ(RTC_Alarm_IRQn);
    }
}

// @brief 使能RTC唤醒
int32_t drv_rtc_wkup_enable_set(bool is_enable, uint32_t ticks)
{
    return 0;
}
#endif /*defined(BSP_USING_RTC)*/

