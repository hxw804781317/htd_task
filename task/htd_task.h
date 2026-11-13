// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_TASK_H_
#define _HTD_TASK_H_

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "htd_config.h"
#include "htd_compiler.h"
#include "htd_section.h"
#include "htd_fifo.h"
#include "htd_slist.h"
#include "htd_list.h"
#include "htd_log.h"
#include "htd_base16.h"
#include "htd_base32.h"
#include "htd_base64.h"
#include "htd_bitmap.h"
#include "htd_time.h"
#include "htd_printf_fmt.h"

#include "htd_task_port.h"
#include "htd_task_sch.h"
#include "htd_task_thread_sch.h"


#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/



//////////////////////////////////////////////////////////////////
#if (!defined(ASSERT))
    #ifdef VER_RELEASE
        #define ASSERT(ex)
    #elif (defined(DEBUG_LVL)) && (DEBUG_LVL < 3)
        #define ASSERT(ex)
    #else
        extern void htd_assert_failed(const char* file, uint32_t line);
        #define ASSERT(expr) ((expr) ? (void)0U : htd_assert_failed(__FILE__, __LINE__))
    #endif
#endif


//////////////////////////////////////////////////////////////////////
// 判断是否在中断函数
#ifndef IS_IRQ
    #if defined(ARCH_ARM_CORTEX_M)
        #define IS_IRQ()                (__get_IPSR() != 0U)
    #endif
#endif
#ifndef STR
    #define STR(s)                      #s
#endif 
#ifndef STRINGIFY
    #define STRINGIFY(val)              STR(val)
#endif 
// 向下对齐
#ifndef ALIGN_DOWN
    #define ALIGN_DOWN(n, align)        ((n) & ~((align) - 1))
#endif 
// 向上对齐
#ifndef ALIGN_UP
    #define ALIGN_UP(n, align)          (((n) + (align) - 1) & ~((align) - 1))
#endif 
// 判断是否为2的指数倍数
#ifndef IS_POWER_OF_TWO
    #define IS_POWER_OF_TWO(A)          (((A) != 0) && ((((A) - 1) & (A)) == 0))
#endif
// 数组成员个数
#ifndef ARRAY_COUNT
    #define ARRAY_COUNT(X)              (sizeof(X) / sizeof(X[0]))
#endif
// 拼接两个宏
#ifndef CONCAT_2
    #define CONCAT_2_(p1, p2)           p1##p2
    #define CONCAT_2(p1, p2)            CONCAT_2_(p1, p2)
#endif
// 拼接三个宏
#ifndef CONCAT_3
    #define CONCAT_3_(p1, p2, p3)       p1##p2##p3
    #define CONCAT_3(p1, p2, p3)        CONCAT_3_(p1, p2, p3)
#endif
// 最小值
#ifndef MIN
    #define MIN(x,y) (((x)<(y))?(x):(y))
#endif
// 最大值
#ifndef MAX
    #define MAX(x,y) (((x)>(y))?(x):(y))
#endif
// 绝对值
#ifndef ABS
    #define ABS(x) (((x)>=0)?(x):-(x))
#endif
// 指向结构体成员
#ifndef OFFSETOF
    #if defined(__SES_ARM) && defined(__GNUC__)
        #define OFFSETOF(T, x)          __builtin_offsetof(T, x)
    #else
        #define OFFSETOF(T, x)          ((size_t) &((T *)0)->x)
    #endif
#endif
// 结构体成员偏移字节
#ifndef CONTAINER_OF
    #define CONTAINER_OF(ptr, type, member)         ((type *)((char *)(ptr) - (unsigned long)(&((type *)0)->member)))
#endif
// BIT位
#ifndef __BIT
    #define __BIT(x)                    (1uL<<(x))          // 位
#endif
#ifndef __BITS
    #define __BITS(start, end)          ((__BIT(end) - __BIT(start)) + __BIT(end))
#endif
#ifndef __MASK
    #define __MASK(x)                   __BIT(x)            // 屏蔽位(选择的位为1,其它为0)
#endif
#ifndef __UNMASK
    #define __UNMASK(x)                 (~(__BIT(x)))       // 反屏蔽位(选择的位为0,其它为1)
#endif
#ifndef __GET_BIT
    #define __GET_BIT(x,bit)            ((x) & __MASK(bit))
#endif
#ifndef __SET_BIT
    #define __SET_BIT(x,bit)            ((x) = (x) | __MASK(bit))
