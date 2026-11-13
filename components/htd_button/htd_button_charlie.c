// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL       4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG       "button.charlie"
#include "htd_button_charlie.h"


// @brief 初始化引脚
static void button_charlie_pin_init(button_charlie_t * p_object)
{
    // 所有引脚配置为高阻态输入模式
    uint32_t pos;
    for (pos=0; pos<p_object->pin_count; ++pos) {
        button_pin_hiz_set(p_object->pin_table[pos]);
    }
}

// @brief 探测按下去的按键
static bool button_charlie_pressed_idx_detect(button_charlie_t* p_object, uint32_t* idx)
{
    uint32_t i, j;
    for (i=0; i<p_object->pin_count; ++i) {
        // 全部设为高阻, 避免冲突
        button_charlie_pin_init(p_object);
        
        // 上拉输入
        button_pin_input_pullup_set(p_object->pin_table[i]);
        for (j=0; j<p_object->pin_count; ++j) {
            if (i == j) {
                continue;
            }
            
            // 输出拉低
            button_pin_output_pp_set(p_object->pin_table[j]);
            button_pin_write(p_object->pin_table[j], 0);
            DELAY_US(5);        // 延迟等待电平稳定
            // 读取引脚电平, 拉低说明已经按下
            if (!button_pin_read(p_object->pin_table[i])) {
                i = i * p_object->pin_count + j;
                *idx = p_object->button_idx_table[i];
                return true;
            }
        }
    }
    return false;
}

// @brief 按下去执行事件
static void button_charlie_pressed_evt(button_charlie_t* p_object, uint32_t active_idx)
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
    uint32_t timeout_ms = (p_object->continuous_cnt + 1) * BUTTON_CHARLIE_HOLD_INTERVAL_MS;
    uint32_t pressed_ms = TIME_MS_DIFF(now_ms, p_object->active_time_ms);
    if (pressed_ms > timeout_ms) {
        p_object->evt_func(p_object->active_idx, BUTTON_CHARLIE_EVT_HOLD, pressed_ms);
        p_object->continuous_cnt++;
    }
}

// @brief 弹起来执行事件
static bool button_charlie_release_evt(button_charlie_t* p_object)
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
#if BUTTON_CHARLIE_SUPPORT_REPEAT
    if (p_object->repeat_count > 1) {
        // 多击事件
        if (release_time_ms >= HTD_BUTTON_REPEAT_INTER_TIME_MS) {
            p_object->evt_func(p_object->active_idx, BUTTON_CHARLIE_EVT_REPEAT, p_object->repeat_count);
            p_object->repeat_count = 0;
        } else {
            is_loop = true;
        }
    } else 
