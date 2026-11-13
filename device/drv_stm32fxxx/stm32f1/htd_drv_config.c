// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL   4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG   "drv.config"
#include "htd_device.h"
#include "htd_drv_common.h"

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


// @brief GPIO5~GPIO9 中断
void EXTI9_5_IRQHandler(void)
{
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
}

// @brief GPIO5~GPIO9 中断
void EXTI15_10_IRQHandler(void)
{
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

