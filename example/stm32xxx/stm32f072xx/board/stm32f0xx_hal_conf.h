/**
  ******************************************************************************
  * @file    stm32f0xx_hal_conf.h
  * @author  MCD Application Team
  * @brief   HAL configuration file.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2016 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STM32F0xx_HAL_CONF_H
#define __STM32F0xx_HAL_CONF_H

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "htd_config.h"

#ifdef __cplusplus
 extern "C" {
#endif


/////////////////////////////////模块选择/////////////////////////////////////
#define HAL_MODULE_ENABLED
//#define HAL_ADC_MODULE_ENABLED
/* #define HAL_CAN_LEGACY_MODULE_ENABLED */
//#define HAL_CAN_MODULE_ENABLED
//#define HAL_CEC_MODULE_ENABLED
//#define HAL_COMP_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED
#define HAL_CRC_MODULE_ENABLED
#define HAL_DAC_MODULE_ENABLED
#define HAL_DMA_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_EXTI_MODULE_ENABLED
#define HAL_I2C_MODULE_ENABLED
//#define HAL_I2S_MODULE_ENABLED
//#define HAL_IRDA_MODULE_ENABLED
#define HAL_IWDG_MODULE_ENABLED
//#define HAL_PCD_MODULE_ENABLED
#define HAL_PWR_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
#define HAL_RTC_MODULE_ENABLED
//#define HAL_SMARTCARD_MODULE_ENABLED
//#define HAL_SMBUS_MODULE_ENABLED
#define HAL_SPI_MODULE_ENABLED
//#define HAL_TIM_MODULE_ENABLED
//#define HAL_TSC_MODULE_ENABLED
#define HAL_UART_MODULE_ENABLED
#define HAL_USART_MODULE_ENABLED
#define HAL_WWDG_MODULE_ENABLED



/////////////////////////////////时钟配置/////////////////////////////////////
// 外部高速时钟HSE，8M
#if !defined  (HSE_VALUE) 
    #define HSE_VALUE               8000000U  /*!< Value of the External oscillator in Hz */
#endif /* HSE_VALUE */

/**
  * @brief In the following line adjust the External High Speed oscillator (HSE) Startup 
  *        Timeout value 
  */
#if !defined  (HSE_STARTUP_TIMEOUT)
    #define HSE_STARTUP_TIMEOUT     100U      /*!< Time out for HSE start up, in ms */
#endif /* HSE_STARTUP_TIMEOUT */

// 内部高速时钟HSI
#if !defined  (HSI_VALUE)
    #define HSI_VALUE               8000000U /*!< Value of the Internal oscillator in Hz */
#endif /* HSI_VALUE */

/**
  * @brief In the following line adjust the Internal High Speed oscillator (HSI) Startup 
  *        Timeout value 
  */
#if !defined  (HSI_STARTUP_TIMEOUT) 
    #define HSI_STARTUP_TIMEOUT     5000U     /*!< Time out for HSI start up */
#endif /* HSI_STARTUP_TIMEOUT */  

/**
  * @brief Internal High Speed oscillator for ADC (HSI14) value.
  */
#if !defined  (HSI14_VALUE) 
    #define HSI14_VALUE             14000000U /*!< Value of the Internal High Speed oscillator for ADC in Hz.
                                             The real value may vary depending on the variations
                                             in voltage and temperature.  */
#endif /* HSI14_VALUE */

/**
  * @brief Internal High Speed oscillator for USB (HSI48) value.
  */
#if !defined  (HSI48_VALUE) 
    #define HSI48_VALUE             48000000U /*!< Value of the Internal High Speed oscillator for USB in Hz.
                                             The real value may vary depending on the variations
                                             in voltage and temperature.  */
#endif /* HSI48_VALUE */

// 内部低速时钟 LSI
#if !defined  (LSI_VALUE) 
    #define LSI_VALUE               32000U
#endif /* LSI_VALUE */                     /*!< Value of the Internal Low Speed oscillator in Hz
                                                The real value may vary depending on the variations
                                                in voltage and temperature. */

// 外部低速时钟
#if !defined  (LSE_VALUE)
    #define LSE_VALUE               32768U    /*!< Value of the External Low Speed oscillator in Hz */
#endif /* LSE_VALUE */

#if !defined  (LSE_STARTUP_TIMEOUT)
    #define LSE_STARTUP_TIMEOUT     5000U     /*!< Time out for LSE start up, in ms */
#endif /* LSE_STARTUP_TIMEOUT */


/* Tip: To avoid modifying this file each time you need to use different HSE,
   ===  you can define the HSE value in your toolchain compiler preprocessor. */

