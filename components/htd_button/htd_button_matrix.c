// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL       4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG       "button.matrix"
#include "htd_button_matrix.h"


// @brief 初始化引脚
static void button_matrix_pin_init(button_matrix_t * p_object)
{
    uint32_t i;
    // 列：推挽输出拉低
    for (i=0; i<p_object->pin_col_count; ++i) {
        button_pin_output_pp_set(p_object->pin_col_table[i]);
        button_pin_write(p_object->pin_col_table[i], 0);
    }
    // 行: 下降沿中断
    for (i=0; i<p_object->pin_row_count; ++i) {
        button_pin_int_falling_set(p_object->pin_row_table[i], true);
    }
}

// @brief 探测按下去的按键
static bool button_matrix_pressed_idx_detect(button_matrix_t* p_object, uint32_t* idx)
{
    uint32_t i;
    int32_t col_idx = -1;
    
    // 行拉低，列上拉，读列引脚电平
    for (i=0; i<p_object->pin_row_count; ++i) {
        button_pin_output_pp_set(p_object->pin_row_table[i]);
        button_pin_write(p_object->pin_col_table[i], 0);
    }
    for (i=0; i<p_object->pin_col_count; ++i) {
        button_pin_input_pullup_set(p_object->pin_col_table[i]);
    }
    DELAY_US(10);       // 延迟一点时间，让电平生效
    for (i=0; i<p_object->pin_col_count; ++i) {
        if (!button_pin_read(p_object->pin_col_table[i])) {
            col_idx = i;    // 只记第一个有效行
            break;
        }
    }
    if (col_idx < 0) {
        return false;
    }

    // 列拉低，行上拉，读行引脚电平
    int32_t row_idx = -1;
    for (i=0; i<p_object->pin_col_count; ++i) {
        button_pin_output_pp_set(p_object->pin_col_table[i]);
        button_pin_write(p_object->pin_col_table[i], 0);
    }
    for (i=0; i<p_object->pin_row_count; ++i) {
        button_pin_input_pullup_set(p_object->pin_row_table[i]);
    }
    DELAY_US(10);       // 延迟一点时间，让电平生效
    for (i=0; i<p_object->pin_row_count; ++i) {
        if (!button_pin_read(p_object->pin_row_table[i])) {
            row_idx = i;    // 只记第一个有效行
            break;
        }
    }
    if (row_idx < 0) {
        return false;
    }

    // 计算第几个按键被按下
    i = row_idx * p_object->pin_col_count + col_idx;
    if (idx) {
        *idx = i;
    }
    return true;
}

// @brief 按下去执行事件
static void button_matrix_pressed_evt(button_matrix_t* p_object, uint32_t active_idx)
{
    // 探测按键事件
    uint32_t now_ms = TIME_MS_GET();
    if (p_object->active_idx != active_idx) {
        p_object->active_idx = active_idx;
        p_object->active_time_ms = now_ms;
        p_object->release_time_ms = 0;
        p_object->repeat_count++;
    } else if (p_object->release_time_ms) {
        p_object->repeat_count++;
        p_object->release_time_ms = 0;
        p_object->continuous_cnt = 0;
        p_object->active_time_ms = now_ms;
    }

    // 按下去不断上报事件
    uint32_t timeout_ms = (p_object->continuous_cnt + 1) * BUTTON_MATRIX_HOLD_INTERVAL_MS;
    uint32_t pressed_ms = TIME_MS_DIFF(now_ms, p_object->active_time_ms);
    if (pressed_ms > timeout_ms) {
        p_object->evt_func(p_object->active_idx, BUTTON_MATRIX_EVT_HOLD, pressed_ms);
        p_object->continuous_cnt++;
    }
}

