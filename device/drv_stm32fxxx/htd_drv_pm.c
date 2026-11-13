//#define DEBUG_LVL       4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG       "drv.pm"
#include "htd_drv_common.h"
#include "htd_device.h"
#include "htd_drv_wkup.h"


#if defined(HAL_RTC_MODULE_ENABLED)
// PM 定时器要求
//     1：sleep/stop 模式能一直运行
//     2：能精准计时
//     3: 能获取当前计数值，能设置下一次比较值触发中断
//////////////////////////////////////////////////////////////////////
HTD_SECTION_DEF(HTD_PM_SLEEP_SECTION_NAME, pm_sleep_cb_intance_t);
static const uint32_t* m_sleep_section_start;
static const uint32_t* m_sleep_section_end;
system_clock_config_handle_t system_clock_config_handle = NULL;

// 计算 CPU 使用率
static uint32_t m_cpu_monitor_start_ms = 0;
static uint32_t m_cpu_monitor_sleep_ticks;
static uint8_t m_cpu_usage_percent = 0;


// @brief 初始化
int32_t htd_pm_init(system_clock_config_handle_t handle)
{
    // 初始化全局变量
    m_sleep_section_start = (uint32_t*)HTD_SECTION_START_ADDR(HTD_PM_SLEEP_SECTION_NAME);
    m_sleep_section_end = (uint32_t*)HTD_SECTION_END_ADDR(HTD_PM_SLEEP_SECTION_NAME);
    m_cpu_monitor_sleep_ticks = 0;
    m_cpu_monitor_start_ms = htd_pm_cpu_time_ms_get();
    system_clock_config_handle = handle;
    
    // 初始化唤醒定时器
    htd_wkup_timer_init();

#if 0
    while (1) {
        htd_pm_run(500);
        print_debug("timestamps=%u", htd_rtc_timestamps_get());
        htd_delay_ms(500);
    }
#endif
    return 0;
}

// @brief CPU 监督
static void drv_pm_cpu_monitor(uint32_t sleep_ticks)
{
    uint32_t total_ticks;
    uint32_t now_ms = htd_pm_cpu_time_ms_get();
    uint32_t inter_ms = htd_time_ms_diff(now_ms, m_cpu_monitor_start_ms);
    m_cpu_monitor_sleep_ticks += sleep_ticks;
    if (inter_ms >= CPU_USAGE_INTER_MS) {
        total_ticks = htd_wkup_timer_ms2ticks(inter_ms);
        m_cpu_usage_percent = (total_ticks - m_cpu_monitor_sleep_ticks) * 100 / total_ticks; 
        m_cpu_monitor_start_ms = now_ms;
        m_cpu_monitor_sleep_ticks = 0;
    }
}

// @brief 进入低功耗模式
void drv_pm_enter_lowpower_mode(uint32_t sleep_ms)
{
#if defined(PM_USING_SLEEP_MODE)        // sleep 模式
    HAL_PWR_EnterSLEEPMode(PWR_LOWPOWERREGULATOR_ON, PWR_SLEEPENTRY_WFE);
    htd_rtc_register_sync_wait();

#elif defined(PM_USING_STOP_MODE)
    HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFE);
    if (system_clock_config_handle) {
        system_clock_config_handle();
    }
    htd_rtc_register_sync_wait();
    
#else
    htd_delay_ms(sleep_ms);
    htd_delay_us(1);
#endif
}

// @brief 
static void htd_pm_rtt_enable_set(bool is_enable)
{
    //if (is_enable) {
    //    drv_pin_af_swd();
    //}
}

