// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL       4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG       "task"
#include "htd_task.h"


#define ALIGNED_LEN         sizeof(uintptr_t)

// @brief memset
void *__htd_memset(void *s, int c, size_t count)
{
#if HTD_MEM_USE_BYTELOOP
    uint8_t* p_dst = s;
    while (count--) {
        *p_dst = c;
    }
    return s;
#else
    unsigned int d = c & 0xff; 
    char *m = (char*)s;
    uint32_t front = (((uintptr_t)s) & (ALIGNED_LEN - 1));
    if (front) {
        front = ALIGNED_LEN - front;
        if (front > count) {
            front = count;
        }
        count -= front;
        while (front--) {
            *m++ = (char)d;
        }
    }
    
    if (count >= ALIGNED_LEN) {
        uintptr_t buffer;
        uintptr_t *p_ptr = (uintptr_t*)m;
        if (ALIGNED_LEN == 2) {
            buffer = (d << 8) | d;
        } else if (ALIGNED_LEN == 4) {
            buffer = (d << 8) | d;
            buffer |= (buffer << 16);
        } else {
            buffer = 0;
            for (int i=0; i<ALIGNED_LEN; i++) {
                buffer = (buffer << 8) | d;
            }
        }
        while (count >= ALIGNED_LEN * 4) {
            *p_ptr++ = buffer;
            *p_ptr++ = buffer;
            *p_ptr++ = buffer;
            *p_ptr++ = buffer;
            count -= 4 * ALIGNED_LEN;
        }
        while (count >= ALIGNED_LEN) {
            *p_ptr++ = buffer;
            count -= ALIGNED_LEN;
        }
        m = (char*)p_ptr;
    }
    
    while (count--) {
        *m++ = (char)d;
    }
    return s;
#endif
}

// @brief 内存拷贝 memcpy
void *__htd_memcpy(void *dst, const void *src, size_t count)
{
#if HTD_MEM_USE_BYTELOOP
    uint8_t* p_dst = dst;
    uint8_t* p_src = src;
    while (count--) {
        *p_dst++ = p_src++;
    }
    return dst;
#else
    char* p8_dst = (char*)dst;
    const char* p8_src = (const char*)src;
    uint32_t front = (((uintptr_t)src) & (ALIGNED_LEN - 1));
    if ((((uintptr_t)dst) & (ALIGNED_LEN - 1)) == front && count >= ALIGNED_LEN) {
        if (front) {
            front = ALIGNED_LEN - front;
            if (front > count) {
                front = count;
            }
            count -= front;
            while (front--) {
                *p8_dst++ = *p8_src++;
            }
        }
        
        uintptr_t *p_dst = (uintptr_t*)p8_dst;
        const uintptr_t *p_src = (const uintptr_t*)p8_src;
        while (count >= ALIGNED_LEN * 4) {
            *p_dst++ = *p_src++;
            *p_dst++ = *p_src++;
            *p_dst++ = *p_src++;
            *p_dst++ = *p_src++;
            count -= ALIGNED_LEN * 4;
        }
        while (count >= ALIGNED_LEN) {
            *p_dst++ = *p_src++;
            count -= ALIGNED_LEN;
        }
        p8_dst = (char*)p_dst;
        p8_src = (const char*)p_src;
    }

    while (count--) {
        *p8_dst++ = *p8_src++;
    }
    return dst;
#endif
}

// @brief memmove
void *__htd_memmove(void *dst, const void *src, size_t n)
{
    char *p_dst = (char*)dst;
    char *p_src = (char*)src;
    if (p_src < p_dst && p_dst < p_src + n) {
        p_dst += n;
        p_src += n;
        while (n--) {
            *(--p_dst) = *(--p_src);
        }
        return dst;
    } 
    return htd_memcpy(dst, src, n);
}

// @brief 内存比较
int32_t __htd_memcmp(const void *v1, const void *v2, size_t n)
{
    const uint8_t *p1 = v1;
    const uint8_t *p2 = v2;
    while (n--) {
        if (*p1 != *p2) {
            return (*p1 - *p2);
        }
        p1++, p2++;
    }
    return 0;
}

// @brief 获取字符串长度 strlen
size_t __htd_strlen(const char *p_str)
{
    const char *p_start = p_str;
    while (*p_str) {
        p_str++;
    }
    return p_str - p_start;
}

// @brief 获取字符串长度 strnlen
size_t __htd_strnlen(const char *p_str, size_t max_len)
{
    const char *p_start = p_str;
    while (max_len && *p_str) {
        p_str++;
        max_len--;
    }
    return p_str - p_start;
}

