#define DEBUG_LVL   4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG   "drv.rtc"
#include "htd_drv_common.h"
#include "htd_device.h"
#include "htd_drv_rtc.h"


#if defined(BSP_USING_RTC) && defined(RTC) && (!defined(SOC_SERIES_STM32F1))
//////////////////////////////////////////////////////////////////////
static RTC_HandleTypeDef m_rtc_handle;


// @brief 设置时间(年月日时分秒，参数包含时区)
int32_t htd_rtc_time_set(htd_time_t* p_time)
{
    uint32_t local_timestamps = htd_time2timestamps(p_time) - htd_rtc_timezone_get();
    htd_timestamps2time(local_timestamps, p_time);

    RTC_DateTypeDef RTC_DateStruct = {0};
    RTC_DateStruct.Date = p_time->day;
    RTC_DateStruct.Month = p_time->month;
    RTC_DateStruct.Year = p_time->year - 2000;
    RTC_DateStruct.WeekDay = p_time->wday;
    if (HAL_RTC_SetDate(&m_rtc_handle, &RTC_DateStruct, RTC_FORMAT_BIN) != HAL_OK) {
        return -1;
    }

    RTC_TimeTypeDef RTC_TimeStruct = {0};
    RTC_TimeStruct.Seconds = p_time->second;
    RTC_TimeStruct.Minutes = p_time->min;
    RTC_TimeStruct.Hours = p_time->hour;
    if (HAL_RTC_SetTime(&m_rtc_handle, &RTC_TimeStruct, RTC_FORMAT_BIN) != HAL_OK) {
        return -1;
    }
    return 0;
}

// @brief 获取时间
static htd_time_t rtc_time_get(void)
{
    RTC_TimeTypeDef RTC_TimeStruct = {0};
    HAL_RTC_GetTime(&m_rtc_handle, &RTC_TimeStruct, RTC_FORMAT_BIN);
    RTC_DateTypeDef RTC_DateStruct = {0};
    HAL_RTC_GetDate(&m_rtc_handle, &RTC_DateStruct, RTC_FORMAT_BIN);

    htd_time_t time;
    time.second = RTC_TimeStruct.Seconds;
    time.min = RTC_TimeStruct.Minutes;
    time.hour = RTC_TimeStruct.Hours;
    time.day = RTC_DateStruct.Date;
    time.month = RTC_DateStruct.Month;
    time.year = RTC_DateStruct.Year + 2000;
    time.wday = RTC_DateStruct.WeekDay;
    return time;
}

// @brief 获取当前的时间(年月日时分秒，包含时区)
htd_time_t htd_rtc_time_get(void)
{
    uint32_t local_timestamps;
    htd_time_t time = rtc_time_get();
    local_timestamps = htd_time2timestamps(&time) + htd_rtc_timezone_get();
    htd_timestamps2time(local_timestamps, &time);
    return time;
}

// @brief 获取当前的RTC时间戳(不包含时区)
uint32_t htd_rtc_timestamps_get(void)
{
    htd_time_t time = rtc_time_get();
    return htd_time2timestamps(&time);
}

// @brief 备份区数据; 
// @param bkp_idx 备份区序号
// @return 返回获取到的数据长度
int32_t htd_rtc_bkp_data_get(uint8_t bkp_idx, uint8_t* p_data, uint32_t data_len)
{
#ifdef RTC_BACKUP_SUPPORT
    if (bkp_idx + 1 < RTC_BKP_NUMBER) {
        uint16_t data = HAL_RTCEx_BKUPRead(&m_rtc_handle, RTC_BKP_DR1 + bkp_idx + 1);
        if (data_len > sizeof(data)) {
            data_len = sizeof(data);
        }
        memcpy(p_data, &data, data_len);
        return data_len;
    } 
#endif
    return 0;
}

// @brief 设置备份区数据
// @param bkp_idx 备份区序号
// @return 返回设置的数据长度
int32_t htd_rtc_bkp_data_set(uint8_t bkp_idx, const uint8_t* p_data, uint32_t data_len)
{
#ifdef RTC_BACKUP_SUPPORT
    if (bkp_idx + 1 < RTC_BKP_NUMBER) {
        uint32_t data;
        if (data_len == 1) {
            data = p_data[0];
        } else {
            data = *(uint16_t*)p_data;
            data_len = sizeof(uint16_t);
        }
        HAL_RTCEx_BKUPWrite(&m_rtc_handle, RTC_BKP_DR1 + bkp_idx + 1, data);
        return data_len;
    } 
#endif
    return 0;
}

