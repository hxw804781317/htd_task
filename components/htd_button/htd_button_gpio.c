// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL       4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG       "htd_button"
#include "htd_button_gpio.h"

// 按键逻辑结构体
typedef struct {
    uint16_t init_flag;
    uint8_t button_cnt;
    const htd_button_t* p_button;
    htd_button_evt_func_t evt_func;         // 事件回调函数

    htd_button_id_t active_all_id;
    htd_button_id_t active_id;
    uint8_t repeat_count;
    uint8_t delay_report;
    uint16_t continuous_cnt;
    uint32_t active_time_ms;
    uint32_t release_time_ms;
    uint32_t func_exec_bitflag;
} htd_button_handle_t;
#define HTD_BUTTON_INIT_FLAG       0xa98a



// @brief 扫描所有按键，记录按下的按键
static void htd_button_scan_handle(htd_button_handle_t* p_handle)
{
    uint32_t i;
    uint32_t now_ms = TIME_MS_GET();
    p_handle->active_all_id = 0;
    for (i=0; i<p_handle->button_cnt; ++i) {
        if (p_handle->p_button[i].is_pressed_handle()) {
            p_handle->active_all_id |= p_handle->p_button[i].button_id;
        }
    }
    if ((p_handle->active_all_id != p_handle->active_id) && 
        ((p_handle->active_all_id & p_handle->active_id) == p_handle->active_id)) {
        print_warning("active_all_id=0x%08x, 0x%08x", p_handle->active_all_id, p_handle->active_id);
        p_handle->active_id = p_handle->active_all_id;
        p_handle->repeat_count = 0;
        p_handle->delay_report = 0;
        p_handle->active_time_ms = now_ms;
        p_handle->release_time_ms = 0;
        p_handle->func_exec_bitflag = 0;
        
        // 判断是否需要延迟
        const htd_button_t* p_button = NULL;
        for (i=0; i<p_handle->button_cnt; ++i) {
            if (p_handle->p_button[i].button_id == p_handle->active_id) {
                p_button = &p_handle->p_button[i];
                break;
            }
        }
        if (p_button) {
            for (i=0; i<p_button->func_count; ++i) {
                if (p_button->p_func[i].evt == HTD_BUTTON_EV_TYPE_REPEAT) {
                    p_handle->delay_report = true;
                    break;
                }
            }
        }
    }
}

// @brief 初始化所有按键
// @param p_config 配置信息，该配置信息必须为全局变量
// @param p_button 需要先配置好对应的按键信息
// @return 成功返回对应的按键句柄，失败返回空
void* htd_button_handle_create(const htd_button_t* p_button, uint8_t count, htd_button_evt_func_t evt_func)
{
    ASSERT(p_button);
    ASSERT(count && count <= (sizeof(htd_button_id_t) * 8));
    ASSERT(evt_func);
    
    htd_button_handle_t* p_handle = (htd_button_handle_t*)htd_malloc(sizeof(htd_button_handle_t));
    ASSERT(p_handle);
    htd_memset(p_handle, 0, sizeof(htd_button_handle_t));
    p_handle->p_button = p_button;
    p_handle->button_cnt = count;
    p_handle->evt_func = evt_func;
    p_handle->init_flag = HTD_BUTTON_INIT_FLAG;
    htd_button_scan_handle(p_handle);
    return p_handle;
}

// @brief 销毁按键句柄
void htd_button_handle_destroy(void* p_handle)
{
    htd_button_handle_t* p_button = (htd_button_handle_t*)p_handle;
    if (p_button) {
        htd_free(p_handle);
    }
}

// @brief 按下事件
static bool htd_button_pressed_evt(htd_button_handle_t* p_handle, const htd_button_t* p_button)
{
    uint32_t now_ms = TIME_MS_GET();
    if (p_handle->release_time_ms || p_handle->active_time_ms == 0) {
        p_handle->func_exec_bitflag = 0;
        p_handle->release_time_ms = 0;
        p_handle->continuous_cnt = 0;
        p_handle->active_time_ms = now_ms;
    }

    bool is_loop = false;
    uint32_t timeout_ms;
    for (uint32_t i=0; i<p_button->func_count; ++i) {
        if (p_handle->func_exec_bitflag & __BIT(i)) {
            continue;
        }
        is_loop = true;
        if (p_handle->repeat_count > 1) {
            if (p_button->p_func[i].evt == HTD_BUTTON_EV_TYPE_REPEAT) {
                p_handle->repeat_count++;
                p_handle->func_exec_bitflag |= __BIT(i);
            }
            continue;
        }
        
        if (p_button->p_func[i].evt == HTD_BUTTON_EV_TYPE_PCPD) {
            // 按下去计时，按下去执行
            timeout_ms = TIME_MS_DIFF(now_ms, p_handle->active_time_ms);
            print_debug("pressed: PCPD. i=%d, timeout_ms=%d", i, timeout_ms);
            if (timeout_ms >= p_button->p_func[i].pcpd_press_ms) {
                p_handle->evt_func(p_button->button_id, i);
                p_handle->func_exec_bitflag |= __BIT(i);
            }
        } else if (p_button->p_func[i].evt == HTD_BUTTON_EV_TYPE_REPEAT) {
            print_debug("repeat: count=%d", p_handle->repeat_count);
            p_handle->repeat_count++;
            p_handle->func_exec_bitflag |= __BIT(i);
        } else if (p_button->p_func[i].evt == HTD_BUTTON_EV_TYPE_PRESS_HOLD) {
            // 按下去不断上报事件
            timeout_ms = (p_handle->continuous_cnt + 1) * p_button->p_func[i].press_hold_timeout_ms;
            print_debug("pressed: PRESS HOLD. i=%d, timeout_ms=%d", i, timeout_ms);
            if (TIME_MS_DIFF(now_ms, p_handle->active_time_ms) > timeout_ms) {
                p_handle->evt_func(p_button->button_id, i);
                p_handle->continuous_cnt++;
            }
        } else {
            ;
        }
    }
    return is_loop;
}

