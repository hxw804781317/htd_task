
#include "app_board.h"


void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

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

/**
* @brief PCD MSP Initialization
* This function configures the hardware resources used in this example
* @param hpcd: PCD handle pointer
* @retval None
*/
void HAL_PCD_MspInit(PCD_HandleTypeDef* hpcd)
{
    //if (hpcd->Instance == USB) {
        // 使能 USB 时钟
        //__HAL_RCC_USB_CLK_ENABLE();
    //}
}

/**
* @brief PCD MSP De-Initialization
* This function freeze the hardware resources used in this example
* @param hpcd: PCD handle pointer
* @retval None
*/
void HAL_PCD_MspDeInit(PCD_HandleTypeDef* hpcd)
{
    //if (hpcd->Instance == USB) {
        // 禁用 USB 时钟
        //__HAL_RCC_USB_CLK_DISABLE();
    //}
}


