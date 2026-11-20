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
    static uint8_t led_val = 0xff;
    if (led_val == 0xff) {
        htd_pin_output_pp_set(PIN_NUM_GET(C, 13));
        led_val = 0;
    }
    htd_pin_write(PIN_NUM_GET(C, 13), led_val);
    led_val = led_val ? 0 : 1;
}


int main(void)
{
    // 初始化板子
    app_board_init();

    while (1) {
        app_led_blink();
        htd_delay_ms(1000);
    }

    // 灯闪烁调试
    //led_blink_loop_test();
    //htd_delay_test();

    //htd_task_load_function(load_func_test, NULL);
    htd_task_sch_loop(app_sleep_func);
    return 0;
}