// @brief 唤醒参数初始化
static void rtc_wakeup_param_init(uint32_t rcc_freq)
{
    m_rtc_handle.Init.OutPut = RTC_OUTPUT_DISABLE;
    m_rtc_handle.Init.HourFormat = RTC_HOURFORMAT_24;
    m_rtc_handle.Init.OutPut = RTC_OUTPUT_DISABLE;
    m_rtc_handle.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    m_rtc_handle.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
    // 1 Hz = RTCCLK / [(AsynchPrediv + 1) × (SynchPrediv + 1)]
    m_rtc_handle.Init.AsynchPrediv = RTC_ASYN_PRE_DIV;
    m_rtc_handle.Init.SynchPrediv = rcc_freq / (RTC_ASYN_PRE_DIV+1) - 1;
}

// @brief 初始化 RTC
int32_t htd_rtc_init(uint32_t start_timestamps)
{
    uint32_t rtc_rcc_freq;
    //__HAL_RCC_PWR_CLK_ENABLE();
#ifdef __HAL_RCC_BKP_CLK_ENABLE
    __HAL_RCC_BKP_CLK_ENABLE();
#endif
    HAL_PWR_EnableBkUpAccess();    
    
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
    
#if defined(BSP_CLOCK_USE_XTAL32768)
    // 外部低速时钟，32768
    rtc_rcc_freq = LSE_VALUE;
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSE;
    RCC_OscInitStruct.LSEState = RCC_LSE_ON;
    RCC_OscInitStruct.LSIState = RCC_LSI_OFF;
    PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
#else
    // 内部低速时钟，32000/40000
    rtc_rcc_freq = LSI_VALUE;
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI;
    RCC_OscInitStruct.LSEState = RCC_LSE_OFF;
    RCC_OscInitStruct.LSIState = RCC_LSI_ON;
    PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
#endif
    if (HAL_OK != HAL_RCC_OscConfig(&RCC_OscInitStruct)) {
        print_error("RTC osc config failed.");
        return -1;
    }
    if (HAL_OK != HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct)) {
        print_error("periph clock config failed.");
        return -1;
    }
    __HAL_RCC_RTC_ENABLE();
    //HAL_PWR_EnableBkUpAccess();
    m_rtc_handle.Instance = RTC;
    
#ifdef RTC_BACKUP_SUPPORT
    if (HAL_RTCEx_BKUPRead(&m_rtc_handle, RTC_BKP_DR1) == 0xa58d) {
        return 0;
    }
#else
    if (LL_RTC_IsActiveFlag_INITS(m_rtc_handle.Instance) == SET) {
        print_warning("RTC is init.");
        return 0;
    }
#endif

    rtc_wakeup_param_init(rtc_rcc_freq);
    if (HAL_RTC_Init(&m_rtc_handle) != HAL_OK) {
        return -1;
    }
    
#ifdef RTC_BACKUP_SUPPORT
    HAL_RTCEx_BKUPWrite(&m_rtc_handle, RTC_BKP_DR1, 0xa58d);
#endif
    
    // 设置默认的rtc时间
    htd_time_t time;
    if (start_timestamps == 0xffffffff) {
        time.year = __YEAR__;
        time.month = __MONTH__;
        time.day = __DAY__;
        time.hour = __HOUR__;
        time.min = __MINUTE__;
        time.second = __SECOND__;
        start_timestamps = htd_time2timestamps(&time);
    } 
    
    start_timestamps -= htd_rtc_timezone_get();
    htd_timestamps2time(start_timestamps, &time);
    htd_rtc_time_set(&time);
    return 0;
}

// @brief 获取句柄
RTC_HandleTypeDef* drv_rtc_handle_get(void) 
{
    return &m_rtc_handle;
}