// @brief 拷贝字符串 strcpy
char *__htd_strcpy(char *dst, const char *src)
{
    char* p_dst = dst;
    while ((*p_dst++ = *src++) != '\0');
    return dst;
}

// @brief 拷贝字符串 strncpy
char *__htd_strncpy(char *dst, const char *src, size_t n)
{
    char* p_dst = dst;
    while (n && *src) {
        n--;
        *p_dst++ = *src++;
    }
    while (n--) {
        *p_dst++ = 0;
    }
    return dst;
}

// @brief 字符串比较
int32_t __htd_strcmp(const char *v1, const char *v2)
{
    while (*v1 && *v1 == *v2) {
        v1++, v2++;
    }
    return (*v1 - *v2);
}

// @brief 字符串对比，忽略字母大小写
uint32_t __htd_strcasecmp(const char *v1, const char *v2)
{
    char byte_v1, byte_v2;
    do {
        byte_v1 = *v1++;
        byte_v2 = *v2++;
        if (byte_v1 >= 'A' && byte_v1 <= 'Z') {
            byte_v1 += ('a' - 'A');
        }
        if (byte_v2 >= 'A' && byte_v2 <= 'Z') {
            byte_v2 += ('a' - 'A');
        }
    } while (byte_v1 == byte_v2 && byte_v1 != '\0');
    return byte_v1 - byte_v2;
}

// @brief 字符串查找
char *__htd_strstr(const char *v1, const char *v2)
{
    int v2_len = htd_strlen(v2);
    if (!v2_len) {
        return (char *)v1;
    }
    
    int v1_len = htd_strlen(v1);
    while (v1_len >= v2_len) {
        v1_len --;
        if (!htd_memcmp(v1, v2, v2_len)) {
            return (char *)v1;
        }
        v1 ++;
    }
    return NULL;
}

// @brief 判断内存每个字节是否都是 val
bool htd_mem_allof(const uint8_t *dst, uint8_t val, uint16_t n)
{
    while (n--) {
        if (*dst++ != val) {
            return false;
        }
    }
    return true;
}





//////////////////////////////////////////////////////////////////////
#if (!defined(HTD_TASK_NONOS)) && defined(HTD_CONSOLE_ENABLE) && 0
#include "htd_console_process.h"

/* 互斥锁稳定性测试
 * 测试目的：
 *  1. 多任务并发访问共享资源时，数据一致性得到保证。
 *  2. 高优先级任务不会因低优先级任务持有互斥锁而无限期阻塞（优先级继承）。
 *  3. 系统长时间运行无死锁、无数据损坏。
 *
 * 测试场景：
 *  - 创建三个任务：高、中、低优先级。
 *  - 共享资源：一个全局计数器和一个缓冲区（模拟关键数据）。
 *  - 低优先级任务获取互斥锁后长时间占用（模拟耗时操作）。
 *  - 中优先级任务不使用互斥锁，但会频繁抢占（模拟无关的高 CPU 负载）。
 *  - 高优先级任务需要获取互斥锁来更新共享数据。
 *  - 若互斥锁支持优先级继承，高优先级任务会在低优先级任务释放锁后立即执行，
 *    否则高优先级任务会被中优先级任务无限阻塞。
 *
 * 输出信息：
 *  - 每个任务的操作计数。
 *  - 检测到的数据错误（如果有）。
 *  - 高优先级任务的等待时间（用于验证优先级继承）。
 */

/* ==================== 配置参数 ==================== */
/* 共享资源 */
static volatile uint32_t shared_counter = 0;
static volatile uint8_t  shared_buffer[128];
static htd_mutex_lock_t* m_mutex_lock = NULL;

/* 统计信息 */
static volatile uint32_t high_task_count = 0;
static volatile uint32_t low_task_count  = 0;
static volatile uint32_t mid_task_count  = 0;
static volatile uint32_t data_error_count = 0;   /* 数据不一致错误计数 */
static volatile uint32_t high_task_wait_time_ms = 0;  /* 高优先级任务等待时间（微秒，需硬件定时器） */


/* 中优先级任务：不使用互斥锁，仅占用 CPU 模拟高负载 */
static void medium_priority_task(void *pvParameters)
{
    print_debug("---medium_priority_task---");
    for (;;) {
        /* 模拟耗时计算（忙循环） */
        for (uint32_t i = 0; i < 1000000; i++) {
            /* 空操作，消耗 CPU 时间 */
        }
        mid_task_count++;
        htd_sleep_ms(200);
        print_debug("---medium_priority_task---");
    }
}

