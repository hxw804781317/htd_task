#define DEBUG_LVL   4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG   "time"
#include "htd_task.h"
#include "htd_time.h"



/////////////////////////////////////////////////////////////////////////////
#define BASE_YEAR                   (1970)
#define BASE_WEEK                   (4)

const static uint16_t mon_ydays[2][12] = {
    {0,31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334},
    {0,31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335},
};
const static uint8_t mon_day_info[] = {
    0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};


// @brief 计算某年总共多少天
static uint32_t year_day_distance(uint32_t year)
{
    year = year - 1;
    int leap_year = year / 4 - year / 100 + year / 400;
    return year * 365 + leap_year;
}

// @brief 查找某一个月
static uint8_t month_find(bool lpyr, uint32_t days)
{
    for (int i=1; i<12; ++i) {
        if (days < mon_ydays[lpyr][i]) {
            return i;
        }
    }
    return 12;
}

// @brief 时间戳转时间(年月日时分秒)
htd_time_t htd_timestamps2time(uint32_t timestamps, htd_time_t *p_time)
{
    uint32_t days, ydays;
    htd_time_t cdr;

    if (p_time == NULL) {
        p_time = &cdr;
    }
    days = timestamps % ONEDAY_SECONDS;
    p_time->second = days % 60;
    p_time->min = (days % 3600) / 60;
    p_time->hour = (days / 3600);
    days = timestamps / ONEDAY_SECONDS;
    p_time->wday = (days % 7 + BASE_WEEK) % 7;
    p_time->year = BASE_YEAR;
    while (1) {
        ydays = IS_LEAP_YEAR(p_time->year) ? 366 : 365;
        if (days < ydays) {
            break;
        }
        days -= ydays;
        p_time->year++;
    }
    ydays -= 365;
    p_time->month = month_find(ydays, days);
    days -= mon_ydays[ydays][p_time->month-1];
    p_time->day = days + 1;
    return *p_time;
}

// @brief 时间戳转年月日
void htd_timestamps2ymd(uint32_t timestamps, uint16_t* p_year, uint8_t* p_month, uint8_t* p_day)
{
    uint32_t ydays, days = timestamps / ONEDAY_SECONDS;
    uint16_t year;
    uint8_t month;
    uint8_t day;

    year = BASE_YEAR;
    while (1) {
        ydays = IS_LEAP_YEAR(year) ? 366 : 365;
        if (days < ydays) {
            break;
        }
        days -= ydays;
        year++;
    }
    ydays -= 365;
    month = month_find(ydays, days);
    days -= mon_ydays[ydays][month-1];
    day = days + 1;
    
    if (p_year) {
        *p_year = year;
    }
    if (p_month) {
        *p_month = month;
    }
    if (p_day) {
        *p_day = day;
    }
}

// @brief 时间(年月日时分秒)转时间戳
uint32_t htd_time2timestamps(htd_time_t *p_time)
{
    uint32_t days = year_day_distance(p_time->year) - year_day_distance(BASE_YEAR);
    if (IS_LEAP_YEAR(p_time->year)) {
        days += mon_ydays[1][p_time->month-1];
    } else {
        days += mon_ydays[0][p_time->month-1];
    }
    days += p_time->day - 1;
    return days * ONEDAY_SECONDS + (p_time->second + p_time->min*60 + p_time->hour * 60 * 60);
}

// @brief 获取某个月有多少天
uint8_t htd_month_days(uint8_t lpyr, uint8_t mon)
{
    if (mon == 2) {
        return mon_day_info[2] + (!!lpyr);
    }
    return mon_day_info[mon];
}

// @brief 当天起始时间
uint32_t htd_today_first_timestamps(uint32_t timestamps)
{
    return (timestamps - (timestamps % ONEDAY_SECONDS));
}

// @brief 事件戳转星期
uint8_t htd_timestamps2week(uint32_t timestamps)
{
    return ((((timestamps/ONEDAY_SECONDS) % 7) + BASE_WEEK) % 7);
}

// @brief 时间戳转天数
uint32_t htd_timestamps2day(uint32_t timestamps)
{
    return (timestamps / ONEDAY_SECONDS);
}

uint8_t htd_timestamps2hour(uint32_t timestamps)
{
    return ((timestamps % ONEDAY_SECONDS) / 3600);
}

uint8_t htd_timestamps2minute(uint32_t timestamps)
{
    return ((timestamps % (3600)) / 60);
}

static uint32_t start_week_offset_get(uint8_t week_start)
{
    int32_t offset;
    offset = (BASE_WEEK - week_start);
    if (offset <= 0) {
        offset += 7;
    }
    return 7 - offset;
}