// @brief 获取亚秒频率
uint32_t drv_rtc_ss_freq_get(void)
{
#if defined(BSP_CLOCK_USE_XTAL32768)
    return (LSE_VALUE / (RTC_ASYN_PRE_DIV + 1));
#else
    return (LSI_VALUE / (RTC_ASYN_PRE_DIV + 1));
#endif
}

// @brief 获取当前亚秒
uint32_t drv_rtc_ss_ticks_get(void)
{
#if (!defined(SOC_SERIES_STM32F2))
    RTC_TimeTypeDef RTC_TimeStruct = {0};
    HAL_RTC_GetTime(&m_rtc_handle, &RTC_TimeStruct, RTC_FORMAT_BIN);
    RTC_DateTypeDef RTC_DateStruct = {0};
    HAL_RTC_GetDate(&m_rtc_handle, &RTC_DateStruct, RTC_FORMAT_BIN);
    uint32_t seconds = RTC_TimeStruct.Hours * 3600 + RTC_TimeStruct.Minutes * 60 + RTC_TimeStruct.Seconds;
    uint32_t ticks = seconds * drv_rtc_ss_freq_get();
    ticks += drv_rtc_ss_freq_get() - RTC_TimeStruct.SubSeconds;
    return ticks;
#else
    return 0;
#endif
}

// @brief 使能RTC闹钟
void drv_rtc_alarm_enable_set(bool is_enable, uint32_t tick_seconds)
{
    if (is_enable) {
        RTC_AlarmTypeDef rtc_alarm = {0};
        rtc_alarm.AlarmTime.Hours = tick_seconds / 3600;
        rtc_alarm.AlarmTime.Minutes = (tick_seconds % 3600) / 60;
        rtc_alarm.AlarmTime.Seconds = tick_seconds % 60;
        rtc_alarm.Alarm = RTC_ALARM_A;
        if (HAL_RTC_SetAlarm_IT(&m_rtc_handle, &rtc_alarm, RTC_FORMAT_BIN) != HAL_OK) {
            Error_Handler();
        }
    } else {
        HAL_RTC_DeactivateAlarm(&m_rtc_handle, RTC_ALARM_A);
    }
}

#if defined(SOC_SERIES_STM32F2) || defined(SOC_SERIES_STM32F4) || defined(SOC_SERIES_STM32F7)
// @brief 使能RTC唤醒
int32_t drv_rtc_wkup_enable_set(bool is_enable, uint32_t ticks)
{
    __HAL_RTC_WAKEUPTIMER_CLEAR_FLAG(&m_rtc_handle, RTC_FLAG_WUTF);
    if (is_enable) {
        // 多少个 TICK 后唤醒系统
        if (HAL_RTCEx_SetWakeUpTimer_IT(&m_rtc_handle, ticks, RTC_WAKEUPCLOCK_RTCCLK_DIV16) != HAL_OK) {
            ASSERT(0);
            return 0;
        }
    } else {
        HAL_RTCEx_DeactivateWakeUpTimer(&m_rtc_handle);
    }
    return 0;
}
#endif 
#endif /*defined(BSP_USING_RTC)*/





#if 0
//////////////////////////////////////////////////////////////////////
// @brief 等待上次写操作完成
__STATIC_INLINE void stm32_rtc_lwoff_wait(void)
{
    while (LL_RTC_IsActiveFlag_WUTW(RTC) == 0U) {}
}

// @brief 同步RTC寄存器(休眠唤醒后需要调用一次)
void htd_rtc_register_sync_wait(void)
{
    LL_RTC_ClearFlag_RS(RTC);
    while (LL_RTC_IsActiveFlag_RS(RTC) == 0) {}
}

// @brief 进入初始化模式
static void rtc_enter_init_mode(void)
{
    LL_RTC_EnableInitMode(RTC);
    while (LL_RTC_IsActiveFlag_INIT(RTC) != 1) {}
}

// @brief 退出初始化模式
static void rtc_exit_init_mode(void)
{
    LL_RTC_DisableInitMode(RTC);
    htd_rtc_register_sync_wait();
}