// @brief 弹起来执行事件
static bool button_matrix_release_evt(button_matrix_t* p_object)
{
    if (p_object->active_time_ms == 0) {
        // 未曾按下
        return false;
    }

    bool is_loop = false;
    uint32_t now_ms = TIME_MS_GET();
    if (p_object->release_time_ms == 0) {
        p_object->release_time_ms = now_ms;
        p_object->continuous_cnt = 0;
    }
    
    uint32_t release_time_ms = TIME_MS_DIFF(now_ms, p_object->release_time_ms);
    uint32_t pressed_ms = TIME_MS_DIFF(p_object->release_time_ms, p_object->active_time_ms);
#if BUTTON_MATRIX_SUPPORT_REPEAT
    if (p_object->repeat_count > 1) {
        // 多击事件
        if (release_time_ms >= HTD_BUTTON_REPEAT_INTER_TIME_MS) {
            p_object->evt_func(p_object->active_idx, BUTTON_MATRIX_EVT_REPEAT, p_object->repeat_count);
            p_object->repeat_count = 0;
        } else {
            is_loop = true;
        }
    } else 
#endif
    if (pressed_ms < BUTTON_MATRIX_SHORT_PRESS_MS) {
        // 按下时间未到短按时间, 有可能是抖动，不理
        if (release_time_ms < HTD_BUTTON_REPEAT_INTER_TIME_MS) {
            is_loop = true;
        }
    } else if (pressed_ms < BUTTON_MATRIX_SHORT_TIMEOUT_MS) {
    #if BUTTON_MATRIX_SUPPORT_REPEAT
        // 按下时间在短按时间之内
        if (release_time_ms > HTD_BUTTON_REPEAT_INTER_TIME_MS+5) {
            p_object->evt_func(p_object->active_idx, BUTTON_MATRIX_EVT_SHORT, pressed_ms);
        } else {
            is_loop = true; // 延迟通报时间未到
        }
    #else
        p_object->evt_func(p_object->active_idx, BUTTON_MATRIX_EVT_SHORT, pressed_ms);
    #endif
    } else if (pressed_ms < BUTTON_MATRIX_LONG_PRESS_MS) {
        // 鉴于短按到长按之间，无事件
        
    } else {
        // 按下时间超出短按，为长按
        p_object->evt_func(p_object->active_idx, BUTTON_MATRIX_EVT_LONG, pressed_ms);
    }

    if (is_loop == false) {
        p_object->release_time_ms = 0;
        p_object->active_time_ms = 0;
        p_object->repeat_count = 0;
        p_object->continuous_cnt = 0;
        p_object->active_idx = 0xffff;
    }
    return is_loop;
}

// @brief 扫描句柄
static void button_matrix_scan_handle(void* arg)
{
    button_matrix_t* p_object = (button_matrix_t*)arg;
    ASSERT(p_object);
    if (p_object->is_running == false) {
        button_unload_timer_function(button_matrix_scan_handle);
        return;
    }

    uint32_t idx;
    if (!button_matrix_pressed_idx_detect(p_object, &idx)) {
        if (button_matrix_release_evt(p_object)) {
            return;
        }
        
        // 初始化引脚
        button_matrix_pin_init(p_object); 
        // 没有按键按下
        button_unload_timer_function(button_matrix_scan_handle);
        p_object->is_running = false;
        return ;
    }
    // 按下事件
    button_matrix_pressed_evt(p_object, idx);
}

// @brief 创建一个矩阵控制按键对象
button_matrix_t* htd_button_matrix_create(const pin_num_t *pin_row, uint8_t pin_row_count, const pin_num_t* pin_column, uint8_t pin_column_count, button_matrix_evt_notify_t evt_func)
{
    ASSERT(pin_row);
    ASSERT(pin_row_count);
    ASSERT(pin_column);
    ASSERT(pin_column_count);
    ASSERT(evt_func);

    // 计算 malloc 额外需要的长度
    uint32_t malloc_len = sizeof(button_matrix_t);
    malloc_len += sizeof(pin_num_t) * pin_row_count;
    malloc_len += sizeof(pin_num_t) * pin_column_count;

    // 创建对象并初始化
    uint8_t* p_ptr;
    button_matrix_t* p_object = htd_malloc(malloc_len);
    ASSERT(p_object);
    htd_memset(p_object, 0, sizeof(button_matrix_t));
    p_object->active_idx = 0xffff;
    p_object->evt_func = evt_func;
    p_object->pin_col_count = pin_column_count;
    p_object->pin_row_count = pin_row_count;
    p_ptr = (uint8_t*)(p_object + 1);
    p_object->pin_col_table = (pin_num_t*)p_ptr;
    p_ptr += sizeof(pin_num_t) * pin_column_count;
    htd_memcpy(p_object->pin_col_table, pin_column, sizeof(pin_num_t) * pin_column_count);
    p_object->pin_row_table = (pin_num_t*)p_ptr;
    p_ptr += sizeof(pin_num_t) * pin_row_count;
    htd_memcpy(p_object->pin_row_table, pin_row, sizeof(pin_num_t) * pin_row_count);
    
    // 初始化引脚
    button_matrix_pin_init(p_object);
    return p_object;
}