/* ########################### System Configuration ######################### */
/**
  * @brief This is the HAL system configuration section
  */     
#define VDD_VALUE                   3300U  /*!< Value of VDD in mv */           
#define TICK_INT_PRIORITY            ((uint32_t)(1U<<__NVIC_PRIO_BITS) - 1U) /*!< tick interrupt priority (lowest by default)             */
                                                                              /*  Warning: Must be set to higher priority for HAL_Delay()  */
                                                                              /*  and HAL_GetTick() usage under interrupt context          */
#define USE_RTOS                    0U
#define PREFETCH_ENABLE             1U
#define INSTRUCTION_CACHE_ENABLE    0U
#define DATA_CACHE_ENABLE           0U
#define USE_SPI_CRC                 0U

#define USE_HAL_ADC_REGISTER_CALLBACKS         0U /* ADC register callback disabled       */
#define USE_HAL_CAN_REGISTER_CALLBACKS         0U /* CAN register callback disabled       */
#define USE_HAL_COMP_REGISTER_CALLBACKS        0U /* COMP register callback disabled      */
#define USE_HAL_CEC_REGISTER_CALLBACKS         0U /* CEC register callback disabled       */
#define USE_HAL_DAC_REGISTER_CALLBACKS         0U /* DAC register callback disabled       */
#define USE_HAL_I2C_REGISTER_CALLBACKS         0U /* I2C register callback disabled       */
#define USE_HAL_SMBUS_REGISTER_CALLBACKS       0U /* SMBUS register callback disabled     */
#define USE_HAL_UART_REGISTER_CALLBACKS        0U /* UART register callback disabled      */
#define USE_HAL_USART_REGISTER_CALLBACKS       0U /* USART register callback disabled     */
#define USE_HAL_IRDA_REGISTER_CALLBACKS        0U /* IRDA register callback disabled      */
#define USE_HAL_SMARTCARD_REGISTER_CALLBACKS   0U /* SMARTCARD register callback disabled */
#define USE_HAL_WWDG_REGISTER_CALLBACKS        0U /* WWDG register callback disabled      */
#define USE_HAL_RTC_REGISTER_CALLBACKS         0U /* RTC register callback disabled       */
#define USE_HAL_SPI_REGISTER_CALLBACKS         0U /* SPI register callback disabled       */
#define USE_HAL_I2S_REGISTER_CALLBACKS         0U /* I2S register callback disabled       */
#define USE_HAL_TIM_REGISTER_CALLBACKS         0U /* TIM register callback disabled       */
#define USE_HAL_TSC_REGISTER_CALLBACKS         0U /* TSC register callback disabled       */
#define USE_HAL_PCD_REGISTER_CALLBACKS         0U /* PCD register callback disabled       */

/* ########################## Assert Selection ############################## */
/**
  * @brief Uncomment the line below to expanse the "assert_param" macro in the 
  *        HAL drivers code
  */
#define USE_FULL_ASSERT    1U 
#if (defined(USE_FULL_ASSERT) && (USE_FULL_ASSERT == 1))
    extern void htd_assert_failed(const char* file, uint32_t line);
    #define assert_param(expr) ((expr) ? (void)0U : htd_assert_failed(__FILE__, __LINE__))
#else
    #define assert_param(expr)
#endif /* USE_FULL_ASSERT */

/* Includes ------------------------------------------------------------------*/
/**
  * @brief Include module's header file 
  */
#include "stm32f0xx_ll_utils.h"
#ifdef HAL_RCC_MODULE_ENABLED
    #include "stm32f0xx_hal_rcc.h"
    #include "stm32f0xx_ll_rcc.h"
#endif /* HAL_RCC_MODULE_ENABLED */

#ifdef HAL_GPIO_MODULE_ENABLED
    #include "stm32f0xx_hal_gpio.h"
    #include "stm32f0xx_ll_gpio.h"
#endif /* HAL_GPIO_MODULE_ENABLED */

#ifdef HAL_EXTI_MODULE_ENABLED
    #include "stm32f0xx_hal_exti.h"
    #include "stm32f0xx_ll_exti.h"
#endif /* HAL_EXTI_MODULE_ENABLED */

#ifdef HAL_DMA_MODULE_ENABLED
    #include "stm32f0xx_hal_dma.h"
#endif /* HAL_DMA_MODULE_ENABLED */

#ifdef HAL_CORTEX_MODULE_ENABLED
    #include "stm32f0xx_hal_cortex.h"
#endif /* HAL_CORTEX_MODULE_ENABLED */

