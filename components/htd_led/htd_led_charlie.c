// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL       4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG       "led.charlie"
#include "htd_led_charlie.h"


//////////////////////////////////////////////////////////////////////
// @brief 初始化引脚
static void led_charlie_pin_init(led_charlie_t * p_object)
{
    // 所有引脚配置为高阻态输入模式
    uint32_t pos;
    for (pos=0; pos<p_object->pin_count; ++pos) {
        led_pin_input_nopull_set(p_object->pin_table[pos]);
    }
}


#if defined(HTD_LED_USE_HW_TIMER)               // 使用硬件定时器实现查理复用LED
//////////////////////////////////////////////////////////////////////
// @brief 定时器句柄
static void led_charlie_hw_timer_handle(void* arg)
{
    led_charlie_t* p_object = (led_charlie_t*)led_hw_timer_param_get(arg);
    ASSERT(p_object);
    if (p_object->is_running == false) {
        led_hw_timer_stop(p_object->hw_timer_handle);
        return;
    }
    // 有开关命令
    if (p_object->on_off_idx != LED_INDEX_MAX) {
        led_bitmap_pos_set(p_object->state_table, p_object->state_table_count, p_object->on_off_idx, p_object->on_off);
        p_object->on_off_idx = LED_INDEX_MAX;
    }
    // 灯亮着，先灭掉
    if (p_object->row_on) {
        led_charlie_pin_init(p_object);
        p_object->row_on = false;
    }

    // 每次只亮一行
    uint32_t k = p_object->row_idx * p_object->pin_count;
    for (uint32_t j=0; j<p_object->pin_count; ++j) {
        if (j == p_object->row_idx) {
            continue;
        }
        if (led_bitmap_pos_is_set(p_object->state_table, p_object->state_table_count, p_object->led_idx_table[k])) {
            if (p_object->row_on == false) {
                p_object->row_on = true;
                led_pin_output_pp_set(p_object->pin_table[p_object->row_idx]);
                led_pin_write(p_object->pin_table[p_object->row_idx], 0);        // 目标引脚阴极拉低
            }
            led_pin_output_pp_set(p_object->pin_table[j]);
            led_pin_write(p_object->pin_table[j], 1);                           // 目标引脚阳极拉高
        }
        k++;
    }
    p_object->row_idx++;
    if (p_object->row_idx >= p_object->pin_count) {
        p_object->row_idx = 0;
    }
}

// @brief 实际销毁操作句柄
static void led_charlie_destroy(void* arg)
{
    led_charlie_t* p_object = (led_charlie_t*)arg;
    if (p_object && p_object->is_running) {
        // 暂停&销毁定时器
        led_hw_timer_stop(p_object->hw_timer_handle);
        led_hw_timer_destroy(p_object->hw_timer_handle);
        
        led_charlie_pin_init(p_object);
        htd_free(p_object);
    }
}

// @brief 实际启动操作句柄
static void led_charlie_start_handle(void* arg)
{
    led_charlie_t* p_object = (led_charlie_t*)arg;
    if (p_object && p_object->is_running == false) {
        // 启动扫描
        led_hw_timer_restart(p_object->hw_timer_handle, p_object->row_on_us);
        p_object->is_running = true;
    }
}

// @brief 实际结束操作句柄
static void led_charlie_stop_handle(void* arg)
{
    led_charlie_t* p_object = (led_charlie_t*)arg;
    if (p_object) {
        led_hw_timer_stop(p_object->hw_timer_handle);
        
        // 所有引脚设置为高阻输入
        led_charlie_pin_init(p_object);
        p_object->is_running = false;
    }
}

// @brief 开关控制实际操作句柄
static void led_charlie_onoff_handle(void* arg)
{
    led_onoff_param_t* p_param = (led_onoff_param_t*)arg;
    if (p_param) {
        led_charlie_t* p_object = p_param->p_object;
        if (p_object) {
            p_object->on_off_idx = p_param->led_idx;
            p_object->on_off = p_param->onoff;
        }
        htd_free(p_param);
    }
}


#else                                           // 使用软件定时器实现查理复用LED
//////////////////////////////////////////////////////////////////////
typedef struct {
    led_charlie_t* p_object;
    uint32_t led_idx;
    uint8_t onoff;
} led_onoff_param_t;