#endif
    if (pressed_ms < BUTTON_CHARLIE_SHORT_PRESS_MS) {
        // 按下时间未到短按时间, 有可能是抖动，不理
        if (release_time_ms < HTD_BUTTON_REPEAT_INTER_TIME_MS) {
            is_loop = true;
        }
    } else if (pressed_ms < BUTTON_CHARLIE_SHORT_TIMEOUT_MS) {
    #if BUTTON_CHARLIE_SUPPORT_REPEAT
        // 按下时间在短按时间之内
        if (release_time_ms > HTD_BUTTON_REPEAT_INTER_TIME_MS+5) {
            p_object->evt_func(p_object->active_idx, BUTTON_CHARLIE_EVT_SHORT, pressed_ms);
        } else {
            is_loop = true; // 延迟通报时间未到
        }
    #else
        p_object->evt_func(p_object->active_idx, BUTTON_CHARLIE_EVT_SHORT, pressed_ms);
    #endif
    } else if (pressed_ms < BUTTON_CHARLIE_LONG_PRESS_MS) {
        // 鉴于短按到长按之间，无事件
        
    } else {
        // 按下时间超出短按，为长按
        p_object->evt_func(p_object->active_idx, BUTTON_CHARLIE_EVT_LONG, pressed_ms);
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
static void button_charlie_scan_handle(void* arg)
{
    button_charlie_t* p_object = (button_charlie_t*)arg;
    ASSERT(p_object);
    if (p_object->is_running == false) {
        button_unload_timer_function(button_charlie_scan_handle);
        return;
    }

    uint32_t idx;
    if (!button_charlie_pressed_idx_detect(p_object, &idx)) {
        button_charlie_release_evt(p_object);
        return ;
    }
    
    // 按下事件
    button_charlie_pressed_evt(p_object, idx);
}

// @brief 创建一个对象
button_charlie_t* htd_button_charlie_create(const pin_num_t* pin_num_table, uint8_t pin_num_table_count, button_charlie_evt_notify_t evt_func)
{
    // button_idx_table 长度
    uint32_t button_idx_table_len = pin_num_table_count * pin_num_table_count;
    ASSERT(button_idx_table_len < BUTTON_INDEX_MAX);
    ASSERT(pin_num_table);

    uint32_t malloc_len = sizeof(button_charlie_t);
    malloc_len += button_idx_table_len;
    malloc_len += sizeof(pin_num_t) * pin_num_table_count;
    
    uint8_t* p_ptr;
    button_charlie_t* p_object = (button_charlie_t*)htd_malloc(malloc_len);
    ASSERT(p_object);
    htd_memset(p_object, 0, sizeof(button_charlie_t));
    p_object->evt_func = evt_func;
    p_object->button_idx_table_len = button_idx_table_len;
    p_object->pin_count = pin_num_table_count;
    p_ptr = (uint8_t*)(p_object + 1);
    p_object->pin_table = (pin_num_t*)p_ptr;
    p_ptr += sizeof(pin_num_t) * pin_num_table_count;
    htd_memcpy(p_object->pin_table, pin_num_table, sizeof(pin_num_t) * pin_num_table_count);
    
    // 初始化序列表
    uint32_t i, j, pos, index = 0;
    p_object->button_idx_table = (button_idx_t*)p_ptr;
    p_ptr += button_idx_table_len;
    for (pos=i=0; i<p_object->pin_count; ++i) {
        for (j=0; j<p_object->pin_count; ++j) {
            if (i == j) {
                p_object->button_idx_table[pos] = 0;
            } else {
                p_object->button_idx_table[pos] = index++;
            }
            ++pos;
        }
    }
    button_charlie_pin_init(p_object);
    return p_object;
}

// @brief 实际销毁句柄
static void button_charlie_destroy_handle(void* arg)
{
    button_charlie_t* p_object = (button_charlie_t*)arg;
    if (p_object) {
        button_charlie_pin_init(p_object);
        htd_free(p_object);
    }
}

// @brief 销毁查理复用对象
int32_t htd_button_charlie_destroy(button_charlie_t* p_object)
{
    if (!button_load_func(button_charlie_destroy_handle, p_object)) {
        ASSERT(0);
    }
    return 0;
}

// @brief 实际开始句柄
static void button_charlie_start_handle(void* arg)
{
    button_charlie_t* p_object = (button_charlie_t*)arg;
    if (p_object == NULL || p_object->is_running) {
        return ;
    }
    p_object->is_running = true;
    
    // 启动轮训
    if (false == button_load_timer_func(true, 10, button_charlie_scan_handle, p_object)) {
        print_error("load timer function failed.");
        ASSERT(0);
    }
}

// @brief 启动查理复用扫描(每隔5毫秒扫描一次)
int32_t htd_button_charlie_start(button_charlie_t* p_object)
{
    if (!button_load_func(button_charlie_start_handle, p_object)) {
        ASSERT(0);
    }
    return 0;
}

// @brief 实际停止操作句柄
static void button_charlie_stop_handle(void* arg)
{
    // 所有引脚设置为高阻输入
    button_charlie_t* p_object = (button_charlie_t*)arg;
    button_charlie_pin_init(p_object);
    p_object->is_running = false;
}

// @brief 关闭查理复用扫描, 所有灯都会灭
void htd_button_charlie_stop(button_charlie_t* p_object)
{
    ASSERT(p_object);
    if (!button_load_func(button_charlie_stop_handle, p_object)) {
        ASSERT(0);
    }
}

// @brief 判断某个按键是否按下
bool htd_button_charlie_is_pressed(button_charlie_t* p_object, uint32_t idx)
{
    ASSERT(p_object);
    if (p_object->is_running && p_object->active_idx == idx) {
        return true;
    }
    return false;
}

// @brief 显示按键矩阵序号
void htd_button_charlie_show(button_charlie_t* p_object)
{
#if defined(DEBUG_LVL) && (DEBUG_LVL == 4)
    uint32_t i, j, pos;
    print_debug("button index:");
    htd_print(" PIN\t");
    for (pos=0; pos<p_object->pin_count; ++pos) {
        htd_print("0x%02x, ", p_object->pin_table[pos]);
    }
    htd_print("\r\n");
    
    for (pos=i=0; i<p_object->pin_count; ++i) {
        htd_print("0x%02x\t", p_object->pin_table[i]);
        for (j=0; j<p_object->pin_count; ++j) {
            if (i == j) {
                htd_print("XXXX, ");
            } else {
                htd_print(" %3u, ", p_object->button_idx_table[pos]);
            }
            ++pos;
        }
        htd_print("\r\n");
    }
#endif
}



//////////////////////////////////////////////////////////////////////
// 未调试
#if (defined(HTD_CONSOLE_ENABLE) && (DEBUG_LVL == 4) && 0)
#include "htd_console_process.h"
#include "htd_device.h"

// 定义引脚
#ifdef SOC_FAMILY_STM32
    #define PIN_1      PIN_NUM_GET(A, 0)
    #define PIN_2      PIN_NUM_GET(A, 1)
    #define PIN_3      PIN_NUM_GET(A, 2)
    #define PIN_4      PIN_NUM_GET(A, 3)
#endif

static const pin_num_t charlie_pin[] = {PIN_1, PIN_2, PIN_3, PIN_4};
// 定义矩阵按键对象句柄
static button_charlie_t* m_button_charlie = NULL;

// @brief 事件通告句柄
static void app_button_charlie_notify_handle(uint32_t idx, button_charlie_evt_t evt, uint32_t param)
{
    if (evt == BUTTON_CHARLIE_EVT_HOLD) {
        print_warning("idx=%d, Hold: timeout_ms=%u", idx, param);
    } else if (evt == BUTTON_CHARLIE_EVT_SHORT) {
        print_warning("idx=%d, Short: timeout_ms=%u", idx, param);
    } else if (evt == BUTTON_CHARLIE_EVT_LONG) {
        print_warning("idx=%d, Long: timeout_ms=%u", idx, param);
    } else if (evt == BUTTON_CHARLIE_EVT_REPEAT) {
        print_warning("idx=%d, Repeat: count=%d", idx, param);
    }
}

// @brief 调试命令
static int button_charlie_cmd(int argc, char **argv)
{
    if (argc < 2) {
        print_error("Usage: %s start/stop/show", argv[0]);
        return 0;
    }
    if (htd_memcmp(argv[1], "start", 6) == 0) {
        if (m_button_charlie == NULL) {
            m_button_charlie = htd_button_charlie_create(charlie_pin, ARRAY_COUNT(charlie_pin), app_button_charlie_notify_handle);
            ASSERT(m_button_charlie);
        }
        htd_button_charlie_start(m_button_charlie);
    } else if (htd_memcmp(argv[1], "stop", 5) == 0) {
        if (m_button_charlie) {
            htd_button_charlie_stop(m_button_charlie);
            htd_button_charlie_destroy(m_button_charlie);
            m_button_charlie = NULL;
        }
    } else if (htd_memcmp(argv[1], "show", 5) == 0) {
        if (m_button_charlie) {
            htd_button_charlie_show(m_button_charlie);
        }
    } else {
        print_error("Usage: %s start/stop/show", argv[0]);
    }
    return 0;
}
CONSOLE_CMD_EXPORT(button_charlie_cmd, button_charlie);
#endif

