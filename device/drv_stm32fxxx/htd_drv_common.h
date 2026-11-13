#ifndef _HTD_DRV_COMMON_H_
#define _HTD_DRV_COMMON_H_

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "stm32_assert.h"
#include "htd_drv_config.h"

#ifdef __cplusplus
extern "C" {
#endif


//////////////////////////////////////////////////////////////////////
#if !defined(SRAM_BASE)
    #if defined(SRAM1_BASE)
        #define SRAM_BASE                   SRAM1_BASE
    #endif
#endif /*!defined(SRAM_BASE)*/



//////////////////////////////////////////////////////////////////////
#ifdef USART1
    #define UART1                           USART1
    #define UART1_IRQn                      USART1_IRQn
    #define UART1_IRQHandler                USART1_IRQHandler
    #define __HAL_RCC_UART1_CLK_ENABLE      __HAL_RCC_USART1_CLK_ENABLE
    #define __HAL_RCC_UART1_CLK_DISABLE     __HAL_RCC_USART1_CLK_DISABLE
#endif
#ifdef USART2
    #define UART2                           USART2
    #define UART2_IRQn                      USART2_IRQn
    #define UART2_IRQHandler                USART2_IRQHandler
    #define __HAL_RCC_UART2_CLK_ENABLE      __HAL_RCC_USART2_CLK_ENABLE
    #define __HAL_RCC_UART2_CLK_DISABLE     __HAL_RCC_USART2_CLK_DISABLE
#endif
#ifdef USART3
    #define UART3                           USART3
    #define UART3_IRQn                      USART3_IRQn
    #define UART3_IRQHandler                USART3_IRQHandler
    #define __HAL_RCC_UART3_CLK_ENABLE      __HAL_RCC_USART3_CLK_ENABLE
    #define __HAL_RCC_UART3_CLK_DISABLE     __HAL_RCC_USART3_CLK_DISABLE
#endif
#ifdef USART4
    #define UART4                           USART4
    #define UART4_IRQn                      USART4_IRQn
    #define UART4_IRQHandler                USART4_IRQHandler
    #define __HAL_RCC_UART4_CLK_ENABLE      __HAL_RCC_USART4_CLK_ENABLE
    #define __HAL_RCC_UART4_CLK_DISABLE     __HAL_RCC_USART4_CLK_DISABLE
#endif
#ifdef USART5
    #define UART5                           USART5
    #define UART5_IRQn                      USART5_IRQn
    #define UART5_IRQHandler                USART5_IRQHandler
    #define __HAL_RCC_UART5_CLK_ENABLE      __HAL_RCC_USART5_CLK_ENABLE
    #define __HAL_RCC_UART5_CLK_DISABLE     __HAL_RCC_USART5_CLK_DISABLE
#endif
#ifdef USART6
    #define UART6                           USART6
    #define UART6_IRQn                      USART6_IRQn
    #define UART6_IRQHandler                USART6_IRQHandler
    #define __HAL_RCC_UART6_CLK_ENABLE      __HAL_RCC_USART6_CLK_ENABLE
    #define __HAL_RCC_UART6_CLK_DISABLE     __HAL_RCC_USART6_CLK_DISABLE
#endif
#ifdef USART7
    #define UART7                           USART7
    #define UART7_IRQn                      USART7_IRQn
    #define UART7_IRQHandler                USART7_IRQHandler
    #define __HAL_RCC_UART7_CLK_ENABLE      __HAL_RCC_USART7_CLK_ENABLE
    #define __HAL_RCC_UART7_CLK_DISABLE     __HAL_RCC_USART7_CLK_DISABLE
#endif
#ifdef USART8
    #define UART8                           USART8
    #define UART8_IRQn                      USART8_IRQn
    #define UART8_IRQHandler                USART8_IRQHandler
    #define __HAL_RCC_UART8_CLK_ENABLE      __HAL_RCC_USART8_CLK_ENABLE
    #define __HAL_RCC_UART8_CLK_DISABLE     __HAL_RCC_USART8_CLK_DISABLE
#endif




// @brief 申请内存
void* htd_drv_malloc(uint32_t size);
// @brief 释放内存
void htd_drv_free(void* p_ptr);


#ifdef __cplusplus
extern "C" {
#endif
#endif /*_HTD_DRV_COMMON_H_*/