// @brief 扫描句柄
static void led_charlie_scan_handle(void* arg)
{
    led_charlie_t* p_object = (led_charlie_t*)arg;
    if (p_object->is_running == false) {
        led_unload_timer_function(led_charlie_scan_handle);
        return;
    }

    uint32_t i, j, k;
    bool row_on = false;
    for (k=i=0; i<p_object->pin_count; ++i) {
        for (j=0; j<p_object->pin_count; ++j) {                 // 遍历列，输出高电平(正极)
            if (i != j) {
                if (led_bitmap_pos_is_set(p_object->state_table, p_object->state_table_count, p_object->led_idx_table[k])) {
                    if (row_on == false) {
                        led_pin_output_pp_set(p_object->pin_table[i]);
                        led_pin_write(p_object->pin_table[i], 0);           // 目标引脚阴极拉低
                    }
                    led_pin_output_pp_set(p_object->pin_table[j]);
                    led_pin_write(p_object->pin_table[j], 1);               // 目标引脚阳极拉高
                    row_on = true;
                } 
            }
            ++k;
        }
        if (row_on) {
            // 这里延迟有点消费系统性能，可以使用硬件定时器做更高精度的优化
            DELAY_US(p_object->row_on_us);
            led_charlie_pin_init(p_object);
            row_on = false;
        }
    }
}

// @brief 实际销毁操作句柄
static void led_charlie_destroy(void* arg)
{
    led_charlie_t* p_object = (led_charlie_t*)arg;
    if (p_object) {
        led_unload_timer_function(led_charlie_scan_handle);
        led_charlie_pin_init(p_object);
        htd_free(p_object);
    }
}

// @brief 实际启动操作句柄
static void led_charlie_start_handle(void* arg)
{
    led_charlie_t* p_object = (led_charlie_t*)arg;
    if (p_object && p_object->is_running == false) {
        if (!led_load_timer_func(true, 1000/HTD_LED_CHARLIE_FRAME_RATE, led_charlie_scan_handle, p_object)) {
            ASSERT(0);
        }
        p_object->is_running = true;
    }
}

// @brief 实际结束操作句柄
static void led_charlie_stop_handle(void* arg)
{
    led_charlie_t* p_object = (led_charlie_t*)arg;
    if (p_object) {
        // 所有引脚设置为高阻输入
        led_unload_timer_function(led_charlie_scan_handle);
        led_charlie_pin_init(p_object);
        p_object->is_running = false;
    }
}

// @brief 开关控制实际操作句柄
static void led_charlie_onoff_handle(void* arg)
{
    led_onoff_param_t* p_param = (led_onoff_param_t*)arg;
    if (p_param) {
        if (p_param->p_object) {
            led_bitmap_pos_set(p_param->p_object->state_table, p_param->p_object->state_table_count, p_param->led_idx, p_param->onoff);
        }
        htd_free(p_param);
    }
}
#endif /**/



//////////////////////////////////////////////////////////////////////
// @brief 创建一个查理复用LED对象
led_charlie_t* htd_led_charlie_create(const pin_num_t* pin_num_table, uint8_t pin_num_table_count)
{
    ASSERT(pin_num_table);
    ASSERT(pin_num_table_count > 1);

    // state_table 长度，led_idx_table 长度
    uint32_t state_table_len = ALIGN_UP(pin_num_table_count, 32) / 32 * sizeof(uint32_t);
    uint32_t led_idx_table_len = pin_num_table_count * pin_num_table_count;
    ASSERT(led_idx_table_len < LED_INDEX_MAX);

    uint32_t malloc_len = sizeof(led_charlie_t);
    malloc_len += state_table_len;
    malloc_len += led_idx_table_len;
    malloc_len += sizeof(pin_num_t) * pin_num_table_count;

    uint8_t* p_ptr;
    led_charlie_t* p_object = (led_charlie_t*)htd_malloc(malloc_len);
    ASSERT(p_object);
    htd_memset(p_object, 0, sizeof(led_charlie_t));
    p_object->row_on_us = 1000000 / (HTD_LED_CHARLIE_FRAME_RATE * pin_num_table_count);
    p_object->state_table_count = state_table_len / sizeof(uint32_t);
    p_object->led_idx_table_len = led_idx_table_len;
    p_object->pin_count = pin_num_table_count;
    p_ptr = (uint8_t*)(p_object + 1);
    p_object->state_table = (uint32_t*)p_ptr;
    p_ptr += state_table_len;
    htd_memset(p_object->state_table, 0, state_table_len);
    p_object->pin_table = (pin_num_t*)p_ptr;
    htd_memcpy(p_object->pin_table, pin_num_table, sizeof(pin_num_t) * pin_num_table_count);
    p_ptr += sizeof(pin_num_t) * pin_num_table_count;

    // 初始化LED序列表
    p_object->led_idx_table = (led_idx_t*)p_ptr;
    p_ptr += led_idx_table_len;
    uint32_t led_idx = 0, i, j, pos;
    for (pos=i=0; i<pin_num_table_count; ++i) {
        for (j=0; j<pin_num_table_count; ++j) {
            if (i == j) {
                p_object->led_idx_table[pos] = 0;
            } else {
                p_object->led_idx_table[pos] = led_idx++;
            }
            ++pos;
        }
    }

    // 初始化引脚
    led_charlie_pin_init(p_object);

    // 初始化硬件定时器
#ifdef HTD_LED_USE_HW_TIMER
    p_object->on_off_idx = LED_INDEX_MAX;
    void* hw_timer = led_hw_timer_create(HTD_LED_HW_TIMER_NAME, 10, true, led_charlie_hw_timer_handle, p_object);
    ASSERT(hw_timer);
    p_object->hw_timer_handle = hw_timer;
#endif /*HTD_LED_USE_HW_TIMER*/
    return p_object;
}