#endif
#ifndef __CLR_BIT
    #define __CLR_BIT(x,bit)            ((x) = (x) & __UNMASK(bit))
#endif
#ifndef __SET_BITS
    #define __SET_BITS(reg, bits)       (reg) |= (bits)
#endif
#ifndef __CLR_BITS
    #define __CLR_BITS(reg, bits)       (reg) &= ~(bits)
#endif
#ifndef __GET_BITS
    #define __GET_BITS(reg, bits)       (reg & (bits))
#endif
// 指针数值切换
#ifndef UINT8_VAL
    #define UINT8_VAL(v)                (*(uint8_t*)(v))
#endif
#ifndef UINT16_VAL
    #define UINT16_VAL(v)               (*(uint16_t*)(v))
#endif
#ifndef INT16_VAL
    #define INT16_VAL(v)                (*(int16_t*)(v))
#endif 
#ifndef UINT32_VAL
    #define UINT32_VAL(v)               (*(uint32_t*)(v))
#endif
#ifndef INT32_VAL
    #define INT32_VAL(v)                (*(int32_t*)(v))
#endif 


// 字符c在某个范围之内
#define htd_in_range(c, lo, up)         ((uint8_t)(c) >= (lo) && (uint8_t)(c) <= (up))
// 是否为数字
#define htd_isdigit(c)                  htd_in_range((c), '0', '9')
// 是否为数字或者字母
#define htd_isxdigit(c)                 (htd_isdigit(c) || htd_in_range((c), 'a', 'f') || htd_in_range((c), 'A', 'F'))
// 是否为小写字母
#define htd_islower(c)                  htd_in_range((c), 'a', 'z')
// 是否为大写字母
#define htd_isupper(c)                  htd_in_range((c), 'A', 'Z')
// 是否为空格
#define htd_isspace(c)                  ((c) == ' ' || (c) == '\f' || (c) == '\n' || (c) == '\r' || (c) == '\t' || (c) == '\v')
// 大写转小写
#define htd_tolower(c)                  (htd_isupper(c) ? (c) - 'A' + 'a' : c)
// 小写转大写
#define htd_toupper(c)                  (htd_islower(c) ? (c) - 'a' + 'A' : c)



//////////////////////////////////////////////////////////////////////
// 错误码
#define E_SUCCESS                         (0)
#define E_FAILED                          (-1)
#define E_NO_MEM                          (-2)
#define E_INVALID_ARG                     (-3)
#define E_NOT_FOUND                       (-4)
#define E_INVALID_DATA                    (-5)
#define E_BUSY                            (-6)
#define E_NOT_SUPPORTED                   (-7)
#define E_CHECKSUM_FAILED                 (-8)

#define ERROR_CODE_SUCCESS                 0                                  // 成功
#define ERROR_CODE_NON                     ERROR_CODE_SUCCESS                 // 成功
#define ERROR_CODE_INVALID_PARAM           (ERROR_CODE_NON+1)                 // 参数错误
#define ERROR_CODE_NOT_SUPPORTED           (ERROR_CODE_INVALID_PARAM+1)       // 不支持该指令
#define ERROR_CODE_NO_MEM                  (ERROR_CODE_NOT_SUPPORTED+1)       // 内存空间不够
#define ERROR_CODE_CHECKSUM_FAILED         (ERROR_CODE_NO_MEM+1)              // 数据包校验码错误
#define ERROR_CODE_NULL                    (ERROR_CODE_CHECKSUM_FAILED+1)     // 数据内容为空
#define ERROR_CODE_BUSY                    (ERROR_CODE_NULL+1)                // 当前忙
#define ERROR_CODE_TIMEOUT                 (ERROR_CODE_BUSY+1)                // 超时
#define ERROR_CODE_ACK_ERROR               (ERROR_CODE_TIMEOUT+1)             // 应答数据错误
#define ERROR_CODE_FAILED                  (ERROR_CODE_ACK_ERROR+1)           // 失败
#define ERROR_CODE_INVALID_STATE           (ERROR_CODE_FAILED+1)              // 状态错误
#define ERROR_CODE_COMPLETED               (ERROR_CODE_INVALID_STATE+1)       // 已经完成
#define ERROR_CODE_NOT_FOUND               (ERROR_CODE_COMPLETED+1)           // 查找不到
#define ERROR_CODE_INVALID_LENGTH          (ERROR_CODE_NOT_FOUND+1)           // 长度错误
#define ERROR_CODE_INVALID_FLAGS           (ERROR_CODE_INVALID_LENGTH+1)      // 无效的标记位
#define ERROR_CODE_INVALID_DATA            (ERROR_CODE_INVALID_FLAGS+1)       // 数据无效
#define ERROR_CODE_DATA_SIZE               (ERROR_CODE_INVALID_DATA+1)        // 数据大小错误
#define ERROR_CODE_INVALID_ADDR            (ERROR_CODE_DATA_SIZE+1)           // 错误的地址
#define ERROR_CODE_NO_SPACE                (ERROR_CODE_INVALID_ADDR+1)        // 空间不足
#define ERROR_CODE_INVALID_FILE_NAME       (ERROR_CODE_NO_SPACE+1)            // 无效的文件名字
#define ERROR_CODE_LOW_BATTERY             (ERROR_CODE_INVALID_FILE_NAME+1)   // 电量低
#define ERROR_CODE_INTERVAL                (ERROR_CODE_LOW_BATTERY+1)         // 连接间隔错误






