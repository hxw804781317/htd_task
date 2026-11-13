#ifndef _HTD_TIME_H_
#define _HTD_TIME_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

// 获取编译时的年月日，时分秒
#define __YEAR__    ((((__DATE__ [7] - '0') * 10 + (__DATE__ [8] - '0')) * 10 + (__DATE__ [9] - '0')) * 10 + (__DATE__ [10] - '0'))
#define __MONTH__   ( __DATE__ [2] == 'n' ? (__DATE__ [1] == 'a' ? 1 : 6) \
                    : __DATE__ [2] == 'b' ? 2 \
                    : __DATE__ [2] == 'r' ? (__DATE__ [0] == 'M' ? 3 : 4) \
                    : __DATE__ [2] == 'y' ? 5 \
                    : __DATE__ [2] == 'n' ? 6 \
                    : __DATE__ [2] == 'l' ? 7 \
                    : __DATE__ [2] == 'g' ? 8 \
                    : __DATE__ [2] == 'p' ? 9 \
                    : __DATE__ [2] == 't' ? 10 \
                    : __DATE__ [2] == 'v' ? 11 : 12)
#define __DAY__     ((__DATE__ [4] == ' ' ? 0 : ((__DATE__ [4] - '0') * 10 )) + (__DATE__ [5] - '0'))
#define __HOUR__    ((__TIME__ [0] == ' ' ? 0 : ((__TIME__ [0] - '0') * 10 )) + (__TIME__ [1] - '0'))
#define __MINUTE__  ((__TIME__ [3] == ' ' ? 0 : ((__TIME__ [3] - '0') * 10 )) + (__TIME__ [4] - '0'))
#define __SECOND__  ((__TIME__ [6] == ' ' ? 0 : ((__TIME__ [6] - '0') * 10 )) + (__TIME__ [7] - '0'))

// 是否为闰年
#ifndef IS_LEAP_YEAR
    #define IS_LEAP_YEAR(yr)            (!((yr) % 400) || (((yr) % 100) && !((yr) % 4)))
#endif
// 一天时间秒数
#ifndef ONEDAY_SECONDS
    #define ONEDAY_SECONDS              (24*60*60)
#endif /*ONEDAY_SECONDS*/

typedef struct {
    uint32_t year:12;       // BASE_YEAR+
    uint32_t month:4;       // 1-12
    uint32_t day:5;         // 1-31
    uint32_t hour:5;        // 0-23
    uint32_t min:6;         // 0-59
    
    uint8_t second;         // 0-59
    uint8_t wday;           // 1,2,3,4,5,6,0
    //uint16_t ms;            // 毫秒
} htd_time_t;


// @brief 获取某个月有多少天
uint8_t htd_month_days(uint8_t lpyr, uint8_t mon);
// @brief 时间戳转时间(年月日时分秒)
htd_time_t htd_timestamps2time(uint32_t timestamps, htd_time_t* p_time);
// @brief 时间(年月日时分秒)转时间戳
uint32_t htd_time2timestamps(htd_time_t *p_time);
// @brief 时间戳转年月日
void htd_timestamps2ymd(uint32_t timestamps, uint16_t* p_year, uint8_t* p_month, uint8_t* p_day);
// @brief 当天起始时间
uint32_t htd_today_first_timestamps(uint32_t timestamps);


// @brief 时间戳转星期
uint8_t htd_timestamps2week(uint32_t timestamps);
// @brief 时间戳转天数
uint32_t htd_timestamps2day(uint32_t timestamps);
// @brief 通过时间戳获取当前几点
uint8_t htd_timestamps2hour(uint32_t timestamps);
// @brief 通过时间戳获取当前第几分钟
uint8_t htd_timestamps2minute(uint32_t timestamps);


// @brief 时间戳(timestamps)是否在今天(sh)时(sm)分到(eh)时(em)分之内
bool htd_timestamps_is_in_range(uint32_t timestamps, uint8_t sh, uint8_t sm, uint8_t eh, uint8_t em);
// @brief 时间戳(timestamps) 和今天的某时(hour)某分(minute)差距多少秒
uint32_t htd_timestamps_distance(uint32_t timestamps, uint8_t hour, uint8_t minute);
// @brief 获取相隔多少天后的日历
htd_time_t htd_time_after_nday_get(const htd_time_t *p_time, int16_t n);
// @brief 时间对比(精度到天)
int htd_date_compare(const htd_time_t *p_fst, const htd_time_t *p_snd);


// @brief 判断两个时间戳是否为同一个星期之内的
// @param week_start 星期起始日；周天~周六分别为0~6
bool htd_timestamps_is_same_week(uint32_t timestamps1, uint32_t timestamps2, uint8_t week_start);
// @brief 判断两个时间戳是否为同一天
bool htd_timestamps_is_same_day(uint32_t timestamps1, uint32_t timestamps2);


#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_TIME_H_*/