// @brief 弹起事件
static bool htd_button_release_evt(htd_button_handle_t* p_handle, const htd_button_t* p_button)
{
    uint32_t now_ms = TIME_MS_GET();
    if (p_handle->release_time_ms == 0) {
        p_handle->release_time_ms = now_ms;
        p_handle->func_exec_bitflag = 0;
        p_handle->continuous_cnt = 0;
    }

    bool is_loop = false;
    uint32_t timeout_ms;
    for (uint32_t i=0; i<p_button->func_count; ++i) {
        if (p_handle->func_exec_bitflag & __BIT(i)) {
            continue;
        }
        if (p_handle->repeat_count > 1) {
            if (p_button->p_func[i].evt == HTD_BUTTON_EV_TYPE_REPEAT) { // 重复事件
                timeout_ms = TIME_MS_DIFF(now_ms, p_handle->release_time_ms);
                print_debug("release: repeat. i=%d, timeout_ms=%d", i, timeout_ms);
                if (timeout_ms >= p_button->p_func[i].repeat_inter_ms) {
                    if (p_button->p_func[i].repeat_count == p_handle->repeat_count) {
                        p_handle->evt_func(p_button->button_id, i);
                        p_handle->func_exec_bitflag |= __BIT(i);
                    }
                } else {
                    is_loop = true;
                }
            } 
            continue;
        }

        if (p_button->p_func[i].evt == HTD_BUTTON_EV_TYPE_PCBD) {         // 按下去计时，弹起执行 
            timeout_ms = TIME_MS_DIFF(p_handle->release_time_ms, p_handle->active_time_ms);
            print_debug("release: PCBD. i=%d, timeout_ms=%d", i, timeout_ms);
            if (timeout_ms >= p_button->p_func[i].pcbd_press_ms && timeout_ms < p_button->p_func[i].pcbd_timeout_ms) {
                timeout_ms = HTD_BUTTON_REPEAT_INTER_TIME_MS+100;
                if (p_handle->delay_report) {
                    is_loop = true;
                    timeout_ms = TIME_MS_DIFF(now_ms, p_handle->release_time_ms);
                }
                if (timeout_ms > HTD_BUTTON_REPEAT_INTER_TIME_MS+50) {
                    p_handle->evt_func(p_button->button_id, i);
                    p_handle->func_exec_bitflag |= __BIT(i);
                }
            } else {
                p_handle->func_exec_bitflag |= __BIT(i);
            }
        } else if (p_button->p_func[i].evt == HTD_BUTTON_EV_TYPE_BCBD) {  // 弹起计时，弹起执行
            timeout_ms = TIME_MS_DIFF(now_ms, p_handle->release_time_ms);
            print_debug("release: BCBD. i=%d, timeout_ms=%d", i, timeout_ms);
            if (p_handle->delay_report && timeout_ms < HTD_BUTTON_REPEAT_INTER_TIME_MS+50) {
                is_loop = true;
                continue;
            }
            if (timeout_ms >= p_button->p_func[i].bcbd_release_ms) {
                p_handle->evt_func(p_button->button_id, i);
                p_handle->func_exec_bitflag |= __BIT(i);
            } else {
                is_loop = true;
            }
        } 
    }

    if (is_loop == false) {
        p_handle->release_time_ms = 0;
        p_handle->active_time_ms = 0;
        p_handle->repeat_count = 0;
        p_handle->active_id = 0;
        p_handle->func_exec_bitflag = 0;
        p_handle->continuous_cnt = 0;
    }
    return is_loop;
}

// @brief 按键轮询探测句柄
static void htd_button_loop_handle(void* arg) 
{
    htd_button_handle_t* p_handle = (htd_button_handle_t*)arg;
    if (p_handle == NULL || p_handle->init_flag != HTD_BUTTON_INIT_FLAG || p_handle->p_button == NULL) {
        return;
    }
    
    htd_button_scan_handle(p_handle);
    if (p_handle->active_time_ms == 0 || p_handle->active_id == 0) {
        return;
    }
    
    const htd_button_t* p_button = NULL;
    for (uint32_t i=0; i<p_handle->button_cnt; ++i) {
        if (p_handle->p_button[i].button_id == p_handle->active_id) {
            p_button = &p_handle->p_button[i];
            break;
        }
    }
    if (p_button == NULL) {
        p_handle->active_id = 0;
        return;
    }

    bool is_loop = true;
    if (p_button->is_pressed_handle()) {
        is_loop = htd_button_pressed_evt(p_handle, p_button);
    } else {
        is_loop = htd_button_release_evt(p_handle, p_button);
    }
    if (is_loop) {
        button_load_timer_func(false, 10, htd_button_loop_handle, arg);
    }
}

// @brief 按键事件调用接口，比如按键中断来了，就调用这个接口
// @param p_handle 按键句柄，htd_button_create的返回值
// @node  这里面会调用 htd_button_load_timer_function 函数，如果中断
//        不可以调用该函数，需要将中断事件抛到线程中再调用 htd_button_process;
void htd_button_process(void* p_handle)
{
    htd_button_handle_t* p_button = (htd_button_handle_t*)p_handle;
    ASSERT(p_button);
    button_load_timer_func(false, 10, htd_button_loop_handle, p_handle);
}