// @brief 实际销毁句柄
static void button_matrix_destroy_handle(void* arg)
{
    uint32_t i;
    button_matrix_t* p_object = (button_matrix_t*)arg;
    if (p_object) {
        for (i=0; i<p_object->pin_col_count; ++i) {
            button_pin_hiz_set(p_object->pin_col_table[i]);
        }
        for (i=0; i<p_object->pin_row_count; ++i) {
            button_pin_hiz_set(p_object->pin_row_table[i]);
        }
        htd_free(p_object);
    }
}

// @brief 销毁矩阵控制按键对象
int32_t htd_button_matrix_destroy(button_matrix_t* p_object)
{
    button_load_func(button_matrix_destroy_handle, p_object);
    return 0;
}

// @brief 中断事件实际执行句柄
static void button_matrix_irq_evt_handler(void *arg)
{
    button_matrix_t* p_object = (button_matrix_t*)arg;
    if (p_object == NULL || p_object->is_running) {
        return;
    }
    p_object->is_running = true;

    // 行中断关掉
    uint32_t i;
    for (i=0; i<p_object->pin_row_count; ++i) {
        button_pin_int_falling_set(p_object->pin_row_table[i], false);
    }
    DELAY_US(10);

    // 行拉低
    for (i=0; i<p_object->pin_row_count; ++i) {
        button_pin_output_pp_set(p_object->pin_row_table[i]);
        button_pin_write(p_object->pin_row_table[i], 0);
    }
    // 列上拉输入
    for (i=0; i<p_object->pin_col_count; ++i) {
        button_pin_input_pullup_set(p_object->pin_col_table[i]);
    }
    // 启动轮训
    if (false == button_load_timer_func(true, 10, button_matrix_scan_handle, p_object)) {
        print_error("load timer function failed.");
        ASSERT(0);
        return;
    }
}

// @brief 中断回调函数
void htd_button_matrix_irq_evt_handler(button_matrix_t* p_object) 
{
    button_load_func(button_matrix_irq_evt_handler, p_object);
}

// @brief 判断某个按键是否按下
bool htd_button_matrix_is_pressed(button_matrix_t* p_object, uint32_t idx)
{
    ASSERT(p_object);
    if (p_object->is_running && p_object->active_idx == idx) {
        return true;
    }
    return false;
}

