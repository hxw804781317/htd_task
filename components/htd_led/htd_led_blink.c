// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL       2       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG       "led"
#include "htd_led_blink.h"
#include "htd_led_list.h"


//////////////////////////////////////////////////////////////////////
// 定义 led 数据类型
typedef struct htd_led_s htd_led_t;
// 控制句柄
typedef void (*htd_led_ctrl_handle_t)(htd_led_t* led);

// 周期闪烁
typedef struct {
    uint16_t period_count;      // 多少个周期
    uint16_t count;
    uint8_t is_onoff;
} led_blink_period_param_t;

// 自定义闪烁
typedef struct {
    uint16_t *ms_array;
    uint16_t ms_array_count;
    uint16_t count;
    uint8_t is_onoff;
} led_blink_custom_param_t;

// 定义一个灯
#if (defined ( __CC_ARM ) || defined(__CLANG_ARM))
#pragma anon_unions
#endif
struct htd_led_s{
    htd_list_node_t node;
    uint8_t idx;
    uint8_t brightness;                             // 当前亮度，0~100
    uint32_t timeout_ms;
    uint32_t start_ms;
    htd_led_ctrl_handle_t ctrl_handle;              // 不同类型对应不同的控制句柄
    union {
        led_blink_period_param_t period_blink;      // 周期闪烁
        led_blink_custom_param_t custom_blink;      // 自定义闪烁
    };
};

typedef struct {
    htd_list_node_t list;
    htd_led_onoff_func_t onoff_func;
    void* p_lock;
}led_ctrl_t;
static led_ctrl_t m_led_ctrl = {0};


// @brief 初始化
// @param count 控制的 led 数量
int32_t htd_led_init(htd_led_onoff_func_t onoff_handle, uint32_t count)
{
    ASSERT(count > 0);
    ASSERT(onoff_handle);
    ASSERT(m_led_ctrl.onoff_func == NULL);
    
    htd_memset(&m_led_ctrl, 0, sizeof(led_ctrl_t));
    htd_list_head_init(&m_led_ctrl.list);
    m_led_ctrl.p_lock = led_lock_create("htd-led");
    ASSERT(m_led_ctrl.p_lock);
    m_led_ctrl.onoff_func = onoff_handle;

    uint32_t i;
    htd_led_t *p_leds = htd_malloc(sizeof(htd_led_t) * count);
    ASSERT(p_leds);
    htd_memset(p_leds, 0, sizeof(htd_led_t) * count);
    for (i=0; i<count; ++i) {
        p_leds[i].idx = i;
        htd_list_insert_before(&p_leds[i].node, &m_led_ctrl.list);
        m_led_ctrl.onoff_func(i, 0);
    }
    return 0;
}

// @brief 定时控制句柄
static void led_timer_ctrl_handle(void* arg)
{
    led_ctrl_t* p_ctrl = (led_ctrl_t*)arg;
    if (p_ctrl == NULL) {
        return;
    }
    
    led_lock(p_ctrl->p_lock);
    htd_list_node_t* p_first = p_ctrl->list.next;
    htd_led_t* p_info = htd_list_entry(p_first, htd_led_t, node);
    if (p_info->timeout_ms == 0 || p_info->ctrl_handle == NULL) {
        led_unlock(p_ctrl->p_lock);
        return;
    }

    // 执行控制，控制里面会将节点从链表移除，并重新添加进链表
    uint32_t now_ms = TIME_MS_GET();
    uint32_t timeout_ms = TIME_MS_DIFF(now_ms, p_info->start_ms);
    if (timeout_ms >= p_info->timeout_ms) {
        p_info->ctrl_handle(p_info);
    }
    
    // 重新获取新的首节点
    p_first = p_ctrl->list.next;
    p_info = htd_list_entry(p_first, htd_led_t, node);
    if (p_info->ctrl_handle) {
        timeout_ms = TIME_MS_DIFF(now_ms, p_info->start_ms);
        led_load_timer_func(false, timeout_ms, led_timer_ctrl_handle, arg);
    }
    led_unlock(p_ctrl->p_lock);
}

