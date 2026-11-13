// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL       4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG       "button.port"
#include "htd_button_port.h"

// @brief 标记bitmap表第pos位为1或者0，is_enable为true，标记为1，否则为0
void button_bitmap_pos_set(uint32_t* p_bitmap_table, uint32_t count, uint16_t pos, bool is_enable)
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
bool button_bitmap_pos_is_set(uint32_t *p_bitmap_table, uint32_t count, uint16_t pos)
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
bool button_load_func(void (*timeout_cb)(void*), void* arg)
{
    return htd_task_load_function(timeout_cb, arg); 
}

// @brief 加载一个定时任务
bool button_load_timer_func(bool is_repeat, uint32_t timeout_ms, void (*timeout_cb)(void*), void* arg)
{
    return htd_task_load_timer_function(is_repeat, timeout_ms, timeout_cb, arg);
}

// @brief 卸载定时任务
void button_unload_timer_function(void (*timeout_cb)(void*))
{
    htd_task_unload_timer_function(timeout_cb);
}

// @brief 配置引脚为高阻态
void button_pin_hiz_set(pin_num_t pin_num)
{
    htd_pin_hiz_set(pin_num);
}

// @brief 配置引脚为推挽输出
void button_pin_output_pp_set(pin_num_t pin_num)
{
    htd_pin_output_pp_set(pin_num);
}

// @brief 配置电平
void button_pin_write(pin_num_t pin_num, uint8_t is_set)
{
    htd_pin_write(pin_num, is_set ? 1 : 0);
}

// @brief 配置为上拉输入
void button_pin_input_pullup_set(pin_num_t pin_num)
{
    htd_pin_input_set(pin_num, HTD_PIN_PULLUP);
}

// @brief 配置引脚为下降沿中断，上拉输入
void button_pin_int_falling_set(pin_num_t pin_num, bool is_enable)
{
    htd_pin_int_set(pin_num, HTD_PIN_INT_FALLING, is_enable);
}

// @brief 读取引脚电平
bool button_pin_read(pin_num_t pin_num)
{
    return htd_pin_read(pin_num);
}

