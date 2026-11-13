#ifndef _HTD_DRV_CONFIG_STM32F2XX_H_
#define _HTD_DRV_CONFIG_STM32F2XX_H_

#include "htd_drv_gpio.h"

#if defined(SOC_SERIES_STM32F2)
#ifdef __cplusplus
extern "C" {
#endif


////////////////////////////////GPIO中断号//////////////////////////////////////
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
typedef DMA_Stream_TypeDef DMA_Stream_TypeDef_t;

// 定义 DMA 通道信息
typedef struct {
    DMA_Stream_TypeDef_t* instance;
    IRQn_Type irqn;
} stm32_dma_ch_t;


/* DMA1 stream0 */
#if defined(BSP_SPIM3_RX_USING_DMA) && !defined(SPIM3_RX_DMA_INSTANCE)
    #define SPIM3_RX_DMA_IRQHandler         DMA1_Stream0_IRQHandler
    #define SPIM3_RX_DMA_INSTANCE           DMA1_Stream0
    #define SPIM3_RX_DMA_CHANNEL            DMA_CHANNEL_0
    #define SPIM3_RX_DMA_IRQ                DMA1_Stream0_IRQn
#elif defined(BSP_UART5_RX_USING_DMA) && !defined(UART5_RX_DMA_INSTANCE)
    #define UART5_RX_DMA_IRQHandler         DMA1_Stream0_IRQHandler
    #define UART5_RX_DMA_INSTANCE           DMA1_Stream0
    #define UART5_RX_DMA_CHANNEL            DMA_CHANNEL_4
    #define UART5_RX_DMA_IRQ                DMA1_Stream0_IRQn
#endif

/* DMA1 stream1 */
#if defined(BSP_UART3_RX_USING_DMA) && !defined(UART3_RX_DMA_INSTANCE)
    #define UART3_RX_DMA_IRQHandler         DMA1_Stream1_IRQHandler
    #define UART3_RX_DMA_INSTANCE           DMA1_Stream1
    #define UART3_RX_DMA_CHANNEL            DMA_CHANNEL_4
    #define UART3_RX_DMA_IRQ                DMA1_Stream1_IRQn
#endif

/* DMA1 stream2 */
#if defined(BSP_SPIM3_RX_USING_DMA) && !defined(SPIM3_RX_DMA_INSTANCE)
    #define SPIM3_RX_DMA_IRQHandler         DMA1_Stream2_IRQHandler
    #define SPIM3_RX_DMA_INSTANCE           DMA1_Stream2
    #define SPIM3_RX_DMA_CHANNEL            DMA_CHANNEL_0
    #define SPIM3_RX_DMA_IRQ                DMA1_Stream2_IRQn
#elif defined(BSP_UART4_RX_USING_DMA) && !defined(UART4_RX_DMA_INSTANCE)
    #define UART4_RX_DMA_IRQHandler         DMA1_Stream2_IRQHandler
    #define UART4_RX_DMA_INSTANCE           DMA1_Stream2
    #define UART4_RX_DMA_CHANNEL            DMA_CHANNEL_4
    #define UART4_RX_DMA_IRQ                DMA1_Stream2_IRQn
#endif

/* DMA1 stream3 */
#if defined(BSP_SPIM2_RX_USING_DMA) && !defined(SPIM2_RX_DMA_INSTANCE)
    #define SPIM2_RX_DMA_IRQHandler         DMA1_Stream3_IRQHandler
    #define SPIM2_RX_DMA_INSTANCE           DMA1_Stream3
    #define SPIM2_RX_DMA_CHANNEL            DMA_CHANNEL_0
    #define SPIM2_RX_DMA_IRQ                DMA1_Stream3_IRQn
#endif

/* DMA1 stream4 */
#if defined(BSP_SPIM2_TX_USING_DMA) && !defined(SPIM2_TX_DMA_INSTANCE)
    #define SPIM2_TX_DMA_IRQHandler         DMA1_Stream4_IRQHandler
    #define SPIM2_TX_DMA_INSTANCE           DMA1_Stream4
    #define SPIM2_TX_DMA_CHANNEL            DMA_CHANNEL_0
    #define SPIM2_TX_DMA_IRQ                DMA1_Stream4_IRQn
#endif

/* DMA1 stream5 */
#if defined(BSP_SPIM3_TX_USING_DMA) && !defined(SPIM3_TX_DMA_INSTANCE)
    #define SPIM3_TX_DMA_IRQHandler         DMA1_Stream5_IRQHandler
    #define SPIM3_TX_DMA_INSTANCE           DMA1_Stream5
    #define SPIM3_TX_DMA_CHANNEL            DMA_CHANNEL_0
    #define SPIM3_TX_DMA_IRQ                DMA1_Stream5_IRQn
#elif defined(BSP_UART2_RX_USING_DMA) && !defined(UART2_RX_DMA_INSTANCE)
    #define UART2_RX_DMA_IRQHandler         DMA1_Stream5_IRQHandler
    #define UART2_RX_DMA_INSTANCE           DMA1_Stream5
    #define UART2_RX_DMA_CHANNEL            DMA_CHANNEL_4
    #define UART2_RX_DMA_IRQ                DMA1_Stream5_IRQn
#endif

/* DMA1 stream6 */

/* DMA1 stream7 */
#if defined(BSP_SPIM3_TX_USING_DMA) && !defined(SPIM3_TX_DMA_INSTANCE)
    #define SPIM3_TX_DMA_IRQHandler         DMA1_Stream7_IRQHandler
    #define SPIM3_TX_DMA_INSTANCE           DMA1_Stream7
    #define SPIM3_TX_DMA_CHANNEL            DMA_CHANNEL_0
    #define SPIM3_TX_DMA_IRQ                DMA1_Stream7_IRQn
#endif

/* DMA2 stream0 */
#if defined(BSP_SPIM1_RX_USING_DMA) && !defined(SPIM1_RX_DMA_INSTANCE)
    #define SPIM1_RX_DMA_IRQHandler         DMA2_Stream0_IRQHandler
    #define SPIM1_RX_DMA_INSTANCE           DMA2_Stream0
    #define SPIM1_RX_DMA_CHANNEL            DMA_CHANNEL_3
    #define SPIM1_RX_DMA_IRQ                DMA2_Stream0_IRQn
#endif

/* DMA2 stream1 */

/* DMA2 stream2 */
#if defined(BSP_SPIM1_RX_USING_DMA) && !defined(SPIM1_RX_DMA_INSTANCE)
    #define SPIM1_RX_DMA_IRQHandler         DMA2_Stream2_IRQHandler
    #define SPIM1_RX_DMA_INSTANCE           DMA2_Stream2
    #define SPIM1_RX_DMA_CHANNEL            DMA_CHANNEL_3
    #define SPIM1_RX_DMA_IRQ                DMA2_Stream2_IRQn
#elif defined(BSP_UART1_RX_USING_DMA) && !defined(UART1_RX_DMA_INSTANCE)
    #define UART1_RX_DMA_IRQHandler         DMA2_Stream2_IRQHandler
    #define UART1_RX_DMA_INSTANCE           DMA2_Stream2
    #define UART1_RX_DMA_CHANNEL            DMA_CHANNEL_4
    #define UART1_RX_DMA_IRQ                DMA2_Stream2_IRQn
#elif defined(BSP_UART6_RX_USING_DMA) && !defined(UART6_RX_DMA_INSTANCE)
    #define UART6_RX_DMA_IRQHandler         DMA2_Stream2_IRQHandler
    #define UART6_RX_DMA_INSTANCE           DMA2_Stream2
    #define UART6_RX_DMA_CHANNEL            DMA_CHANNEL_5
    #define UART6_RX_DMA_IRQ                DMA2_Stream2_IRQn
#endif

/* DMA2 stream3 */
#if defined(BSP_SPIM1_TX_USING_DMA) && !defined(SPIM1_TX_DMA_INSTANCE)
    #define SPIM1_TX_DMA_IRQHandler         DMA2_Stream3_IRQHandler
    #define SPIM1_TX_DMA_INSTANCE           DMA2_Stream3
    #define SPIM1_TX_DMA_CHANNEL            DMA_CHANNEL_3
    #define SPIM1_TX_DMA_IRQ                DMA2_Stream3_IRQn
#endif

/* DMA2 stream4 */

/* DMA2 stream5 */
#if defined(BSP_SPIM1_TX_USING_DMA) && !defined(SPIM1_TX_DMA_INSTANCE)
    #define SPIM1_TX_DMA_IRQHandler         DMA2_Stream5_IRQHandler
    #define SPIM1_TX_DMA_INSTANCE           DMA2_Stream5
    #define SPIM1_TX_DMA_CHANNEL            DMA_CHANNEL_3
    #define SPIM1_TX_DMA_IRQ                DMA2_Stream5_IRQn
#elif defined(BSP_UART1_RX_USING_DMA) && !defined(UART1_RX_DMA_INSTANCE)
    #define UART1_RX_DMA_IRQHandler         DMA2_Stream5_IRQHandler
    #define UART1_RX_DMA_INSTANCE           DMA2_Stream5
    #define UART1_RX_DMA_CHANNEL            DMA_CHANNEL_4
    #define UART1_RX_DMA_IRQ                DMA2_Stream5_IRQn
#endif


/* DMA2 stream7 */
#if defined(BSP_UART1_TX_USING_DMA) && !defined(UART1_TX_DMA_INSTANCE)
    #define UART1_TX_DMA_IRQHandler         DMA2_Stream7_IRQHandler
    #define UART1_TX_DMA_INSTANCE           DMA2_Stream7
    #define UART1_TX_DMA_CHANNEL            DMA_CHANNEL_4
    #define UART1_TX_DMA_IRQ                DMA2_Stream7_IRQn
#endif



#ifdef __cplusplus
}
#endif
#endif /*defined(SOC_SERIES_STM32F2)*/
#endif /*_HTD_DRV_CONFIG_STM32F2XX_H_*/