// @brief 添加到对应的链表中
static void led_node_insert(led_ctrl_t* p_ctrl, htd_led_t* p_led)
{
    htd_led_t* p_info;
    htd_list_node_t *pos;
    uint32_t timeout_ms, now_ms;
    
    now_ms = TIME_MS_GET();
    htd_list_for_each(pos, &p_ctrl->list) {
        p_info = htd_list_entry(pos, htd_led_t, node);
        if (p_info->ctrl_handle == NULL) {
            continue;
        }
        timeout_ms = TIME_MS_DIFF(now_ms, p_info->start_ms);
        if (p_led->timeout_ms < timeout_ms) {
            htd_list_insert_before(&p_led->node, &p_info->node);
            break;
        }
    }
    // 启动
    if (p_info->ctrl_handle) {
        p_info = htd_list_entry(p_ctrl->list.next, htd_led_t, node);
        timeout_ms = TIME_MS_DIFF(now_ms, p_info->start_ms);
        led_load_timer_func(false, timeout_ms, led_timer_ctrl_handle, p_ctrl);
    }
}

// @brief 查找对应的led设备
static htd_led_t* led_node_find(led_ctrl_t* p_ctrl, uint32_t idx)
{
    htd_led_t* p_info;
    htd_list_node_t *pos;
    htd_list_for_each(pos, &p_ctrl->list) {
        p_info = htd_list_entry(pos, htd_led_t, node);
        if (p_info->idx == idx) {
            return p_info;
        }
    }
    return NULL;
}

// @brief 打印所有led信息
void htd_led_dump(void)
{
#if defined(DEBUG_LVL) && (DEBUG_LVL == 4)
    htd_led_t* p_info;
    htd_list_node_t *pos;
    htd_list_for_each(pos, &m_led_ctrl.list) {
        p_info = htd_list_entry(pos, htd_led_t, node);
        print_log("idx=%d, brightness=%d, timeout_ms=%ld", p_info->idx, p_info->brightness, p_info->timeout_ms);
    }
#endif
}

// @brief 亮灯/灭灯
int32_t htd_led_onoff(uint32_t idx, uint8_t onoff)
{
    htd_led_t* p_led;
    led_lock(m_led_ctrl.p_lock);
    p_led = led_node_find(&m_led_ctrl, idx);
    if (p_led) {
        p_led->ctrl_handle = NULL;
        if (onoff) {
            p_led->brightness = 100;
            m_led_ctrl.onoff_func(idx, 100);
        } else {
            p_led->brightness = 0;
            m_led_ctrl.onoff_func(idx, 0);
        }
    }
    led_unlock(m_led_ctrl.p_lock);
    return 0;
}

// @brief 判断灯光是否为亮
bool htd_led_is_on(uint32_t idx)
{
    uint8_t brightness = 0;
    htd_led_t* p_led;
    led_lock(m_led_ctrl.p_lock);
    p_led = led_node_find(&m_led_ctrl, idx);
    if (p_led) {
        brightness = p_led->brightness;
    }
    led_unlock(m_led_ctrl.p_lock);
    return (brightness == 0 ? false : true);
}

// @brief 延迟控制亮
static void led_on_handle(htd_led_t* p_led)
{
    if (p_led == NULL) {
        return;
    }
    m_led_ctrl.onoff_func(p_led->idx, 100);
    p_led->ctrl_handle = NULL;
    htd_list_del(&p_led->node);
    htd_list_insert_after(&p_led->node, &m_led_ctrl.list);
}

// @brief time_ms后开关某个灯
int32_t htd_led_onoff_time_ms(uint32_t idx, uint8_t onoff, uint32_t time_ms)
{
    htd_led_t* p_led;
    led_lock(m_led_ctrl.p_lock);
    p_led = led_node_find(&m_led_ctrl, idx);
    if (p_led) {
        if (time_ms == 0) {
            led_on_handle(p_led);
        } else {
            p_led->start_ms = TIME_MS_GET();
            p_led->timeout_ms = time_ms;
            p_led->ctrl_handle = led_on_handle;
            htd_list_del(&p_led->node);
            led_node_insert(&m_led_ctrl, p_led);
        }
    }
    led_unlock(m_led_ctrl.p_lock);
    return 0;
}