// @brief 设置时间(年月日时分秒)
int32_t htd_rtc_time_set(htd_time_t* p_time)
{
    LL_RTC_DateTypeDef RTC_DateStruct;
    RTC_DateStruct.Year = p_time->year - 2000;
    RTC_DateStruct.Month = p_time->month;
    RTC_DateStruct.Day = p_time->day;
    RTC_DateStruct.WeekDay = (p_time->wday > 6) ? 0 : p_time->wday;
    if (LL_RTC_DATE_Init(RTC, LL_RTC_FORMAT_BIN, &RTC_DateStruct) != SUCCESS) {
        return -1;
    }
    
    LL_RTC_TimeTypeDef RTC_TimeStruct;
    RTC_TimeStruct.TimeFormat = LL_RTC_TIME_FORMAT_AM_OR_24;
    RTC_TimeStruct.Hours = p_time->hour;
    RTC_TimeStruct.Minutes = p_time->min;
    RTC_TimeStruct.Seconds = p_time->second;
    if (LL_RTC_TIME_Init(RTC, LL_RTC_FORMAT_BIN, &RTC_TimeStruct) != SUCCESS) {
        return -1;
    }
    return 0;
}

// @brief 获取当前的时间(年月日时分秒)
htd_time_t htd_rtc_time_get(void)
{
    uint8_t temp;
    htd_time_t time;

#ifndef SOC_SERIES_STM32F2
    // 读取 SSR 亚秒寄存器之后，芯片会备份冻结时分秒，年月日；直到读取DR寄存器才解冻
    LL_RTC_TIME_GetSubSecond(RTC);
#endif
    uint32_t time_hms = LL_RTC_TIME_Get(RTC);
    uint32_t date = LL_RTC_DATE_Get(RTC);

    // 时间：0x00HHMMSS
    temp = __LL_RTC_GET_SECOND(time_hms);
    time.second = __LL_RTC_CONVERT_BCD2BIN(temp);
    temp = __LL_RTC_GET_MINUTE(time_hms);
    time.min = __LL_RTC_CONVERT_BCD2BIN(temp);
    temp = __LL_RTC_GET_HOUR(time_hms);
    time.hour = __LL_RTC_CONVERT_BCD2BIN(temp);

    // 日期：0xWWDDMMYY
    time.wday = __LL_RTC_GET_WEEKDAY(date);
    if (time.wday > 6) {
        time.wday = 0;
    }
    temp = __LL_RTC_GET_DAY(date);
    time.day = __LL_RTC_CONVERT_BCD2BIN(temp);
    temp = __LL_RTC_GET_MONTH(date)
    time.month = __LL_RTC_CONVERT_BCD2BIN(temp);
    temp = __LL_RTC_GET_YEAR(date)
    time.year = __LL_RTC_CONVERT_BCD2BIN(temp) + 2000;
    return time;
}

// @brief 获取当前的时间戳
uint32_t htd_rtc_timestamps_get(void)
{
    htd_time_t time = htd_rtc_time_get();
    return htd_time2timestamps(&time);
}

// @brief 备份区数据; 
// @param bkp_idx 备份区序号
// @return 返回获取到的数据长度
int32_t htd_rtc_bkp_data_get(uint8_t bkp_idx, uint8_t* p_data, uint32_t data_len)
{
#ifdef RTC_BACKUP_SUPPORT
    if (bkp_idx + 1 < RTC_BKP_NUMBER) {
        uint16_t data = LL_RTC_BAK_GetRegister(RTC, RTC_BKP_DR1 + bkp_idx + 1);
        if (data_len > sizeof(data)) {
            data_len = sizeof(data);
        }
        htd_memcpy(p_data, &data, data_len);
        return data_len;
    } 
#endif
    return 0;
}

// @brief 设置备份区数据
// @param bkp_idx 备份区序号
// @return 返回设置的数据长度
int32_t htd_rtc_bkp_data_set(uint8_t bkp_idx, const uint8_t* p_data, uint32_t data_len)
{
#ifdef RTC_BACKUP_SUPPORT
    if (bkp_idx + 1 < RTC_BKP_NUMBER) {
        uint32_t data;
        if (data_len == 1) {
            data = p_data[0];
        } else {
            data = *(uint16_t*)p_data;
            data_len = sizeof(uint16_t);
        }
        LL_RTC_BAK_SetRegister(RTC, RTC_BKP_DR1 + bkp_idx + 1, data);
        return data_len;
    } 
#endif
    return 0;
}

