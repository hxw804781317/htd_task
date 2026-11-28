#define DEBUG_LVL           4
#define DEBUG_TAG           "board"
#include "app_board.h"
#include "stm32_assert.h"

#if defined(BSP_USING_NONOS)
//////////////////////////////////////////////////////////////////////
// @brief 获取系统时间，单位毫秒
uint32_t htd_time_ms_get(void)
{
#if 0
    extern uint32_t HAL_GetTick(void);
    uint32_t time_ms = HAL_GetTick();
    if (time_ms == 0) {
        time_ms = 1;
    }
    return time_ms;
#else
    return htd_pm_cpu_time_ms_get();
#endif
}
#endif /*defined(HTD_USING_NONOS)*/


//////////////////////////////////////////////////////////////////////
// @brief 断言失败回调
void htd_assert_failed(const char* file, uint32_t line)
{
    while (1) {
        print_error("assert: file=%s, line=%d", file, line);
        htd_sleep_ms(1000);
    }
}

// @brief 配置系统时钟
void SystemClock_Config(void)
{
    __HAL_RCC_PWR_CLK_ENABLE();
    HAL_PWR_EnableBkUpAccess();          // 允许访问备份域

    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
#if defined(BSP_CLOCK_USE_XTAL32768)
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSE|RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.LSEState = RCC_LSE_ON;
#else
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.LSIState = RCC_LSI_ON;
#endif
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 25;
    RCC_OscInitStruct.PLL.PLLN = 240;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 4;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK) {
        Error_Handler();
    }

#if 0
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
    PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK) {
        Error_Handler();
    }
#endif
}

// @brief 初始化内存堆
static void app_heap_init(void)
{
#if 0
    static uint8_t m_heap_memory[1024*10];      // 调试先用静态内存
    htd_memheap_dev_t* p_dev = htd_memheap_ff_init(m_heap_memory, sizeof(m_heap_memory));
    ASSERT(p_dev);
    int32_t ret = htd_memheap_sram_dev_add(p_dev);
    ASSERT(ret == 0);
#else
    htd_memheap_dev_t* p_dev;
    int32_t ret;
    uint32_t idx = 0;
    htd_ram_partition_t ram_partition;
    while (1) {
        if (htd_heap_partition_get(&ram_partition, 1, idx++) == 0) {
            break;
        }
        p_dev = htd_memheap_ff_init((void*)ram_partition.addr, ram_partition.size);
        ASSERT(p_dev);
        ret = htd_memheap_sram_dev_add(p_dev);
        ASSERT(ret == 0);
    }
#endif
}

// @brief 初始化板子
void app_board_init(void)
{
    // 初始化hal库
    HAL_Init();
    // 初始化系统时间
    SystemClock_Config();
    // 内存 heap 初始化
    app_heap_init();
    // 初始化控制台, 该函数执行之后就可以打印了
    app_console_init();
    // 初始化RTC
    htd_rtc_init(0xffffffff);
    // 初始化电源管理
    htd_pm_init(SystemClock_Config);
    // 初始化看门狗
    //htd_wdt_init();
    // 初始化任务调度器
    htd_task_sch_init(5);
    // 初始化设备
    htd_dev_init();
    // 控制台开始接收数据
    app_console_read_start();
}