// @brief 显示按键矩阵序号
void htd_button_matrix_show(button_matrix_t* p_object)
{
#if defined(DEBUG_LVL) && (DEBUG_LVL == 4)
    uint32_t i, j, pos;
    print_debug("button index:");
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



//////////////////////////////////////////////////////////////////////
#if (defined(HTD_CONSOLE_ENABLE) && (DEBUG_LVL == 4) && 0)
#include "htd_console_process.h"
#include "htd_device.h"

// 定义引脚
#ifdef SOC_FAMILY_PY32
    #define PIN_R1      PIN_NUM_GET(D, 3)
    #define PIN_R2      PIN_NUM_GET(D, 4)
    #define PIN_R3      PIN_NUM_GET(D, 5)
    #define PIN_R4      PIN_NUM_GET(D, 6)
    
    #define PIN_C1      PIN_NUM_GET(C, 4)
    #define PIN_C2      PIN_NUM_GET(C, 5)
    #define PIN_C3      PIN_NUM_GET(C, 6)
    #define PIN_C4      PIN_NUM_GET(C, 7)
#endif
#ifdef SOC_FAMILY_STM32
    #define PIN_R1      PIN_NUM_GET(A, 0)
    #define PIN_R2      PIN_NUM_GET(A, 1)
    #define PIN_R3      PIN_NUM_GET(A, 2)
    #define PIN_R4      PIN_NUM_GET(A, 3)

    #define PIN_C1      PIN_NUM_GET(B, 3)
    #define PIN_C2      PIN_NUM_GET(B, 4)
    #define PIN_C3      PIN_NUM_GET(B, 5)
    #define PIN_C4      PIN_NUM_GET(B, 6)
#endif
static const pin_num_t col_pin[] = {PIN_C1, PIN_C2, PIN_C3, PIN_C4};
static const pin_num_t row_pin[] = {PIN_R1, PIN_R2, PIN_R3, PIN_R4};
// 定义矩阵按键对象句柄
static button_matrix_t* m_button_matrix = NULL;



// @brief 事件通告句柄
static void app_button_matrix_notify_handle(uint32_t idx, button_matrix_evt_t evt, uint32_t param)
{
    if (evt == BUTTON_MATRIX_EVT_HOLD) {
        print_warning("idx=%d, Hold: timeout_ms=%u", idx, param);
    } else if (evt == BUTTON_MATRIX_EVT_SHORT) {
        print_warning("idx=%d, Short: timeout_ms=%u", idx, param);
    } else if (evt == BUTTON_MATRIX_EVT_LONG) {
        print_warning("idx=%d, Long: timeout_ms=%u", idx, param);
    } else if (evt == BUTTON_MATRIX_EVT_REPEAT) {
        print_warning("idx=%d, Repeat: count=%d", idx, param);
    }
}

// @brief 中断引脚
void app_button_row1_int_handle(void)
{
    htd_button_matrix_irq_evt_handler(m_button_matrix);
    //print_debug("row1");
}
HTD_PIN_INT_FUNC_REGISTER(PIN_R1, app_button_row1_int_handle);
void app_button_row2_int_handle(void)
{
    htd_button_matrix_irq_evt_handler(m_button_matrix);
    //print_debug("row2");
}
HTD_PIN_INT_FUNC_REGISTER(PIN_R2, app_button_row2_int_handle);
void app_button_row3_int_handle(void)
{
    htd_button_matrix_irq_evt_handler(m_button_matrix);
    //print_debug("row3");
}
HTD_PIN_INT_FUNC_REGISTER(PIN_R3, app_button_row3_int_handle);
void app_button_row4_int_handle(void)
{
    htd_button_matrix_irq_evt_handler(m_button_matrix);
    //print_debug("row4");
}
HTD_PIN_INT_FUNC_REGISTER(PIN_R4, app_button_row4_int_handle);


// @brief 调试命令
static int button_matrix_cmd(int argc, char **argv)
{
    if (argc < 2) {
        print_error("Usage: %s start/stop/show", argv[0]);
        return 0;
    }
    if (htd_memcmp(argv[1], "start", 6) == 0) {
        if (m_button_matrix == NULL) {
            m_button_matrix = htd_button_matrix_create(row_pin, ARRAY_COUNT(row_pin), col_pin, ARRAY_COUNT(col_pin), app_button_matrix_notify_handle);
            ASSERT(m_button_matrix);
        }
    } else if (htd_memcmp(argv[1], "stop", 5) == 0) {
        if (m_button_matrix) {
            htd_button_matrix_destroy(m_button_matrix);
            m_button_matrix = NULL;
        }
    } else if (htd_memcmp(argv[1], "show", 5) == 0) {
        if (m_button_matrix) {
            htd_button_matrix_show(m_button_matrix);
        }
    } else {
        print_error("Usage: %s start/stop/show", argv[0]);
    }
    return 0;
}
CONSOLE_CMD_EXPORT(button_matrix_cmd, button_matrix);
#endif


