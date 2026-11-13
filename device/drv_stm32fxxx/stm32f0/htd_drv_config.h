// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_DRV_CONFIG_STM32F0_H_
#define _HTD_DRV_CONFIG_STM32F0_H_

#include "htd_drv_common.h"

#if defined(SOC_SERIES_STM32F0)
#ifdef __cplusplus
extern "C" {
#endif


////////////////////////////////GPIO中断号//////////////////////////////////////
#ifndef EXTI0_IRQn
    #define EXTI0_IRQn                      EXTI0_1_IRQn
#endif
#ifndef EXTI1_IRQn
    #define EXTI1_IRQn                      EXTI0_1_IRQn
#endif
#ifndef EXTI2_IRQn
    #define EXTI2_IRQn                      EXTI2_3_IRQn
#endif
#ifndef EXTI3_IRQn
    #define EXTI3_IRQn                      EXTI2_3_IRQn
#endif
#ifndef EXTI4_IRQn
    #define EXTI4_IRQn                      EXTI4_15_IRQn
#endif
#ifndef EXTI5_IRQn
    #define EXTI5_IRQn                      EXTI4_15_IRQn
#endif
#ifndef EXTI6_IRQn
    #define EXTI6_IRQn                      EXTI4_15_IRQn
#endif
#ifndef EXTI7_IRQn
    #define EXTI7_IRQn                      EXTI4_15_IRQn
#endif
#ifndef EXTI8_IRQn
    #define EXTI8_IRQn                      EXTI4_15_IRQn
#endif
#ifndef EXTI9_IRQn
    #define EXTI9_IRQn                      EXTI4_15_IRQn
#endif
#ifndef EXTI10_IRQn
    #define EXTI10_IRQn                     EXTI4_15_IRQn
#endif
#ifndef EXTI11_IRQn
    #define EXTI11_IRQn                     EXTI4_15_IRQn
#endif
#ifndef EXTI12_IRQn
    #define EXTI12_IRQn                     EXTI4_15_IRQn
#endif
#ifndef EXTI13_IRQn
    #define EXTI13_IRQn                     EXTI4_15_IRQn
#endif
#ifndef EXTI14_IRQn
    #define EXTI14_IRQn                     EXTI4_15_IRQn
#endif
#ifndef EXTI15_IRQn
    #define EXTI15_IRQn                     EXTI4_15_IRQn
#endif




//////////////////////////////////////////////////////////////////////
typedef DMA_Channel_TypeDef DMA_Stream_TypeDef_t;

// 定义 DMA 通道信息
typedef struct {
    DMA_Stream_TypeDef_t* instance;
    IRQn_Type irqn;
} stm32_dma_ch_t;


#ifdef DMA1
    #ifdef DMA1_Channel7
        #define DMA1_Channel_MAX       DMA1_Channel7
    #elif defined(DMA1_Channel6)
        #define DMA1_Channel_MAX       DMA1_Channel6
    #else
        #define DMA1_Channel_MAX       DMA1_Channel5
    #endif
#endif
#ifdef DMA2
    #ifdef DMA2_Channel7
        #define DMA2_Channel_MAX       DMA2_Channel7
    #elif defined(DMA2_Channel6)
        #define DMA2_Channel_MAX       DMA2_Channel6
    #else
        #define DMA2_Channel_MAX       DMA2_Channel5
    #endif
#endif


// @brief DMA1_Channel2 中断WEAK函数
void DMA1_Channel2_IRQHandler(void);
// @brief DMA1_Channel3 中断WEAK函数
void DMA1_Channel3_IRQHandler(void);
// @brief DMA1_Channel4 中断WEAK函数
void DMA1_Channel4_IRQHandler(void);
// @brief DMA1_Channel5 中断WEAK函数
void DMA1_Channel5_IRQHandler(void);
// @brief DMA1_Channel6 中断WEAK函数
void DMA1_Channel6_IRQHandler(void);
// @brief DMA1_Channel7 中断WEAK函数
void DMA1_Channel7_IRQHandler(void);

// @brief DMA2_Channel1 中断WEAK函数
void DMA2_Channel1_IRQHandler(void);
// @brief DMA2_Channel2 中断WEAK函数
void DMA2_Channel2_IRQHandler(void);
// @brief DMA2_Channel3 中断WEAK函数
void DMA2_Channel3_IRQHandler(void);
// @brief DMA2_Channel4 中断WEAK函数
void DMA2_Channel4_IRQHandler(void);
// @brief DMA2_Channel5 中断WEAK函数
void DMA2_Channel5_IRQHandler(void);


#ifndef DMA1_Channel2_IRQn
    #define DMA1_Channel2_IRQn              DMA1_Ch2_3_DMA2_Ch1_2_IRQn
#endif
#ifndef DMA1_Channel3_IRQn
    #define DMA1_Channel3_IRQn              DMA1_Ch2_3_DMA2_Ch1_2_IRQn
#endif
#ifndef DMA1_Channel4_IRQn
    #define DMA1_Channel4_IRQn              DMA1_Ch4_7_DMA2_Ch3_5_IRQn
#endif
#ifndef DMA1_Channel5_IRQn
    #define DMA1_Channel5_IRQn              DMA1_Ch4_7_DMA2_Ch3_5_IRQn
#endif
#ifndef DMA1_Channel6_IRQn
    #define DMA1_Channel6_IRQn              DMA1_Ch4_7_DMA2_Ch3_5_IRQn
#endif
#ifndef DMA1_Channel7_IRQn
    #define DMA1_Channel7_IRQn              DMA1_Ch4_7_DMA2_Ch3_5_IRQn
#endif
#ifndef DMA2_Channel1_IRQn
    #define DMA2_Channel1_IRQn              DMA1_Ch2_3_DMA2_Ch1_2_IRQn
#endif
#ifndef DMA2_Channel2_IRQn
    #define DMA2_Channel2_IRQn              DMA1_Ch2_3_DMA2_Ch1_2_IRQn
#endif
#ifndef DMA2_Channel3_IRQn
    #define DMA2_Channel3_IRQn              DMA1_Ch4_7_DMA2_Ch3_5_IRQn
#endif
#ifndef DMA2_Channel4_IRQn
    #define DMA2_Channel4_IRQn              DMA1_Ch4_7_DMA2_Ch3_5_IRQn
#endif
#ifndef DMA2_Channel5_IRQn
    #define DMA2_Channel5_IRQn              DMA1_Ch4_7_DMA2_Ch3_5_IRQn
#endif


// DMA1-CH2
#if defined(BSP_SPIM1_RX_USING_DMA) && !defined(SPIM1_RX_DMA_INSTANCE)
    #define SPIM1_RX_DMA_IRQHandler         DMA1_Channel2_IRQHandler
    #define SPIM1_RX_DMA_INSTANCE           DMA1_Channel2
    #define SPIM1_RX_DMA_IRQ                DMA1_Channel2_IRQn
    #define SPIM1_RX_DMA_CHANNEL            1
#elif defined(BSP_UART1_TX_USING_DMA) && !defined(UART1_TX_DMA_INSTANCE)
    #define UART1_TX_DMA_IRQHandler         DMA1_Channel2_IRQHandler
    #define UART1_TX_DMA_INSTANCE           DMA1_Channel2
    #define UART1_TX_DMA_IRQ                DMA1_Channel2_IRQn
    #define UART1_TX_DMA_CHANNEL            1
#elif defined(BSP_UART3_TX_USING_DMA) && !defined(UART3_TX_DMA_INSTANCE)
    #define UART3_TX_DMA_IRQHandler         DMA1_Channel2_IRQHandler
    #define UART3_TX_DMA_INSTANCE           DMA1_Channel2
    #define UART3_TX_DMA_IRQ                DMA1_Channel2_IRQn
    #define UART3_TX_DMA_CHANNEL            1
#endif

// DMA1-CH3
#if defined(BSP_SPIM1_TX_USING_DMA) && !defined(SPIM1_TX_DMA_INSTANCE)
    #define SPIM1_TX_DMA_IRQHandler         DMA1_Channel3_IRQHandler
    #define SPIM1_TX_DMA_INSTANCE           DMA1_Channel3
    #define SPIM1_TX_DMA_IRQ                DMA1_Channel3_IRQn
    #define SPIM1_TX_DMA_CHANNEL            1
#elif defined(BSP_UART1_RX_USING_DMA) && !defined(UART1_RX_DMA_INSTANCE)
    #define UART1_RX_DMA_IRQHandler         DMA1_Channel3_IRQHandler
    #define UART1_RX_DMA_INSTANCE           DMA1_Channel3
    #define UART1_RX_DMA_IRQ                DMA1_Channel3_IRQn
    #define UART1_RX_DMA_CHANNEL            1
#elif defined(BSP_UART3_RX_USING_DMA) && !defined(UART3_RX_DMA_INSTANCE)
    #define UART3_RX_DMA_IRQHandler         DMA1_Channel3_IRQHandler
    #define UART3_RX_DMA_INSTANCE           DMA1_Channel3
    #define UART3_RX_DMA_IRQ                DMA1_Channel3_IRQn
    #define UART3_RX_DMA_CHANNEL            1
#endif

// DMA1-CH4
#if defined(BSP_SPIM2_RX_USING_DMA) && !defined(SPIM2_RX_DMA_INSTANCE)
    #define SPIM2_RX_DMA_IRQHandler         DMA1_Channel4_IRQHandler
    #define SPIM2_RX_DMA_INSTANCE           DMA1_Channel4
    #define SPIM2_RX_DMA_IRQ                DMA1_Channel4_IRQn
    #define SPIM2_RX_DMA_CHANNEL            1
#elif defined(BSP_UART2_TX_USING_DMA) && !defined(UART2_TX_DMA_INSTANCE)
    #define UART2_TX_DMA_IRQHandler         DMA1_Channel4_IRQHandler
    #define UART2_TX_DMA_INSTANCE           DMA1_Channel4
    #define UART2_TX_DMA_IRQ                DMA1_Channel4_IRQn
    #define UART2_TX_DMA_CHANNEL            1
#endif

// DMA1-CH5
#if defined(BSP_SPIM2_TX_USING_DMA) && !defined(SPIM2_TX_DMA_INSTANCE)
    #define SPIM2_TX_DMA_IRQHandler         DMA1_Channel5_IRQHandler
    #define SPIM2_TX_DMA_INSTANCE           DMA1_Channel5
    #define SPIM2_TX_DMA_IRQ                DMA1_Channel5_IRQn
    #define SPIM2_TX_DMA_CHANNEL            1
#elif defined(BSP_UART2_RX_USING_DMA) && !defined(UART2_RX_DMA_INSTANCE)
    #define UART2_RX_DMA_IRQHandler         DMA1_Channel5_IRQHandler
    #define UART2_RX_DMA_INSTANCE           DMA1_Channel5
    #define UART2_RX_DMA_IRQ                DMA1_Channel5_IRQn
    #define UART2_RX_DMA_CHANNEL            1
#endif

// DMA1-CH6
#if defined(BSP_UART4_TX_USING_DMA) && !defined(UART4_TX_DMA_INSTANCE)
    #define UART4_TX_DMA_IRQHandler         DMA1_Channel6_IRQHandler
    #define UART4_TX_DMA_INSTANCE           DMA1_Channel6
    #define UART4_TX_DMA_IRQ                DMA1_Channel6_IRQn
    #define UART4_TX_DMA_CHANNEL            1
#endif

// DMA1-CH7
#if defined(BSP_UART4_RX_USING_DMA) && !defined(UART4_RX_DMA_INSTANCE)
    #define UART4_RX_DMA_IRQHandler         DMA1_Channel7_IRQHandler
    #define UART4_RX_DMA_INSTANCE           DMA1_Channel7
    #define UART4_RX_DMA_IRQ                DMA1_Channel7_IRQn
    #define UART4_RX_DMA_CHANNEL            1
#endif



#ifdef __cplusplus
}
#endif
#endif /*defined(SOC_SERIES_STM32F0)*/
#endif /*_HTD_DRV_CONFIG_STM32F0_H_ */