// @brief 配置 RTC 时钟
static uint32_t drv_rtc_clock_config(void)
{
#if defined(BSP_CLOCK_USE_XTAL32768)
    if (LL_RCC_LSE_IsReady() == 0) {
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
    LL_RTC_InitTypeDef rtc_initstruct;
    rtc_initstruct.HourFormat = LL_RTC_HOURFORMAT_24HOUR;
    rtc_initstruct.AsynchPrescaler = RTC_ASYN_PRE_DIV;
    rtc_initstruct.SynchPrescaler = (rtc_rcc_freq / (RTC_ASYN_PRE_DIV+1) - 1);
    if (LL_RTC_Init(RTC, &rtc_initstruct) != SUCCESS) {
        print_error("rtc prescaler init failed.");
        return;
    }
}

// @brief 初始化 RTC
int32_t htd_rtc_init(uint32_t start_timestamps)
{
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
    LL_PWR_EnableBkUpAccess();

#ifdef RTC_BACKUP_SUPPORT
    if (LL_RTC_BAK_GetRegister(RTC, LL_RTC_BKP_DR0) == RTC_INIT_FLAG) {
        htd_rtc_register_sync_wait();       // 已初始化，只需等待同步即可
        return 0;
    }
    LL_RTC_BAK_SetRegister(RTC, LL_RTC_BKP_DR0, RTC_INIT_FLAG);
#else
    if (LL_RTC_IsActiveFlag_INITS(RTC) == SET) {
        print_warning("RTC is init.");
        htd_rtc_register_sync_wait();       // 已初始化，只需等待同步即可
        return 0;
    }
#endif

    // 配置时钟源
    uint32_t rtc_rcc_freq = drv_rtc_clock_config();
    htd_rtc_register_sync_wait();
    
    // 配置预分频器
    drv_rtc_prescaler_config(rtc_rcc_freq);
    
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
    htd_rtc_time_set(&time);
    return 0;
}

// @brief 获取亚秒频率
uint32_t drv_rtc_ss_freq_get(void)
{
#if defined(BSP_CLOCK_USE_XTAL32768)
    return (LSE_VALUE / (RTC_ASYN_PRE_DIV + 1));
#else
    return (LSI_VALUE / (RTC_ASYN_PRE_DIV + 1));
#endif
}

// @brief 获取当前亚秒
uint32_t drv_rtc_ss_ticks_get(void)
{
#if (!defined(SOC_SERIES_STM32F2))
    uint32_t ssr = LL_RTC_TIME_GetSubSecond(RTC);
    uint32_t time_hms = LL_RTC_TIME_Get(RTC);
    (void)LL_RTC_DATE_Get(RTC);

    // 时间：0x00HHMMSS
    uint8_t temp = __LL_RTC_GET_SECOND(time_hms);
    uint32_t seconds = __LL_RTC_CONVERT_BCD2BIN(temp);
    temp = __LL_RTC_GET_MINUTE(time_hms);
    uint32_t minutes = __LL_RTC_CONVERT_BCD2BIN(temp);
    temp = __LL_RTC_GET_HOUR(time_hms);
    uint32_t hours = __LL_RTC_CONVERT_BCD2BIN(temp);
    
    uint32_t timestamps = hours * 3600 + minutes * 60 + seconds;
    uint32_t ticks = timestamps * drv_rtc_ss_freq_get();
    ticks += drv_rtc_ss_freq_get() - ssr;
    return ticks;
#else
    return 0;
#endif
}

// @brief 使能RTC闹钟
void drv_rtc_alarm_enable_set(bool is_enable, uint32_t tick_seconds, uint32_t sub_seconds)
{
    // 清除闹钟标志
    LL_RTC_ClearFlag_ALRA(RTC);
    LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_17);

    if (is_enable) {
        // 使能中断线
        LL_EXTI_EnableIT_0_31(LL_EXTI_LINE_17);
        LL_EXTI_EnableRisingTrig_0_31(LL_EXTI_LINE_17);
        
        LL_RTC_AlarmTypeDef rtc_alarm_initstruct;
        rtc_alarm_initstruct.AlarmTime.TimeFormat = LL_RTC_ALMA_TIME_FORMAT_AM;
        rtc_alarm_initstruct.AlarmTime.Hours = tick_seconds / 3600;
        rtc_alarm_initstruct.AlarmTime.Minutes = (tick_seconds % 3600) / 60;
        rtc_alarm_initstruct.AlarmTime.Seconds = tick_seconds % 60;
        rtc_alarm_initstruct.AlarmMask = LL_RTC_ALMA_MASK_NONE;
        rtc_alarm_initstruct.AlarmDateWeekDaySel = LL_RTC_ALMA_DATEWEEKDAYSEL_DATE;
        rtc_alarm_initstruct.AlarmDateWeekDay = 0x01;
        if (LL_RTC_ALMA_Init(RTC, LL_RTC_FORMAT_BIN, &rtc_alarm_initstruct) != SUCCESS) {
            print_error("Alarm init failed. tick_seconds=%d", tick_seconds);
            return;
        }
        
        LL_RTC_DisableWriteProtection(RTC);
        // 设置亚秒
    #ifdef RTC_ALARMSUBSECONDMASK_NONE
        LL_RTC_ALMA_SetSubSecond(RTC, sub_seconds);
        LL_RTC_ALMA_SetSubSecondMask(RTC, RTC_ALARMSUBSECONDMASK_NONE);
    #endif
        LL_RTC_ALMA_Enable(RTC);
        LL_RTC_EnableWriteProtection(RTC);
        // 使能闹钟A中断
        LL_RTC_EnableIT_ALRA(RTC);
        // 使能 NVIC
        NVIC_SetPriority(RTC_IRQn, 0x00);
        NVIC_EnableIRQ(RTC_IRQn); 
        return;
    }

    LL_RTC_DisableWriteProtection(RTC);
    LL_RTC_ALMA_Disable(RTC);
    LL_RTC_EnableWriteProtection(RTC);
    LL_RTC_DisableIT_ALRA(RTC);
    
    LL_EXTI_DisableIT_0_31(LL_EXTI_LINE_17);
    LL_EXTI_DisableRisingTrig_0_31(LL_EXTI_LINE_17);
    NVIC_DisableIRQ(RTC_IRQn);
}

