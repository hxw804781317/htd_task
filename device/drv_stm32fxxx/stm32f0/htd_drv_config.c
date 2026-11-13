// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
//#define DEBUG_LVL   4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG   "drv.config"
#include "htd_device.h"
#include "htd_drv_common.h"
#include "htd_drv_dma.h"


#ifdef SOC_SERIES_STM32F0
//////////////////////////////////////////////////////////////////////
__WEAK void EXTI0_IRQHandler(void){}
__WEAK void EXTI1_IRQHandler(void){}
__WEAK void EXTI2_IRQHandler(void){}
__WEAK void EXTI3_IRQHandler(void){}
__WEAK void EXTI4_IRQHandler(void){}
__WEAK void EXTI5_IRQHandler(void){}
__WEAK void EXTI6_IRQHandler(void){}
__WEAK void EXTI7_IRQHandler(void){}
__WEAK void EXTI8_IRQHandler(void){}
__WEAK void EXTI9_IRQHandler(void){}
__WEAK void EXTI10_IRQHandler(void){}
__WEAK void EXTI11_IRQHandler(void){}
__WEAK void EXTI12_IRQHandler(void){}
__WEAK void EXTI13_IRQHandler(void){}
__WEAK void EXTI14_IRQHandler(void){}
__WEAK void EXTI15_IRQHandler(void){}


// @brief GPIO0/GPIO1 中断
void EXTI0_1_IRQHandler(void)
{
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_0)) {
        EXTI0_IRQHandler();
    }
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_1)) {
        EXTI1_IRQHandler();
    }
}

// @brief GPIO2/GPIO3 中断
void EXTI2_3_IRQHandler(void)
{
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_2)) {
        EXTI2_IRQHandler();
    }
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_3)) {
        EXTI3_IRQHandler();
    }
}

// @brief GPIO4~GPIO15 中断
void EXTI4_15_IRQHandler(void)
{
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_4)) {
        EXTI4_IRQHandler();
    }
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_5)) {
        EXTI5_IRQHandler();
    }
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_6)) {
        EXTI6_IRQHandler();
    }
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_7)) {
        EXTI7_IRQHandler();
    }
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_8)) {
        EXTI8_IRQHandler();
    }
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_9)) {
        EXTI9_IRQHandler();
    }
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_10)) {
        EXTI10_IRQHandler();
    }
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_11)) {
        EXTI11_IRQHandler();
    }
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_12)) {
        EXTI12_IRQHandler();
    }
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_13)) {
        EXTI13_IRQHandler();
    }
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_14)) {
        EXTI14_IRQHandler();
    }
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_15)) {
        EXTI15_IRQHandler();
    }
}



//////////////////////////////////////////////////////////////////////
// @brief DMA1_Channel2 中断WEAK函数
__WEAK void DMA1_Channel2_IRQHandler(void) {}
// @brief DMA1_Channel3 中断WEAK函数
__WEAK void DMA1_Channel3_IRQHandler(void){}
// @brief DMA1_Channel4 中断WEAK函数
__WEAK void DMA1_Channel4_IRQHandler(void){}
// @brief DMA1_Channel5 中断WEAK函数
__WEAK void DMA1_Channel5_IRQHandler(void){}
// @brief DMA1_Channel6 中断WEAK函数
__WEAK void DMA1_Channel6_IRQHandler(void){}
// @brief DMA1_Channel7 中断WEAK函数
__WEAK void DMA1_Channel7_IRQHandler(void){}
#if defined(DMA2)
// @brief DMA2_Channel1 中断WEAK函数
__WEAK void DMA2_Channel1_IRQHandler(void){}
// @brief DMA2_Channel2 中断WEAK函数
__WEAK void DMA2_Channel2_IRQHandler(void){}
// @brief DMA2_Channel3 中断WEAK函数
__WEAK void DMA2_Channel3_IRQHandler(void){}
// @brief DMA2_Channel4 中断WEAK函数
__WEAK void DMA2_Channel4_IRQHandler(void){}
// @brief DMA2_Channel5 中断WEAK函数
__WEAK void DMA2_Channel5_IRQHandler(void){}
#endif



// @brief 
void DMA1_Ch2_3_DMA2_Ch1_2_IRQHandler(void)
{
#if defined(DMA1)
    print_debug("DAM1->ISR=0x%08x", DMA1->ISR);
    if (DMA1->ISR & (DMA_ISR_HTIF2|DMA_ISR_TCIF2|DMA_ISR_HTIF2|DMA_ISR_TEIF2)) {
        DMA1_Channel2_IRQHandler();
    }
    if (DMA1->ISR & (DMA_ISR_HTIF3|DMA_ISR_TCIF3|DMA_ISR_HTIF3|DMA_ISR_TEIF3)) {
        DMA1_Channel3_IRQHandler();
    }
#endif
#if defined(DMA2)
    print_debug("DAM2->ISR=0x%08x", DMA2->ISR);
    if (DMA2->ISR & (DMA_ISR_HTIF1|DMA_ISR_TCIF1|DMA_ISR_HTIF1|DMA_ISR_TEIF1)) {
        DMA2_Channel1_IRQHandler();
    }
    if (DMA2->ISR & (DMA_ISR_HTIF2|DMA_ISR_TCIF2|DMA_ISR_HTIF2|DMA_ISR_TEIF2)) {
        DMA2_Channel1_IRQHandler();
    }
#endif
}

// @brief 
void DMA1_Ch4_7_DMA2_Ch3_5_IRQHandler(void)
{
#if defined(DMA1)
    print_debug("DAM1->ISR=0x%08x", DMA1->ISR);
    if (DMA1->ISR & (DMA_ISR_HTIF4|DMA_ISR_TCIF4|DMA_ISR_HTIF4|DMA_ISR_TEIF4)) {
        DMA1_Channel4_IRQHandler();
    }
    if (DMA1->ISR & (DMA_ISR_HTIF5|DMA_ISR_TCIF5|DMA_ISR_HTIF5|DMA_ISR_TEIF5)) {
        DMA1_Channel5_IRQHandler();
    }
    #ifdef DMA1_Channel6
        if (DMA1->ISR & (DMA_ISR_HTIF6|DMA_ISR_TCIF6|DMA_ISR_HTIF6|DMA_ISR_TEIF6)) {
            DMA1_Channel6_IRQHandler();
        }
    #endif
    #ifdef DMA1_Channel7
        if (DMA1->ISR & (DMA_ISR_HTIF7|DMA_ISR_TCIF7|DMA_ISR_HTIF7|DMA_ISR_TEIF7)) {
            DMA1_Channel7_IRQHandler();
        }
    #endif
#endif
#if defined(DMA2)
    print_debug("DAM2->ISR=0x%08x", DMA2->ISR);
    if (DMA2->ISR & (DMA_ISR_HTIF3|DMA_ISR_TCIF3|DMA_ISR_HTIF3|DMA_ISR_TEIF3)) {
        DMA2_Channel3_IRQHandler();
    }
    if (DMA2->ISR & (DMA_ISR_HTIF4|DMA_ISR_TCIF4|DMA_ISR_HTIF4|DMA_ISR_TEIF4)) {
        DMA2_Channel4_IRQHandler();
    }
    if (DMA2->ISR & (DMA_ISR_HTIF5|DMA_ISR_TCIF5|DMA_ISR_HTIF5|DMA_ISR_TEIF5)) {
        DMA2_Channel5_IRQHandler();
    }
#endif
}
#endif /*SOC_SERIES_STM32F0*/

