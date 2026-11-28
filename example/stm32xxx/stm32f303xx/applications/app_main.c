#define DEBUG_LVL 4
#define DEBUG_TAG              "main"
#include "app_board.h"
#include "htd_task.h"




//////////////////////////////////////////////////////////////////////
// @brief 模拟休眠函数
uint32_t app_sleep_func(uint32_t sleep_ms)
{
#if 0
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


#if defined(BSP_USING_FREERTOS)
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
        uint32_t sleep_ms = xExpectedIdleTime * 1000 / configTICK_RATE_HZ;
        if (sleep_ms > 5) {
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

void led_blink_loop_test(void)
{
    htd_pin_output_pp_set(PIN_NUM_GET(C, 13));
    uint8_t led_val = 0;
    htd_pin_write(PIN_NUM_GET(C, 13), led_val);
    while (1) {
        led_val = led_val ? 0 : 1;
        htd_pin_write(PIN_NUM_GET(C, 13), led_val);
        HAL_Delay(1000);
        print_warning("blink led.");
    }
}

// @brief 
void app_led_blink(void)
{
    static uint8_t led_val = 0xff;
    if (led_val == 0xff) {
        htd_pin_output_pp_set(PIN_NUM_GET(C, 13));
        led_val = 0;
    }
    htd_pin_write(PIN_NUM_GET(C, 13), led_val);
    led_val = led_val ? 0 : 1;
}

void load_func_test(void* arg)
{
    htd_time_t time = htd_rtc_time_get();
    print_warning("-->time_ms=%u, %04d-%02d-%02d %02d:%02d:%02d", htd_time_ms_get(), time.year, time.month, time.day, time.hour, time.min, time.second);
    htd_task_load_timer_function(false, 1000, load_func_test, NULL);
    app_led_blink();
}

htd_sem_t* blink_sem = NULL;
void led_blink_handle(void* arg)
{
    while (1) {
        //htd_sem_reset(blink_sem);
        //htd_sem_take(blink_sem, 1000);
        htd_delay_ms(1000);
        print_debug("now_ms=%u", htd_time_ms_get());
        //app_led_blink();
    }
}

int main(void)
{
    // 初始化板子
    app_board_init();
    print_debug("----main---");
    
    // 灯闪烁调试
    //led_blink_loop_test();
    //htd_delay_test();

    //htd_task_load_function(load_func_test, NULL);
    htd_task_sch_loop(app_sleep_func);
    return 0;
}





