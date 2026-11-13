#define DEBUG_LVL 4
#define DEBUG_TAG              "main"
#include "app_board.h"



//////////////////////////////////////////////////////////////////////
#if defined(HTD_TASK_NONOS)
// @brief 获取系统时间，单位毫秒
uint32_t htd_time_ms_get(void)
{
    extern uint32_t HAL_GetTick(void);
    uint32_t time_ms = HAL_GetTick();
    if (time_ms == 0) {
        time_ms = 1;
    }
    return time_ms;
}
#endif /*defined(HTD_USING_NONOS)*/

// @brief 延迟us
void htd_delay_us(uint32_t us)
{
    if (us == 0) {
        return ;
    }

    extern uint32_t SystemCoreClock;
    uint32_t time_cycles = us * (SystemCoreClock / 1000000);

    // 保存并启动 DWT
    uint32_t core_debug = CoreDebug->DEMCR;
    CoreDebug->DEMCR = core_debug | CoreDebug_DEMCR_TRCENA_Msk;
    // Save the current state of the CTRL register in the DWT block. Make sure
    // that the cycle counter is enabled.
    uint32_t dwt_ctrl = DWT->CTRL;
    DWT->CTRL = dwt_ctrl | DWT_CTRL_CYCCNTENA_Msk;
    // Store start value of the cycle counter.
    uint32_t cyccnt_initial = DWT->CYCCNT;
    // 等待延迟的时间
    while ((DWT->CYCCNT - cyccnt_initial) < time_cycles){}
    // 恢复原始状态
    DWT->CTRL = dwt_ctrl;
    CoreDebug->DEMCR = core_debug;
}

// @brief 延迟毫秒
void htd_delay_ms(uint32_t ms)
{
    while (ms > 5000) {
        htd_delay_us(5000*1000);
        ms -= 5000;
    }
    htd_delay_us(ms * 1000);
}



//////////////////////////////////////////////////////////////////////
// @brief 重启设备
void htd_reboot(void)
{
    NVIC_SystemReset();
    while (1);
}

// @brief 配置系统时钟
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    // 设置 PLL 的时钟源为外部时钟 HSE
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;        // 倍频9，8M*9=72M
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }
    
    /** Initializes the CPU, AHB and APB buses clocks */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK){
        Error_Handler();
    }
    
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC|RCC_PERIPHCLK_USB;
    PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
    PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLL_DIV1_5;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK){
        Error_Handler();
    }
}

// @brief 初始化内存堆
void app_heap_init(void)
{
    static uint8_t m_heap_memory[1024*10];      // 调试先用静态内存
    htd_memheap_dev_t* p_dev = htd_memheap_ff_init(m_heap_memory, sizeof(m_heap_memory));
    ASSERT(p_dev);
    int32_t ret = htd_memheap_sram_dev_add(p_dev);
    ASSERT(ret == 0);
}

