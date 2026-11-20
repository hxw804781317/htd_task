#include "app_board.h"

// @brief 
void HAL_MspInit(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    //__HAL_RCC_GPIOF_CLK_ENABLE();
    //__HAL_RCC_GPIOG_CLK_ENABLE();

    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();

    // 初始化系统中断
    // 禁用 JTAG-DP; 使能 SW-DP
    //__HAL_AFIO_REMAP_SWJ_NOJTAG();
}