// @brief 销毁查理复用 LED 对象
int32_t htd_led_charlie_destroy(led_charlie_t* p_object)
{
    if (!led_load_func(led_charlie_destroy, p_object)) {
        ASSERT(0);
    }
    return 0;
}

// @brief 启动查理复用扫描(每隔5毫秒扫描一次)
int32_t htd_led_charlie_start(led_charlie_t* p_object)
{
    if (p_object) {
        if (!led_load_func(led_charlie_start_handle, p_object)) {
            ASSERT(0);
        }
    }
    return 0;
}

// @brief 关闭查理复用扫描, 所有灯都会灭
void htd_led_charlie_stop(led_charlie_t* p_object)
{
    if (!led_load_func(led_charlie_stop_handle, p_object)) {
        ASSERT(0);
    }
}

// @brief 控制某个灯亮/灭
int32_t htd_led_charlie_onoff(led_charlie_t* p_object, uint32_t led_idx, uint8_t onoff)
{
    if (p_object) {
        led_onoff_param_t* p_param = (led_onoff_param_t*)htd_malloc(sizeof(led_onoff_param_t));
        ASSERT(p_param);
        p_param->p_object = p_object;
        p_param->led_idx = led_idx;
        p_param->onoff = onoff;
        if (!led_load_func(led_charlie_onoff_handle, p_param)) {
            ASSERT(0);
        }
    }
    return 0;
}

// @brief 显示灯序列号
void htd_led_charlie_show(led_charlie_t* p_object)
{
#if defined(DEBUG_LVL) && (DEBUG_LVL == 4)
    uint32_t i, j, pos;
    print_debug("LED index:");
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
                htd_print(" %3u, ", p_object->led_idx_table[pos]);
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
    #define PIN_1      PIN_NUM_GET(A, 8)
    #define PIN_2      PIN_NUM_GET(A, 9)
    #define PIN_3      PIN_NUM_GET(A, 10)
    #define PIN_4      PIN_NUM_GET(A, 11)
    #define PIN_5      PIN_NUM_GET(A, 12)
    #define PIN_6      PIN_NUM_GET(A, 15)
#endif

static const pin_num_t charlie_pin[] = {PIN_1, PIN_2, PIN_3, PIN_4, PIN_5, PIN_6};
// 定义矩阵按键对象句柄
static led_charlie_t* m_led_charlie = NULL;

// @brief 调试命令
static int led_charlie_cmd(int argc, char **argv)
{
    if (argc < 2) {
        print_error("Usage: %s start/stop/show/on/off", argv[0]);
        return 0;
    }
    if (htd_memcmp(argv[1], "start", 6) == 0) {
        if (m_led_charlie == NULL) {
            m_led_charlie = htd_led_charlie_create(charlie_pin, ARRAY_COUNT(charlie_pin));
            ASSERT(m_led_charlie);
        }
        htd_led_charlie_start(m_led_charlie);
        
    } else if (htd_memcmp(argv[1], "stop", 5) == 0) {
        if (m_led_charlie) {
            htd_led_charlie_stop(m_led_charlie);
            htd_led_charlie_destroy(m_led_charlie);
            m_led_charlie = NULL;
        }
        
    } else if (htd_memcmp(argv[1], "on", 3) == 0) {
        if (argc != 3) {
            print_error("Usage: %s on [index]", argv[0]);
            return -1;
        }
        uint32_t led_idx = atoi(argv[2]);
        print_debug("LED on: %d", led_idx);
        htd_led_charlie_onoff(m_led_charlie, led_idx, true);
        
    } else if (htd_memcmp(argv[1], "off", 4) == 0) {
        if (argc != 3) {
            print_error("Usage: %s on [index]", argv[0]);
            return -1;
        }
        uint32_t led_idx = atoi(argv[2]);
        htd_led_charlie_onoff(m_led_charlie, led_idx, false);
        
    } else if (htd_memcmp(argv[1], "show", 5) == 0) {
        if (m_led_charlie) {
            htd_led_charlie_show(m_led_charlie);
        }
        
    } else if (htd_memcmp(argv[1], "test", 5) == 0) {
        led_pin_output_pp_set(PIN_1);
        led_pin_write(PIN_1, 0);
        led_pin_output_pp_set(PIN_2);
        led_pin_write(PIN_2, 1);
        led_pin_input_nopull_set(PIN_3);
        //led_pin_write(PIN_3, 1);
    
    } else {
        print_error("Usage: %s start/stop/show/on/off", argv[0]);
    }
    return 0;
}
CONSOLE_CMD_EXPORT(led_charlie_cmd, led_charlie);
#endif

