// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_DRV_CLOCK_H_
#define _HTD_DRV_CLOCK_H_

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif


// @brief 使能dma时钟
void drv_clock_dma_enable(void* p_dma_instance, bool is_enable);
// @brief 使能串口时钟
void drv_clock_uart_enable(void* p_instance, bool is_enable);
// @brief 使能 SPI 时钟
void drv_clock_spi_enable(void* p_instance, bool is_enable);
// @brief 使能 i2c 时钟
void drv_clock_i2c_enable(void* p_instance, bool is_enable);


#ifdef __cplusplus
}
#endif
#endif /*_HTD_DRV_CLOCK_H_*/