//////////////////////////////////////////////////////////////////////
// @brief 线程睡眠
void htd_sleep_ms(uint32_t ms);
// @brief 获取系统时间，单位毫秒(中断中调用)
uint32_t htd_time_ms_get_fisr(void);
// @brief 修正系统 ticks
void htd_task_step_tick(uint32_t sleep_ms);

// @brief 毫秒相减
__STATIC_INLINE uint32_t htd_time_ms_sub(uint32_t ms, uint32_t sub_ms)
{
    return (ms - sub_ms);
}
// @brief 毫秒相加
__STATIC_INLINE uint32_t htd_time_ms_add(uint32_t ms, uint32_t sub_ms)
{
    return (ms + sub_ms);
}
// @brief 时间间隔
__STATIC_INLINE uint32_t htd_time_ms_diff(uint32_t now_ms, uint32_t prev_ms)
{
    return ((now_ms - prev_ms) & 0xffffffff);
}

// @brief 数据距离
__STATIC_INLINE uint32_t data_diff(uint32_t now, uint32_t pre, uint32_t mask)
{
    return ((now - pre) & mask);
}
// @brief 数据增长
__STATIC_INLINE uint32_t data_increase(uint32_t data, uint32_t mask)
{
    data++;
    if (data == mask) {
        data = 0;
    }
    return data;
}
// @brief 数据递减
__STATIC_INLINE uint32_t data_decrease(uint32_t data, uint32_t mask)
{
    if (data == 0) {
        data = mask - 1;
    } else {
        data--;
    }
    return data;
}

// @brief 哈希
__STATIC_INLINE uint32_t hash_sdbm(const uint8_t *p_buff, uint32_t buff_len)
{
    uint32_t hash = 0;
    while (buff_len--) {
        hash = (*p_buff++) + (hash << 6) + (hash << 16) - hash;
    }
    return hash;
}
// @brief 计算绝对值
__STATIC_INLINE int __abs(int x)
{
    return (x ^ (x >> 31)) - (x >> 31);
}





//////////////////////////////////////////////////////////////////////
// @brief memset
void *htd_memset(void *s, int c, size_t count);
// @brief 内存拷贝 memcpy
void *htd_memcpy(void *dst, const void *src, size_t count);
// @brief memmove
void *htd_memmove(void *dst, const void *src, size_t n);
// @brief 内存比较
int32_t htd_memcmp(const void *v1, const void *v2, size_t n);
// @brief 获取字符串长度 strlen
size_t htd_strlen(const char *p_str);
// @brief 获取字符串长度 strnlen
size_t htd_strnlen(const char *p_str, size_t max_len);
// @brief 拷贝字符串 strcpy
char *htd_strcpy(char *dst, const char *src);
// @brief 拷贝字符串 strncpy
char *htd_strncpy(char *dst, const char *src, size_t n);
// @brief 字符串比较
int32_t htd_strcmp(const char *v1, const char *v2);
// @brief 字符串对比，忽略字母大小写
uint32_t htd_strcasecmp(const char *v1, const char *v2);
// @brief 字符串查找
char *htd_strstr(const char *v1, const char *v2);
// @brief 判断内存每个字节是否都是 val
bool htd_mem_allof(const uint8_t *dst, uint8_t val, uint16_t n);






#ifdef __cplusplus
}
#endif /*__cplusplus*/
#include "htd_task_nos.h"
#include "htd_task_freertos.h"
#include "htd_task_win32_sdl2.h"
#include "htd_memheap_sram_dev.h"
#include "htd_memheap_ff.h"
#endif /*_HTD_TASK_H_*/