// @brief 周期闪烁控制句柄，亮->灭->亮
static void led_period_blink_handle(htd_led_t* p_led)
{
    if (p_led == NULL) {
        return;
    }
    if (p_led->period_blink.is_onoff) {
        m_led_ctrl.onoff_func(p_led->idx, 0);
    } else {
        m_led_ctrl.onoff_func(p_led->idx, 100);
    }
    p_led->start_ms = TIME_MS_GET();
    p_led->period_blink.is_onoff = !p_led->period_blink.is_onoff;
    p_led->period_blink.count++;
    htd_list_del(&p_led->node);
    if (p_led->period_blink.count >= p_led->period_blink.period_count) {
        p_led->ctrl_handle = NULL;
        // 插入到最后面
        htd_list_insert_before(&p_led->node, &m_led_ctrl.list);
    } else {
        // 根据超时时间插入
        led_node_insert(&m_led_ctrl, p_led);
    }
}

// @brief 周期闪烁
// @param blink_count 闪烁次数
// @param blink_period_ms 闪烁周期，单位毫秒；一亮一灭为一个周期
int32_t htd_led_period_blink(uint32_t idx, uint32_t blink_count, uint32_t blink_period_ms)
{
    htd_led_t* p_led;
    led_lock(m_led_ctrl.p_lock);
    p_led = led_node_find(&m_led_ctrl, idx);
    if (p_led) {
        p_led->ctrl_handle = led_period_blink_handle;
        p_led->start_ms = TIME_MS_GET();
        p_led->timeout_ms = blink_period_ms / 2;
        p_led->period_blink.period_count = blink_count * 2;
        p_led->period_blink.is_onoff = 1;
        p_led->period_blink.count = 0;
        htd_list_del(&p_led->node);
        led_node_insert(&m_led_ctrl, p_led);
        m_led_ctrl.onoff_func(p_led->idx, 100);
    }
    led_unlock(m_led_ctrl.p_lock);
    return 0;
}

// @brief 闪烁
static void led_custom_blink_handle(htd_led_t* p_led)
{
    if (p_led == NULL) {
        return;
    }
    
    if (p_led->custom_blink.is_onoff) {
        m_led_ctrl.onoff_func(p_led->idx, 0);
    } else {
        m_led_ctrl.onoff_func(p_led->idx, 100);
    }
    p_led->start_ms = TIME_MS_GET();
    p_led->custom_blink.is_onoff = !p_led->custom_blink.is_onoff;
    p_led->custom_blink.count++;
    htd_list_del(&p_led->node);
    if (p_led->custom_blink.count >= p_led->custom_blink.ms_array_count) {
        p_led->ctrl_handle = NULL;
        // 插入到最后面
        htd_list_insert_before(&p_led->node, &m_led_ctrl.list);
        htd_free(p_led->custom_blink.ms_array);
        p_led->custom_blink.ms_array = NULL;
    } else {
        // 根据超时时间插入
        p_led->timeout_ms = p_led->custom_blink.ms_array[p_led->custom_blink.count];
        led_node_insert(&m_led_ctrl, p_led);
    }
}

// @brief 自定义闪烁;
// @param p_array 定义亮->灭，亮->灭时长数组，单位毫秒; 
// @param array_count 必须为偶数
int32_t htd_led_custom_blink(uint32_t idx, uint16_t* p_array, uint32_t array_count)
{
    ASSERT(p_array);
    ASSERT(array_count > 0 && ((array_count & 0x01) == 0));
    
    htd_led_t* p_led;
    led_lock(m_led_ctrl.p_lock);
    p_led = led_node_find(&m_led_ctrl, idx);
    if (p_led) {
        p_led->ctrl_handle = led_custom_blink_handle;
        p_led->start_ms = TIME_MS_GET();
        p_led->timeout_ms = p_array[0];
        p_led->custom_blink.ms_array_count = array_count;
        p_led->custom_blink.ms_array = (uint16_t*)htd_malloc(sizeof(uint16_t) * p_led->custom_blink.ms_array_count);
        ASSERT(p_led->custom_blink.ms_array);
        p_led->custom_blink.is_onoff = 1;
        p_led->custom_blink.count = 0;
        
        htd_list_del(&p_led->node);
        led_node_insert(&m_led_ctrl, p_led);
        m_led_ctrl.onoff_func(p_led->idx, 100);
    }
    led_unlock(m_led_ctrl.p_lock);
    return 0;
}