/* 低优先级任务：获取互斥锁并长时间占用 */
static void low_priority_task(void *pvParameters)
{
    print_debug("---low_priority_task---");
    for (;;) {
        htd_mutex_lock(m_mutex_lock);
        htd_sleep_ms(1000);    /* 模拟长时间操作（保持锁） */
        htd_mutex_unlock(m_mutex_lock);

        low_task_count++;
        htd_sleep_ms(10);                       /* 短暂延时，避免连续占用 */
        print_debug("---low_priority_task---");
    }
}

/* 监控任务：定期打印统计信息，检测数据错误 */
static void monitor_task(void *pvParameters)
{
    for (;;) {
        htd_sleep_ms(5000);
        htd_print("\n========== OS thread test ==========\n");
        htd_print("high_task_count: %lu\n", high_task_count);
        htd_print("mid_task_count: %lu\n", mid_task_count);
        htd_print("low_task_count: %lu\n", low_task_count);
        htd_print("data_error_count: %lu\n", data_error_count);
        htd_print("high_task_wait_time_ms: %lu us\n", high_task_wait_time_ms);
        htd_print("shared_counter: %lu\n", shared_counter);
        htd_print("====================================================\n");

        /* 如果出现数据错误，可以通过断言或 LED 指示（此处仅打印） */
        if (data_error_count > 0) {
            print_error("警告：检测到数据不一致！互斥锁可能未正确保护共享资源！\n");
        }
    }
}

/* 高优先级任务：需要互斥锁更新共享数据 */
static void high_priority_task(void *pvParameters)
{
    uint32_t start_time_ms;
    uint32_t local_counter;
    
    for (;;) {
        print_debug("---high_priority_task---");
        /* 记录开始等待的时间 */
        start_time_ms = htd_time_ms_get();
        htd_mutex_lock(m_mutex_lock); 
        high_task_wait_time_ms = htd_time_ms_diff(htd_time_ms_get(), start_time_ms);
        local_counter = shared_counter;
        local_counter++;   /* 增加计数器 */
        shared_counter = local_counter;

        /* 模拟更新缓冲区（简单异或） */
        for (int i = 0; i < sizeof(shared_buffer); i++) {
            shared_buffer[i] ^= 0x55;
        }

        /* 验证数据一致性（可选：读取回并与期望比较，但这里不做复杂校验） */
        /* 简单检查：缓冲区所有字节是否相同（此处仅作演示） */
        uint8_t first = shared_buffer[0];
        for (int i = 1; i < sizeof(shared_buffer); i++) {
            if (shared_buffer[i] != first) {
                data_error_count++;   /* 检测到数据不一致 */
                break;
            }
        }
        high_task_count++;
        htd_mutex_unlock(m_mutex_lock);

        /* 任务休眠一小段时间，避免无限循环占用 CPU */
        htd_sleep_ms(20);
    }
}


// @brief 线程测试
int32_t thread_test_handle(int argc, char **argv)
{
    /* 创建互斥锁 */
    m_mutex_lock = htd_mutex_lock_create("mutex");
    if (m_mutex_lock == NULL) {
        htd_print("互斥锁创建失败！\n");
        return 0;
    }

    htd_thread_t thread_handle;

    // freeRTOS 数值越高，优先级越高
    #define TEST_TASK_HIGH_PRIO       (tskIDLE_PRIORITY + 3)
    #define TEST_TASK_MID_PRIO        (tskIDLE_PRIORITY + 2)
    #define TEST_TASK_LOW_PRIO        (tskIDLE_PRIORITY + 1)
    thread_handle = htd_thread_create("High-Task", high_priority_task, NULL, 1024, TEST_TASK_HIGH_PRIO, 0);
    ASSERT(thread_handle);
    thread_handle = htd_thread_create("Mid-Task", medium_priority_task, NULL, 1024, TEST_TASK_MID_PRIO, 0);
    ASSERT(thread_handle);
    thread_handle = htd_thread_create("Low-Task", low_priority_task, NULL, 1024, TEST_TASK_LOW_PRIO, 0);
    ASSERT(thread_handle);
    thread_handle = htd_thread_create("Monitor", monitor_task, NULL, 1024, tskIDLE_PRIORITY+1, 0);
    ASSERT(thread_handle);
    return 0;
}
CONSOLE_CMD_EXPORT(thread_test_handle, thread_test);
#endif