#ifdef HAL_ADC_MODULE_ENABLED
    #include "stm32f0xx_hal_adc.h"
#endif /* HAL_ADC_MODULE_ENABLED */

#ifdef HAL_CAN_MODULE_ENABLED
    #include "stm32f0xx_hal_can.h"
#endif /* HAL_CAN_MODULE_ENABLED */

#ifdef HAL_CAN_LEGACY_MODULE_ENABLED
    #include "stm32f0xx_hal_can_legacy.h"
#endif /* HAL_CAN_LEGACY_MODULE_ENABLED */

#ifdef HAL_CEC_MODULE_ENABLED
    #include "stm32f0xx_hal_cec.h"
#endif /* HAL_CEC_MODULE_ENABLED */

#ifdef HAL_COMP_MODULE_ENABLED
    #include "stm32f0xx_hal_comp.h"
#endif /* HAL_COMP_MODULE_ENABLED */

#ifdef HAL_CRC_MODULE_ENABLED
    #include "stm32f0xx_hal_crc.h"
    #include "stm32f0xx_ll_crc.h"
#endif /* HAL_CRC_MODULE_ENABLED */

#ifdef HAL_DAC_MODULE_ENABLED
    #include "stm32f0xx_hal_dac.h"
#endif /* HAL_DAC_MODULE_ENABLED */

#ifdef HAL_FLASH_MODULE_ENABLED
    #include "stm32f0xx_hal_flash.h"
#endif /* HAL_FLASH_MODULE_ENABLED */

#ifdef HAL_I2C_MODULE_ENABLED
    #include "stm32f0xx_hal_i2c.h"
#endif /* HAL_I2C_MODULE_ENABLED */

#ifdef HAL_I2S_MODULE_ENABLED
    #include "stm32f0xx_hal_i2s.h"
#endif /* HAL_I2S_MODULE_ENABLED */

#ifdef HAL_IRDA_MODULE_ENABLED
    #include "stm32f0xx_hal_irda.h"
#endif /* HAL_IRDA_MODULE_ENABLED */

#ifdef HAL_IWDG_MODULE_ENABLED
    #include "stm32f0xx_hal_iwdg.h"
#endif /* HAL_IWDG_MODULE_ENABLED */

#ifdef HAL_PCD_MODULE_ENABLED
    #include "stm32f0xx_hal_pcd.h"
#endif /* HAL_PCD_MODULE_ENABLED */

#ifdef HAL_PWR_MODULE_ENABLED
    #include "stm32f0xx_hal_pwr.h"
#endif /* HAL_PWR_MODULE_ENABLED */

#ifdef HAL_RTC_MODULE_ENABLED
    #include "stm32f0xx_hal_rtc.h"
    #include "stm32f0xx_ll_rtc.h"
#endif /* HAL_RTC_MODULE_ENABLED */

#ifdef HAL_SMARTCARD_MODULE_ENABLED
    #include "stm32f0xx_hal_smartcard.h"
#endif /* HAL_SMARTCARD_MODULE_ENABLED */

#ifdef HAL_SMBUS_MODULE_ENABLED
    #include "stm32f0xx_hal_smbus.h"
#endif /* HAL_SMBUS_MODULE_ENABLED */

#ifdef HAL_SPI_MODULE_ENABLED
    #include "stm32f0xx_hal_spi.h"
#endif /* HAL_SPI_MODULE_ENABLED */

#ifdef HAL_TIM_MODULE_ENABLED
    #include "stm32f0xx_hal_tim.h"
#endif /* HAL_TIM_MODULE_ENABLED */

#ifdef HAL_TSC_MODULE_ENABLED
    #include "stm32f0xx_hal_tsc.h"
#endif /* HAL_TSC_MODULE_ENABLED */

#ifdef HAL_UART_MODULE_ENABLED
    #include "stm32f0xx_hal_uart.h"
    #include "stm32f0xx_ll_usart.h"
#endif /* HAL_UART_MODULE_ENABLED */

#ifdef HAL_USART_MODULE_ENABLED
    #include "stm32f0xx_hal_usart.h"
#endif /* HAL_USART_MODULE_ENABLED */

#ifdef HAL_WWDG_MODULE_ENABLED
    #include "stm32f0xx_hal_wwdg.h"
#endif /* HAL_WWDG_MODULE_ENABLED */


#ifndef SRAM_SIZE
    #define SRAM_SIZE           0x4000
#endif 

#ifdef __cplusplus
}
#endif
#endif /* __STM32F0xx_HAL_CONF_H */

