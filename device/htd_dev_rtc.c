#define DEBUG_LVL       1       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG       "dev.rtc"
#include "htd_dev_rtc.h"
#include "htd_common.h"
#include "htd_components.h"


//////////////////////////////////////////////////////////////////////
#ifdef _WIN32
#include <windows.h>
#include <WinBase.h>
static uint32_t m_timestamps;
static htd_time_t m_time;
#endif
static int32_t m_timezone = 8*3600;


// @brief 当前时获取
uint8_t htd_rtc_hour_get(void)
{
#ifdef _WIN32
    return htd_rtc_time_get().hour;
#else
    uint32_t timestamps = htd_rtc_local_timestamps_get();
    return htd_timestamps2hour(timestamps);
#endif
}

// @brief 当前分获取
uint8_t htd_rtc_minute_get(void)
{
#ifdef _WIN32
    return htd_rtc_time_get().min;
#else
    uint32_t timestamps = htd_rtc_local_timestamps_get();
    return htd_timestamps2minute(timestamps);
#endif
}

// @brief 当前秒获取
uint8_t htd_rtc_second_get(void)
{
#ifdef _WIN32
    return htd_rtc_time_get().second;
#else
    uint32_t timestamps = htd_rtc_local_timestamps_get();
    return timestamps % 60;
#endif
}

// @brief 获取当天的年
uint16_t htd_rtc_year_get(void)
{
#ifdef _WIN32
    return htd_rtc_time_get().year;
#else
    htd_time_t cdr = htd_rtc_time_get();
    return cdr.year;
#endif
}

// @brief 获取当天的月
uint8_t htd_rtc_month_get(void)
{
#ifdef _WIN32
    return htd_rtc_time_get().month;
#else
    htd_time_t cdr = htd_rtc_time_get();
    return cdr.month;
#endif
}

// @brief 获取当天的日期
uint8_t htd_rtc_day_get(void)
{
#ifdef _WIN32
    return htd_rtc_time_get().day;
#else
    htd_time_t cdr = htd_rtc_time_get();
    return cdr.day;
#endif
}

// @brief 获取当天的星期
uint8_t htd_rtc_week_get(void)
{
#ifdef _WIN32
    return htd_rtc_time_get().wday;
#else
    htd_time_t cdr = htd_rtc_time_get();
    return cdr.wday;
#endif
}

// @brief 设置时区
void htd_rtc_timezone_set(int32_t seconds)
{
    if (seconds > -(12*3600) && seconds < (12*3600)) {
        m_timezone = seconds;
    }
}

// @brief 获取时区
int32_t htd_rtc_timezone_get(void)
{
    return m_timezone;
}

// @brief 初始化 RTC
// @param start_timestamps 起始时间戳，如果为0xffffffff, 则为默认时间2020-1-1 0:0:0
__WEAK int32_t htd_rtc_init(uint32_t start_timestamps)
{
    return 0;
}
// @brief 同步RTC寄存器(休眠唤醒后需要调用一次)
__WEAK void htd_rtc_register_sync_wait(void)
{
    
}
// @brief 设置时间(年月日时分秒)
__WEAK int32_t htd_rtc_time_set(htd_time_t* p_time)
{
    return -1;
}

// @brief 获取当前的时间(年月日时分秒)，WEEK函数
__WEAK htd_time_t htd_rtc_time_get(void)
{
#ifdef _WIN32
    uint32_t timestamps = _time32(NULL) + htd_rtc_timezone_get();
    if (htd_timestamps2day(timestamps) != htd_timestamps2day(m_timestamps)) {
        htd_timestamps2time(timestamps, &m_time);
        m_timestamps = timestamps;
        return m_time;
    }
    timestamps %= ONEDAY_SECONDS;
    m_time.hour = timestamps / 3600;
    timestamps %= 3600;
    m_time.min = timestamps / 60;
    m_time.second = timestamps % 60;
    return m_time;
#else
    htd_time_t cdr;
    uint32_t timestamps = htd_rtc_local_timestamps_get();
    htd_timestamps2time(timestamps, &cdr);
    return cdr;
#endif
}

// @brief 获取当前的时间戳，WEEK函数
__WEAK uint32_t htd_rtc_timestamps_get(void)
{
#ifdef _WIN32
    return _time32(NULL);
#else
    htd_time_t time = htd_rtc_time_get();
    return htd_time2timestamps(&time);
#endif
}

