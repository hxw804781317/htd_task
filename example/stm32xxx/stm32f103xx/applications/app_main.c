#define DEBUG_LVL 4
#define DEBUG_TAG              "main"
#include "app_board.h"


//////////////////////////////////////////////////////////////////////
// @brief 模拟休眠函数
uint32_t app_sleep_func(uint32_t sleep_ms)
{
    // 不休眠
    return 0;
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

void load_func_test(void* arg)
{
    htd_task_load_timer_function(false, 1000, load_func_test, NULL);

    static uint8_t PB12_val = 0xff;
    if (PB12_val == 0xff) {
        GPIO_InitTypeDef GPIO_InitStruct;
        GPIO_InitStruct.Pin = GPIO_PIN_12;
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
        PB12_val = 0;
    }
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, PB12_val == 0 ? GPIO_PIN_RESET : GPIO_PIN_SET);
    PB12_val = PB12_val ? 0 : 1;
    print_debug("---load_func_test---%u", htd_time_ms_get());
}

// @brief 主函数
int main(void)
{
    // 初始化hal库
    HAL_Init();
    // 初始化系统时间
    SystemClock_Config();
    // 内存 heap 初始化
    app_heap_init();
    // 初始化任务调度器
    htd_task_sch_init(5);
    // 加载任务
    htd_task_load_function(load_func_test, NULL);
    // 进入任务调度
    htd_task_sch_loop(NULL);
    return 0;
}

