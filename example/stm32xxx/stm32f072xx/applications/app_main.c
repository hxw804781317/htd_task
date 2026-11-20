#define DEBUG_LVL 4
#define DEBUG_TAG              "main"
#include "htd_device.h"
#include "htd_components.h"
#include "app_board.h"




//////////////////////////////////////////////////////////////////////
// @brief 模拟休眠函数
uint32_t app_sleep_func(uint32_t sleep_ms)
{
#if 1
    uint32_t diff_ms;
    uint32_t start_ms = htd_time_ms_get();
    // 判断打印日志是否已经传输完毕
    if (app_console_tx_flush(sleep_ms) > 0) {
        return 0;
    }
    diff_ms = htd_time_ms_diff(htd_time_ms_get(), start_ms);
    if (diff_ms >= sleep_ms) {
        return sleep_ms;
    }

    sleep_ms -= diff_ms;
    return htd_pm_run(sleep_ms);
#else
    return 0;
#endif
}


#if defined(HTD_TASK_FREERTOS)
//////////////////////////////////////////////////////////////////////
// @brief FreeRTOS 空闲任务回调
void vApplicationIdleHook(void)
{
    //print_debug("----idle hook------");
}

// @brief 低功耗休眠函数
void vPortSuppressTicksAndSleep(TickType_t xExpectedIdleTime)
{
#if defined(configUSE_TICKLESS_IDLE) && configUSE_TICKLESS_IDLE != 0
    if (eTaskConfirmSleepModeStatus() != eAbortSleep) {
        uint32_t sleep_ms = pdTICKS_TO_MS(xExpectedIdleTime);
        if (sleep_ms > 3) {
            sleep_ms -= 2;              // 注意：不同的模式，唤醒时长不一样；实测sleep(1毫秒以内)，stop(2毫秒以内)
            app_sleep_func(sleep_ms);
        } 
    }
#endif
}

// @brief 栈溢出回调
void vApplicationStackOverflowHook(TaskHandle_t xTask, char * pcTaskName)
{
    taskDISABLE_INTERRUPTS();
    while (1) {
        print_error("%s stack overflow.", pcTaskName);
        htd_delay_ms(500);
    }
}

// @brief 内存申请失败
void vApplicationMallocFailedHook(void)
{
    while (1) {
        print_error("malloc memory failed.");
        htd_delay_ms(1000);
    }
}
#endif




//////////////////////////////////////////////////////////////////////
void stm32_error_handler(const char *s, int num)
{
    while (1) {
        print_debug("error: file=%s, num=%d", s, num);
        htd_delay_ms(1000);
    }
}

// @brief 
void app_led_blink(void)
{
    pin_num_t led_pin = PIN_NUM_GET(A, 15);
    static uint8_t led_val = 0xff;
    if (led_val == 0xff) {
        htd_pin_output_pp_set(led_pin);
        led_val = 0;
    }
    htd_pin_write(led_pin, led_val);
    led_val = led_val ? 0 : 1;
}


void led_blink_loop_test(void)
{
    while (1) {
        app_led_blink();
        //HAL_Delay(1000);
        htd_delay_ms(1000);
        htd_time_t time = htd_rtc_time_get();
        print_warning("-->time_ms=%u, %04d-%02d-%02d %02d:%02d:%02d", htd_time_ms_get(), time.year, time.month, time.day, time.hour, time.min, time.second);
    }
}


void load_func_test(void* arg)
{
    htd_task_load_timer_function(false, 1000, load_func_test, NULL);
    
    htd_time_t time = htd_rtc_time_get();
    print_warning("-->time_ms=%u, %04d-%02d-%02d %02d:%02d:%02d", htd_time_ms_get(), time.year, time.month, time.day, time.hour, time.min, time.second);
    app_led_blink();
}

int main(void)
{
    // 初始化板子
    app_board_init();
    print_debug("----main---");
    
    // 灯闪烁调试
    //led_blink_loop_test();

    //htd_task_load_function(load_func_test, NULL);
    htd_task_sch_loop(app_sleep_func);
    return 0;
}