#ifdef RTC_WAKEUP_SUPPORT
// @brief 使能RTC唤醒
int32_t drv_rtc_wkup_enable_set(bool is_enable, uint32_t ticks)
{
    // 清除唤醒标志
    LL_RTC_ClearFlag_WUT(RTC);
    LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_20);

    if (is_enable) {
        /* 配置 EXTI Line 20 */
        LL_EXTI_EnableIT_0_31(LL_EXTI_LINE_20);
        LL_EXTI_EnableRisingTrig_0_31(LL_EXTI_LINE_20);
        
        LL_RTC_DisableWriteProtection(RTC);
        while (LL_RTC_IsActiveFlag_WUTW(RTC) == 0U) {}
        LL_RTC_WAKEUP_SetClock(RTC, LL_RTC_WAKEUPCLOCK_DIV_16);
        LL_RTC_WAKEUP_SetAutoReload(RTC, ticks);
        // 使能唤醒定时器
        LL_RTC_WAKEUP_Enable(RTC);
        LL_RTC_EnableWriteProtection(RTC);
        // 使能唤醒中断
        LL_RTC_EnableIT_WUT(RTC);
        // 使能 NVIC
        NVIC_SetPriority(RTC_IRQn, 0);
        NVIC_EnableIRQ(RTC_IRQn);
        return 0;
    } 
    
    LL_RTC_DisableWriteProtection(RTC);
    LL_RTC_WAKEUP_Disable(RTC);
    LL_RTC_EnableWriteProtection(RTC);
    LL_RTC_DisableIT_WUT(RTC);
    
    LL_EXTI_DisableIT_0_31(LL_EXTI_LINE_20);
    LL_EXTI_DisableRisingTrig_0_31(LL_EXTI_LINE_20);
    NVIC_DisableIRQ(RTC_IRQn);
    return 0;
}
#endif 
#endif /*defined(BSP_USING_RTC)*/

