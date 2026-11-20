#include "htd_device.h"
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
    if (hpcd->Instance != USB_OTG_FS) {
        return;
    }
    
    // FIXME: 部分芯片需要指定时钟源，这里需要做对应的修改
    
    // 使能时钟
    __HAL_RCC_GPIOA_CLK_ENABLE();
    
    // 配置 PA11 (DM) 和 PA12 (DP)
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_11 | GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;             // 复用推挽
    GPIO_InitStruct.Pull = GPIO_NOPULL;                 // 不使用内部上下拉
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;  // 高速
    GPIO_InitStruct.Alternate = GPIO_AF10_OTG_FS;       // AF10 复用功能
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_USB_OTG_FS_CLK_ENABLE();

    // 禁用 VBUS 检测
    //hpcd->Instance->GCCFG &= ~USB_OTG_GCCFG_VBDEN;
    //hpcd->Instance->DCTL &= ~USB_OTG_DCTL_SDIS;
    
    HAL_NVIC_SetPriority(OTG_FS_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(OTG_FS_IRQn);
    //HAL_NVIC_SetPriority(OTG_FS_WKUP_IRQn, 1, 0);
    //HAL_NVIC_EnableIRQ(OTG_FS_WKUP_IRQn);
}

/**
* @brief PCD MSP De-Initialization
* This function freeze the hardware resources used in this example
* @param hpcd: PCD handle pointer
* @retval None
*/
void HAL_PCD_MspDeInit(PCD_HandleTypeDef* hpcd)
{
    if (hpcd->Instance != USB_OTG_FS) {
        return;
    }
    
    htd_pin_hiz_set(PIN_NUM_GET(A, 11));
    htd_pin_hiz_set(PIN_NUM_GET(A, 12));
    HAL_NVIC_DisableIRQ(OTG_FS_IRQn);
    HAL_NVIC_DisableIRQ(OTG_FS_WKUP_IRQn);
    __HAL_RCC_USB_OTG_FS_CLK_DISABLE();
}