// @brief 备份区数据; 
// @param bkp_idx 备份区序号
// @return 返回获取到的数据长度
__WEAK int32_t htd_rtc_bkp_data_get(uint8_t bkp_idx, uint8_t* p_data, uint32_t data_len)
{
    return 0;
}
// @brief 设置备份区数据
// @param bkp_idx 备份区序号
// @return 返回设置的数据长度
__WEAK int32_t htd_rtc_bkp_data_set(uint8_t bkp_idx, const uint8_t* p_data, uint32_t data_len)
{
    return 0;
}


//////////////////////////////////////////////////////////////////////
// @brief 本地时间戳
uint32_t htd_rtc_local_timestamps_get(void)
{
    return htd_rtc_timestamps_get() + htd_rtc_timezone_get();
}

// @brief 本地时间戳转 UTC0 时间戳
uint32_t htd_rtc_localtimestamps2utc0timestamps(uint32_t localtimestamps)
{
    return localtimestamps - htd_rtc_timezone_get();
}

// @brief UTC0 时间戳转本地时间戳
uint32_t htd_rtc_utc0timestamps2localtimestamps(uint32_t utc0timestamps)
{
    return utc0timestamps + htd_rtc_timezone_get();
}

// @brief 获取昨天的年
uint16_t htd_rtc_yesterday_year_get(void)
{
    htd_time_t time = htd_rtc_time_get();
    htd_time_t yesterday = htd_time_after_nday_get(&time, -1);
    return yesterday.year;
}

// @brief 获取昨天的月
uint8_t htd_rtc_yesterday_month_get(void)
{
    htd_time_t time = htd_rtc_time_get();
    htd_time_t yesterday = htd_time_after_nday_get(&time, -1);
    return yesterday.month;
}

// @brief 获取昨天的日期
uint8_t htd_rtc_yesterday_day_get(void)
{
    htd_time_t time = htd_rtc_time_get();
    htd_time_t yesterday = htd_time_after_nday_get(&time, -1);
    return yesterday.day;
}

// @brief 显示当前rtc信息
void htd_rtc_info_show(void)
{
    htd_time_t time = htd_rtc_time_get();
    print_debug("RTC info:");
    print_debug("%04d-%02d-%02d %02d:%02d:%02d", time.year, time.month, time.day, time.hour, time.min, time.second);
    
    int32_t timezone = htd_rtc_timezone_get();
    uint32_t timestamps = htd_time2timestamps(&time);
    print_debug("timestamps=%u, timezone=%d", timestamps, timezone);

    uint8_t data[8];
    int32_t data_len;
    for (uint32_t i=0; i<100; ++i) {
        data_len = htd_rtc_bkp_data_get(i, data, sizeof(data));
        if (data_len <= 0) {
            break;
        }
        print_hex("backup data", data, data_len);
    }
}



//////////////////////////////////////////////////////////////////////
#if (defined(HTD_CONSOLE_ENABLE) && (DEBUG_LVL == 4) && 0)
// @brief 调试命令
static int rtc_cmd(int argc, char **argv)
{
    if (argc < 2) {
        print_error("Usage: %s open/close/show", argv[0]);
        return 0;
    }
    
    if (memcmp(argv[1], "show", 5) == 0) {
        htd_rtc_info_show();
        return 0;
    } 

    // 设置时间
    if (memcmp(argv[1], "set", 4) == 0) {
        if (argc < 8) {
            print_error("usage: %s set year month day hour mintue second", argv[0]);
            return 0;
        }
        htd_time_t time;
        time.year = atoi(argv[2]);
        time.month = atoi(argv[3]);
        time.day = atoi(argv[4]);
        time.hour = atoi(argv[5]);
        time.min = atoi(argv[6]);
        time.second = atoi(argv[7]);
        if (0 == htd_rtc_time_set(&time)) {
            print_debug("RTC set success. %04d-%02d-%02d %02d:%02d:%02d", 
                time.year, time.month, time.day, time.hour, time.min, time.second);
        } else {
            print_error("RTC set failed.");
        }
        return 0;
    }
    return 0;
}
CONSOLE_CMD_EXPORT(rtc_cmd, rtc);
#endif /*(defined(HTD_CONSOLE_ENABLE) && (DEBUG_LVL > 0))*/


