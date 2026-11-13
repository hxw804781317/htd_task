// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL       4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG       "led.port"
#include "htd_led_port.h"


// @brief 标记bitmap表第pos位为1或者0，is_enable为true，标记为1，否则为0
void led_bitmap_pos_set(uint32_t* p_bitmap_table, uint32_t count, uint16_t pos, bool is_enable)
{
#if 0
    uint32_t all_bits;
    uint32_t index;
    uint32_t val;

    all_bits = count * 32;
    if (pos >= all_bits) {
        print_error("bitmap set failed. pos=%d, count=%d, all_bits=%d", pos, count, all_bits);
        return;
    }

    index = pos >> 5;
    pos &= 31;
    val = p_bitmap_table[index];
    if (is_enable) {
        val |= (1UL << pos);
    } else {
        val &= ~(1UL << pos);
    }
    p_bitmap_table[index] = val;
#else
    htd_bitmap_pos_set(p_bitmap_table, count, pos, is_enable);
#endif
}

// @brief 判断bitmap表第pos是否被标记为1
bool led_bitmap_pos_is_set(uint32_t *p_bitmap_table, uint32_t count, uint16_t pos)
{
#if 0
    uint32_t index;
    if (pos >= count * 32) {
        return false;
    }
    index = pos >> 5;
    pos &= 31;
    return !!(p_bitmap_table[index] & (1UL << pos));
#endif
    return htd_bitmap_pos_is_set(p_bitmap_table, count, pos);
}

// @brief 加载一个任务
bool led_load_func(void (*timeout_cb)(void*), void* arg)
{
    return htd_task_load_function(timeout_cb, arg); 
}

// @brief 加载一个定时任务
bool led_load_timer_func(bool is_repeat, uint32_t timeout_ms, void (*timeout_cb)(void*), void* arg)
{
    return htd_task_load_timer_function(is_repeat, timeout_ms, timeout_cb, arg);
}

// @brief 卸载定时任务
void led_unload_timer_function(void (*timeout_cb)(void*))
{
    htd_task_unload_timer_function(timeout_cb);
}

// @brief 创建锁
void* led_lock_create(const char* p_name)
{
    return htd_mutex_lock_create(p_name);
}

// @brief 加锁
void led_lock(void* p_lock)
{
    htd_mutex_lock((htd_mutex_lock_t*)p_lock);
}

// @brief 解锁
void led_unlock(void* p_lock)
{
    htd_mutex_unlock((htd_mutex_lock_t*)p_lock);
}

// @brief 某个引脚控制为输入高阻态模式(浮空模式)
void led_pin_input_nopull_set(pin_num_t pin_num)
{
    //htd_pin_hiz_set(pin_num);
    htd_pin_input_set(pin_num, HTD_PIN_NOPULL);
}

// @brief 某个引脚控制为输出
void led_pin_output_pp_set(pin_num_t pin_num)
{
    htd_pin_output_pp_set(pin_num);
}

// @brief 控制某个引脚高低电平
void led_pin_write(pin_num_t pin_num, uint8_t is_set)
{
    htd_pin_write(pin_num, is_set ? 1 : 0);
}


// @brief 创建硬件定时器
void* led_hw_timer_create(const char* p_name, uint32_t timeout_us, bool is_repeat, void (*timeout_cb)(void*), void* param)
{
    return htd_dev_timer_create(p_name, timeout_us, is_repeat ? HTD_TIMER_MODE_REPEAT : HTD_TIMER_MODE_ONESHOT, timeout_cb, param);
}
// @brief 销毁定时器
int32_t led_hw_timer_destroy(void* p_handle)
{
    return htd_dev_timer_destroy((htd_timer_handle_t*)p_handle);
}
// @brief 重启定时器
int32_t led_hw_timer_restart(void* p_handle, uint32_t timeout_us)
{
    return htd_dev_timer_restart((htd_timer_handle_t*)p_handle, timeout_us);
}
// @brief 停止硬件定时器
int32_t led_hw_timer_stop(void* p_handle)
{
    return htd_dev_timer_stop((htd_timer_handle_t*)p_handle);
}
// @brief 获取参数
void* led_hw_timer_param_get(void* p_handle)
{
    return htd_dev_timer_param_get((htd_timer_handle_t*)p_handle);
}


