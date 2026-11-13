// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_DRV_DMA_H_
#define _HTD_DRV_DMA_H_

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "htd_drv_config.h"

#ifdef __cplusplus
extern "C" {
#endif


// DMA 中断函数
typedef void (*dma_irq_handle_t)(void);


// @brief 获取未使用的 DMA 通道信息 
int32_t drv_dma_ch_register(bool is_bdma, stm32_dma_ch_t* p_info, void* handle);
// @brief 清空通道号
void drv_dma_ch_unregister(void* handle);


#ifdef __cplusplus
}
#endif
#endif /*_HTD_DRV_DMA_H_ */