// @brief 判断两个时间戳是否为同一个星期之内的
// @param week_start 星期起始日；周天~周六分别为0~6
bool htd_timestamps_is_same_week(uint32_t timestamps1, uint32_t timestamps2, uint8_t week_start)
{
    uint32_t fst = start_week_offset_get(week_start);
    timestamps1 = htd_timestamps2day(timestamps1);
    timestamps2 = htd_timestamps2day(timestamps2);
    if (timestamps1 >= fst && timestamps2 >= fst) {
        timestamps1 -= fst;
        timestamps2 -= fst;
        return ((timestamps1 / 7) == (timestamps2 / 7));
    }
    if (timestamps1 < fst && timestamps2 < fst) {
        return true;
    }
    return false;
}

// @brief 判断两个时间戳是否为同一天
bool htd_timestamps_is_same_day(uint32_t timestamps1, uint32_t timestamps2)
{
    return (htd_timestamps2day(timestamps1) == htd_timestamps2day(timestamps2));
}

// @brief 时间戳(timestamps)是否在今天(sh)时(sm)分到(eh)时(em)分之内
bool htd_timestamps_is_in_range(uint32_t timestamps, uint8_t sh, uint8_t sm, uint8_t eh, uint8_t em)
{
    uint32_t min_index, start_minutes, end_minutes;
    ASSERT(sh < 24);
    ASSERT(sm < 60);
    ASSERT(eh < 24);
    ASSERT(em < 60);
    
    start_minutes = sh * 60 + sm;
    end_minutes = eh * 60  + em;
    if (start_minutes == end_minutes) {
        return true;
    }
    min_index = (timestamps % ONEDAY_SECONDS) / 60;
    print_debug("min_index=%d, start_minutes=%d, end_minutes=%d", min_index, start_minutes, end_minutes);
    if (start_minutes < end_minutes) {
        if (min_index >= start_minutes && min_index < end_minutes) {
            return true;
        }
    } else {
        if (min_index >= start_minutes || min_index < end_minutes) {
            return true;
        }
    }
    return false;
}

// @brief 时间戳(timestamps) 和今天的某时(hour)某分(minute)差距多少秒
uint32_t htd_timestamps_distance(uint32_t timestamps, uint8_t hour, uint8_t minute)
{
    uint32_t dst_timestamps;
    
    ASSERT(hour < 24);
    ASSERT(minute < 60);
    
    timestamps %= ONEDAY_SECONDS;
    dst_timestamps = hour * 3600 + minute * 60;
    if (dst_timestamps > timestamps) {
        return dst_timestamps - timestamps;
    } else {
        return dst_timestamps + ONEDAY_SECONDS - timestamps;
    }
}

// @brief 获取相隔多少天后的日历
htd_time_t htd_time_after_nday_get(const htd_time_t *p_time, int16_t n)
{
    bool is_lpyr;
    htd_time_t next_time;
    htd_memcpy(&next_time, p_time, sizeof(htd_time_t));
    is_lpyr = IS_LEAP_YEAR(next_time.year);
    if (n >= 0) {
        while (n--) {
            next_time.day++;
            if (next_time.day > htd_month_days(is_lpyr, next_time.month)) {
                next_time.day = 1;
                next_time.month++;
                if (next_time.month > 12) {
                    next_time.month = 1;
                    next_time.year++;
                    is_lpyr = IS_LEAP_YEAR(next_time.year);
                }
            }
            next_time.wday++;
            if (next_time.wday >= 7) {
                next_time.wday = 0;
            }
        }
    } else {
        while (n++) {
            next_time.day--;
            if (next_time.day == 0) {
                next_time.month--;
                if (next_time.month == 0) {
                    next_time.year--;
                    is_lpyr = IS_LEAP_YEAR(next_time.year);
                    next_time.month = 12;
                }
                next_time.day = htd_month_days(is_lpyr, next_time.month);
            }
            if (next_time.wday == 0) {
                next_time.wday = 6;
            } else {
                next_time.wday--;
            }
        }
    }
    return next_time;
}

// @brief 日期比较
int htd_date_compare(const htd_time_t *p_fst, const htd_time_t *p_snd) 
{
    if (p_fst->year != p_snd->year) {
        return p_fst->year > p_snd->year ? 1 : -1;
    }
    if (p_fst->month != p_snd->month) {
        return p_fst->month > p_snd->month ? 1 : -1;
    }
    if (p_fst->day != p_snd->day) {
        return p_fst->day > p_snd->day ? 1 : -1;
    }
#if 0
    if (p_fst->hour != p_snd->hour) {
        return p_fst->hour > p_snd->hour ? 1 : -1;
    }
    if (p_fst->minute != p_snd->minute) {
        return p_fst->minute > p_snd->minute ? 1 : -1;
    }
#endif
    return 0;
}

