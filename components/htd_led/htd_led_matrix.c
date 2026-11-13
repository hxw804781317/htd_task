// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL       4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG       "led.matrix"
#include "htd_led_matrix.h"



// @brief 将行引脚配置为高阻
static void led_matrix_pin_init(led_matrix_t* p_object)
{
    uint32_t i;
    for (i=0; i<p_object->pin_col_count; ++i) {
        led_pin_output_pp_set(p_object->pin_col_table[i]);
        led_pin_write(p_object->pin_col_table[i], 0);
    }
    for (i=0; i<p_object->pin_row_count; ++i) {
        led_pin_output_pp_set(p_object->pin_row_table[i]);
        led_pin_write(p_object->pin_row_table[i], 0);
    }
}

// @brief 创建一个矩阵控制LED对象
led_matrix_t* htd_led_matrix_create(const pin_num_t *pin_row_table, uint8_t pin_row_count, const pin_num_t* pin_col_table, uint8_t pin_col_count)
{
    ASSERT(pin_row_table);
    ASSERT(pin_row_count);
    ASSERT(pin_col_table);
    ASSERT(pin_col_count);

    // 计算 malloc 额外需要的长度
    uint32_t malloc_len = sizeof(led_matrix_t);
    malloc_len += sizeof(pin_num_t) * pin_row_count;
    malloc_len += sizeof(pin_num_t) * pin_col_count;
    uint32_t state_table_len = ALIGN_UP(pin_row_count * pin_col_count, 32) / 32 * sizeof(uint32_t);
    malloc_len += state_table_len;

    // 创建对象并初始化
    uint8_t* p_ptr;
    led_matrix_t* p_object = (led_matrix_t*)htd_malloc(malloc_len);
    ASSERT(p_object);
    htd_memset(p_object, 0, sizeof(led_matrix_t));
    p_object->pin_col_count = pin_col_count;
    p_object->pin_row_count = pin_row_count;
    p_object->state_table_count = state_table_len / sizeof(uint32_t);
    p_ptr = (uint8_t*)(p_object + 1);
    p_object->state_table = (uint32_t*)p_ptr;
    p_ptr += state_table_len;
    htd_memset(p_object->state_table, 0, state_table_len);
    p_object->pin_col_table = (pin_num_t*)p_ptr;
    p_ptr += sizeof(pin_num_t) * pin_col_count;
    htd_memcpy(p_object->pin_col_table, pin_col_table, sizeof(pin_num_t) * pin_col_count);
    p_object->pin_row_table = (pin_num_t*)p_ptr;
    p_ptr += sizeof(pin_num_t) * pin_row_count;
    htd_memcpy(p_object->pin_row_table, pin_row_table, sizeof(pin_num_t) * pin_row_count);
    
    led_matrix_pin_init(p_object);
    return p_object;
}

// @brief 实际销毁句柄
static void led_matrix_destroy_handle(void* arg)
{
    led_matrix_t* p_object = (led_matrix_t*)arg;
    if (p_object) {
        led_matrix_pin_init(p_object);
        htd_free(p_object);
    }
}

// @brief 销毁矩阵控制 LED 对象
int32_t htd_led_matrix_destroy(led_matrix_t* p_object)
{
    if (!led_load_func(led_matrix_destroy_handle, p_object)) {
        ASSERT(0);
    }
    return 0;
}

// @brief 扫描句柄
static void led_matrix_scan_handle(void* arg)
{
    led_matrix_t* p_object = (led_matrix_t*)arg;
    if (p_object == NULL || p_object->is_running == false) {
        led_unload_timer_function(led_matrix_scan_handle);
        return;
    }

    bool row_on = false;
    uint32_t i, j, k;
    // 行全部拉高
    for (i=0; i<p_object->pin_row_count; ++i) {
        led_pin_write(p_object->pin_row_table[i], 1);
    }
    for (k=i=0; i<p_object->pin_row_count; ++i) {
        for (j=0; j<p_object->pin_col_count; ++j) {
            if (led_bitmap_pos_is_set(p_object->state_table, p_object->state_table_count, k)) {
                if (row_on == false) {
                    led_pin_write(p_object->pin_row_table[i], 0);           //目标行，拉低
                }
                led_pin_write(p_object->pin_col_table[j], 1);               // 目标列，拉高，LED灯亮
                row_on = true;
            } 
            ++k;
        }
        if (row_on) {
            row_on = false;
            // 短暂延时，提升亮度，防止闪烁
            DELAY_US(100); 
            
            // 列全部拉低，灯灭
            for (j=0; j<p_object->pin_col_count; ++j) {
                led_pin_write(p_object->pin_col_table[j], 0);
            }
            // 行拉高
            led_pin_write(p_object->pin_row_table[i], 1);
        }
    }

    // 行全部拉低，上面结束时，列已经全部拉低
    for (i=0; i<p_object->pin_row_count; ++i) {
        led_pin_write(p_object->pin_row_table[i], 0);
    }
}

// @brief 实际启动操作句柄
static void led_matrix_start_handle(void * arg)
{
    led_matrix_t* p_object = (led_matrix_t*)arg;
    if (p_object) {
        p_object->is_running = true;
        if (!led_load_timer_func(true, 5, led_matrix_scan_handle, p_object)) {
            ASSERT(0);
        }
    }
}

// @brief 启动矩阵控制
int32_t htd_led_matrix_start(led_matrix_t* p_object)
{
    ASSERT(p_object);
    if (!led_load_func(led_matrix_start_handle, p_object)) {
        ASSERT(0);
    }
    return 0;
}

// @brief 实际停止操作句柄
static void led_matrix_stop_handle(void* arg)
{
    led_matrix_t* p_object = (led_matrix_t*)arg;
    if (p_object) {
        led_matrix_pin_init(p_object);
        p_object->is_running = false;
    }
}

// @brief 关闭矩阵控制扫描, 所有灯都会灭
void htd_led_matrix_stop(led_matrix_t* p_object)
{
    ASSERT(p_object);
    if (!led_load_func(led_matrix_stop_handle, p_object)) {
        ASSERT(0);
    }
}

// @brief 控制某个灯亮/灭
int32_t htd_led_matrix_onoff(led_matrix_t* p_object, uint32_t led_idx, uint8_t onoff)
{
    ASSERT(p_object);
    ASSERT(led_idx < p_object->pin_col_count * p_object->pin_row_count);
    led_bitmap_pos_set(p_object->state_table, p_object->state_table_count, led_idx, onoff);
    return 0;
}

// @brief 显示按键矩阵序号
void htd_led_matrix_show(led_matrix_t* p_object)
{
#if defined(DEBUG_LVL) && (DEBUG_LVL == 4)
    uint32_t i, j, pos;
    print_debug("led index:");
    htd_print(" PIN\t");
    for (pos=0; pos<p_object->pin_col_count; ++pos) {
        htd_print("0x%02x, ", p_object->pin_col_table[pos]);
    }
    htd_print("\r\n");
    
    for (i=0; i<p_object->pin_row_count; ++i) {
        htd_print("0x%02x\t", p_object->pin_row_table[i]);
        for (j=0; j<p_object->pin_col_count; ++j) {
            pos = i * p_object->pin_col_count + j;
            htd_print(" %3u, ", pos);
        }
        htd_print("\r\n");
    }
#endif
}

