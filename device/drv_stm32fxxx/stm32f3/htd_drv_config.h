#ifndef _HTD_DRV_CONFIG_STM32F3XX_H_
#define _HTD_DRV_CONFIG_STM32F3XX_H_

#include "htd_drv_gpio.h"

#if defined(SOC_SERIES_STM32F3)
#ifdef __cplusplus
extern "C" {
#endif


////////////////////////////////GPIO中断号//////////////////////////////////////
#ifndef EXTI2_IRQn
    #define EXTI2_IRQn                      EXTI2_TSC_IRQn
#endif
#ifndef EXTI5_IRQn
    #define EXTI5_IRQn                      EXTI9_5_IRQn
#endif
#ifndef EXTI6_IRQn
    #define EXTI6_IRQn                      EXTI9_5_IRQn
#endif
#ifndef EXTI7_IRQn
    #define EXTI7_IRQn                      EXTI9_5_IRQn
#endif
#ifndef EXTI8_IRQn
    #define EXTI8_IRQn                      EXTI9_5_IRQn
#endif
#ifndef EXTI9_IRQn
    #define EXTI9_IRQn                      EXTI9_5_IRQn
#endif
#ifndef EXTI10_IRQn
    #define EXTI10_IRQn                     EXTI15_10_IRQn
#endif
#ifndef EXTI11_IRQn
    #define EXTI11_IRQn                     EXTI15_10_IRQn
#endif
#ifndef EXTI12_IRQn
    #define EXTI12_IRQn                     EXTI15_10_IRQn
#endif
#ifndef EXTI13_IRQn
    #define EXTI13_IRQn                     EXTI15_10_IRQn
#endif
#ifndef EXTI14_IRQn
    #define EXTI14_IRQn                     EXTI15_10_IRQn
#endif
#ifndef EXTI15_IRQn
    #define EXTI15_IRQn                     EXTI15_10_IRQn
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
        #define DMA1_Channel_MAX            DMA1_Channel7
    #elif defined(DMA1_Channel6)
        #define DMA1_Channel_MAX            DMA1_Channel6
    #else
        #define DMA1_Channel_MAX            DMA1_Channel5
    #endif
#endif
#ifdef DMA2
    #ifdef DMA2_Channel7
        #define DMA2_Channel_MAX            DMA2_Channel7
    #elif defined(DMA2_Channel6)
        #define DMA2_Channel_MAX            DMA2_Channel6
    #else
        #define DMA2_Channel_MAX            DMA2_Channel5
    #endif
#endif


// DMA1-Ch2
#if defined(BSP_SPIM3_RX_USING_DMA) && !defined(SPIM3_RX_DMA_INSTANCE)
    #define SPIM3_RX_DMA_IRQHandler         DMA1_Channel2_IRQHandler
    #define SPIM3_RX_DMA_INSTANCE           DMA1_Channel2
    #define SPIM3_RX_DMA_IRQ                DMA1_Channel2_IRQn
    #define SPIM3_RX_DMA_CHANNEL            1
#elif defined(BSP_UART3_TX_USING_DMA) && !defined(UART3_TX_DMA_INSTANCE)
    #define UART3_TX_DMA_IRQHandler         DMA1_Channel2_IRQHandler
    #define UART3_TX_DMA_INSTANCE           DMA1_Channel2
    #define UART3_TX_DMA_CHANNEL            DMA1_Channel2_BASE
    #define UART3_TX_DMA_IRQ                DMA1_Channel2_IRQn
#endif

// DMA1-Ch3
#if defined(BSP_SPIM3_TX_USING_DMA) && !defined(SPIM3_TX_DMA_INSTANCE)
    #define SPIM3_TX_DMA_IRQHandler         DMA1_Channel3_IRQHandler
    #define SPIM3_TX_DMA_INSTANCE           DMA1_Channel3
    #define SPIM3_TX_DMA_IRQ                DMA1_Channel3_IRQn
    #define SPIM3_TX_DMA_CHANNEL            1
#elif defined(BSP_UART3_RX_USING_DMA) && !defined(UART3_RX_DMA_INSTANCE)
    #define UART3_RX_DMA_IRQHandler         DMA1_Channel3_IRQHandler
    #define UART3_RX_DMA_INSTANCE           DMA1_Channel3
    #define UART3_RX_DMA_CHANNEL            DMA1_Channel3_BASE
    #define UART3_RX_DMA_IRQ                DMA1_Channel3_IRQn
#endif

// DMA1-Ch4
#if defined(BSP_SPIM2_RX_USING_DMA) && !defined(SPIM2_RX_DMA_INSTANCE)
    #define SPIM2_RX_DMA_IRQHandler         DMA1_Channel4_IRQHandler
    #define SPIM2_RX_DMA_INSTANCE           DMA1_Channel4
    #define SPIM2_RX_DMA_IRQ                DMA1_Channel4_IRQn
    #define SPIM2_RX_DMA_CHANNEL            1
#elif defined(BSP_UART1_TX_USING_DMA) && !defined(UART1_TX_DMA_INSTANCE)
    #define UART1_TX_DMA_IRQHandler         DMA1_Channel4_IRQHandler
    #define UART1_TX_DMA_INSTANCE           DMA1_Channel4
    #define UART1_TX_DMA_CHANNEL            DMA1_Channel4_BASE
    #define UART1_TX_DMA_IRQ                DMA1_Channel4_IRQn
#endif

// DMA1-Ch5
#if defined(BSP_SPIM2_TX_USING_DMA) && !defined(SPIM2_TX_DMA_INSTANCE)
    #define SPIM2_TX_DMA_IRQHandler         DMA1_Channel5_IRQHandler
    #define SPIM2_TX_DMA_INSTANCE           DMA1_Channel5
    #define SPIM2_TX_DMA_IRQ                DMA1_Channel5_IRQn
    #define SPIM2_TX_DMA_CHANNEL            1
#elif defined(BSP_UART1_RX_USING_DMA) && !defined(UART1_RX_DMA_INSTANCE)
    #define UART1_RX_DMA_IRQHandler         DMA1_Channel5_IRQHandler
    #define UART1_RX_DMA_INSTANCE           DMA1_Channel5
    #define UART1_RX_DMA_CHANNEL            DMA1_Channel5_BASE
    #define UART1_RX_DMA_IRQ                DMA1_Channel5_IRQn
#elif defined(BSP_UART2_RX_USING_DMA) && !defined(UART2_RX_DMA_INSTANCE)
    #define UART2_RX_DMA_IRQHandler         DMA1_Channel5_IRQHandler
    #define UART2_RX_DMA_INSTANCE           DMA1_Channel5
    #define UART2_RX_DMA_CHANNEL            DMA1_Channel5_BASE
    #define UART2_RX_DMA_IRQ                DMA1_Channel5_IRQn
#endif

// DMA1-Ch6
#if defined(BSP_UART2_RX_USING_DMA) && !defined(UART2_RX_DMA_INSTANCE)
    #define UART2_RX_DMA_IRQHandler          DMA1_Channel6_IRQHandler
    #define UART2_RX_DMA_INSTANCE            DMA1_Channel6
    #define UART2_RX_DMA_CHANNEL             DMA1_Channel6_BASE
    #define UART2_RX_DMA_IRQ                 DMA1_Channel6_IRQn
#endif

// DMA1-Ch7
#if defined(BSP_UART2_TX_USING_DMA) && !defined(UART2_TX_DMA_INSTANCE)
    #define UART2_TX_DMA_IRQHandler          DMA1_Channel7_IRQHandler
    #define UART2_TX_DMA_INSTANCE            DMA1_Channel7
    #define UART2_TX_DMA_CHANNEL             DMA1_Channel7_BASE
    #define UART2_TX_DMA_IRQ                 DMA1_Channel7_IRQn
#endif


#ifdef __cplusplus
}
#endif
#endif /*defined(SOC_SERIES_STM32F3)*/
#endif /*_HTD_DRV_CONFIG_STM32F3XX_H_ */