// @brief 运行
uint32_t htd_pm_run(uint32_t sleep_ms)
{
#ifdef BSP_WDT_TIMEOUT_MS
    if (sleep_ms > BSP_WDT_TIMEOUT_MS - 10) {
        sleep_ms = BSP_WDT_TIMEOUT_MS - 10;
    }
#endif /*BSP_WDT_TIMEOUT_MS*/
    
    // 进入睡眠前回调
    pm_sleep_cb_intance_t* p_instance;
    const uint32_t* p_item = htd_section_first_item_get(m_sleep_section_start, m_sleep_section_end);
    while (p_item) {
        p_instance = (pm_sleep_cb_intance_t*)p_item;
        if (p_instance->magic == HTD_PM_SLEEP_MAGIC) {
            p_instance->before_sleep_func(p_instance->before_sleep_func_arg);
        }
        p_item = htd_section_next_item_get(p_item, m_sleep_section_end, sizeof(pm_sleep_cb_intance_t));
    }
    // 喂狗
    htd_dev_wdt_feed();

    // 设置唤醒时间
    uint32_t real_sleep_ms = 0;
    uint32_t sleep_ticks = 0;
    uint32_t start_ticks = htd_wkup_timer_ms_set(sleep_ms);
    if (start_ticks > 0) {
        __DSB();
        // systick 挂起
        HAL_SuspendTick();
        // RTT 禁用
        htd_pm_rtt_enable_set(false);
        // 进入休眠
        drv_pm_enter_lowpower_mode(sleep_ms);
        // RTT 使能
        htd_pm_rtt_enable_set(true);
        // 计算休眠时长
        sleep_ticks = htd_wkup_timer_sleep_ticks_get(start_ticks);
        real_sleep_ms = htd_wkup_timer_ticks2ms(sleep_ticks);
        if (real_sleep_ms > sleep_ms) {
            real_sleep_ms = sleep_ms;
        }
        // systick 使能并补充休眠时长
        HAL_ResumeTick(real_sleep_ms);
        // RTOS 恢复计数
        htd_task_step_tick(real_sleep_ms);
        print_warning("sleep_ms=%u->%u, sleep_ticks=%u->%u", sleep_ms, real_sleep_ms, start_ticks, sleep_ticks);
    } 
    
    // 退出睡眠回调
    p_item = htd_section_first_item_get(m_sleep_section_start, m_sleep_section_end);
    while (p_item) {
        p_instance = (pm_sleep_cb_intance_t*)p_item;
        if (p_instance->magic == HTD_PM_SLEEP_MAGIC) {
            p_instance->after_sleep_func(p_instance->after_sleep_func_arg);
        }
        p_item = htd_section_next_item_get(p_item, m_sleep_section_end, sizeof(pm_sleep_cb_intance_t));
    }
    drv_pm_cpu_monitor(sleep_ticks);
    return real_sleep_ms;
}

// @brief 获取 CPU 运行百分比
uint8_t htd_pm_cpu_usage(void)
{
    return m_cpu_usage_percent;
}

// @brief 遍历所有回调区块
void htd_pm_section_show(void)
{
    pm_sleep_cb_intance_t* p_instance;
    const uint32_t* p_item = htd_section_first_item_get(m_sleep_section_start, m_sleep_section_end);
    while (p_item) {
        p_instance = (pm_sleep_cb_intance_t*)p_item;
        if (p_instance->magic == HTD_PM_SLEEP_MAGIC) {
            print_warning("PM section name: %s_%d", p_instance->p_name, p_instance->priority);
        }
        p_item = htd_section_next_item_get(p_item, m_sleep_section_end, sizeof(pm_sleep_cb_intance_t));
    }
}
#endif /*defined(HAL_RTC_MODULE_ENABLED)*/

// @brief 获取 CPU 运行毫秒数
uint32_t htd_pm_cpu_time_ms_get(void)
{
    extern uint32_t HAL_GetTick(void);
    uint32_t time_ms = HAL_GetTick();
    if (time_ms == 0) {
        time_ms = 1;
    }
    return time_ms;
}

// @brief 重启
void htd_pm_reboot(void)
{
    NVIC_SystemReset();
    while (1);
}

// @brief 关机, 以下几种模式可以唤醒
//    WKUP 引脚：特定引脚（通常是 PA0）上的上升沿。
//    RTC 事件：RTC 闹钟 (Alarm A/B)、RTC 唤醒定时器、入侵事件或时间戳事件。
//    外部复位：NRST 引脚上的复位信号。
//    IWDG 复位：独立看门狗 (IWDG) 触发的复位
void htd_pm_shutdown(void)
{
    // 开启PWR时钟
    //__HAL_RCC_PWR_CLK_ENABLE();
    // 清除待机唤醒标志（必须，否则下次进Standby会直接被立刻唤醒）
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
    // 使能PA0作为WKUP上升沿唤醒引脚; 该按键必须为按下拉低，抬起拉高，拉高时瞬间唤醒
    HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1);
    // 进入待机
    HAL_PWR_EnterSTANDBYMode();
}


