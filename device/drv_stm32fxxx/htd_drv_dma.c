// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL   4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG   "drv.dma"
#include "htd_device.h"
#include "htd_drv_common.h"
#include "htd_drv_dma.h"



#if defined(DMAMUX1) || defined(DMAMUX2)
//////////////////////////////////////////////////////////////////////
static dma_irq_handle_t bdma_irq_handle_table[BDMA_CH_MAX] = {NULL};

// @brief 注册 BDMA
static int32_t bdma_ch_register(stm32_dma_ch_t* p_info, dma_irq_handle_t handle)
{
    ASSERT(p_info);
    ASSERT(handle);

    uint32_t idx;
    for (idx=0; idx<ARRAY_COUNT(bdma_irq_handle_table); ++idx) {
        if (bdma_irq_handle_table[idx] == NULL) {
            bdma_irq_handle_table[idx] = handle;
            break;
        }
    }
    if (idx >= ARRAY_COUNT(bdma_irq_handle_table)) {
        ASSERT(0);
        return -(ERROR_CODE_INVALID_PARAM);
    }
    //p_info->idx = idx;
    //p_info->ch = idx & 0x07;

#if defined(BDMA_Channel0)
    if (idx == 0) {
        p_info->irqn = BDMA_Channel0_IRQn;  
        p_info->instance = BDMA_Channel0; 
        return 0;
    }
#endif /*defined(BDMA_Channel0)*/
#if defined(BDMA_Channel1)
    if (idx == 1) {
        p_info->irqn = BDMA_Channel1_IRQn;  
        p_info->instance = BDMA_Channel1; 
        return 0;
    }
#endif /*defined(BDMA_Channel1)*/
#if defined(BDMA_Channel2)
    if (idx == 2) {
        p_info->irqn = BDMA_Channel2_IRQn;  
        p_info->instance = BDMA_Channel2; 
        return 0;
    }
#endif /*defined(BDMA_Channel2)*/
#if defined(BDMA_Channel3)
    if (idx == 3) {
        p_info->irqn = BDMA_Channel3_IRQn;  
        p_info->instance = BDMA_Channel3; 
        return 0;
    }
#endif /*defined(BDMA_Channel3)*/
#if defined(BDMA_Channel4)
    if (idx == 4) {
        p_info->irqn = BDMA_Channel4_IRQn;  
        p_info->instance = BDMA_Channel4; 
        return 0;
    }
#endif /*defined(BDMA_Channel4)*/
#if defined(BDMA_Channel5)
    if (idx == 5) {
        p_info->irqn = BDMA_Channel5_IRQn;  
        p_info->instance = BDMA_Channel5; 
        return 0;
    }
#endif /*defined(BDMA_Channel5)*/
#if defined(BDMA_Channel6)
    if (idx == 6) {
        p_info->irqn = BDMA_Channel6_IRQn;  
        p_info->instance = BDMA_Channel6; 
        return 0;
    }
#endif /*defined(BDMA_Channel6)*/
#if defined(BDMA_Channel7)
    if (idx == 7) {
        p_info->irqn = BDMA_Channel7_IRQn;  
        p_info->instance = BDMA_Channel7; 
        return 0;
    }
#endif /*defined(BDMA_Channel7)*/
#if defined(BDMA1_Channel0)
    if (idx == 8) {
        p_info->irqn = BDMA1_IRQn;
        p_info->instance = BDMA1_Channel0; 
        return 0;
    }
#endif /*defined(BDMA1_Channel0)*/
#if defined(BDMA1_Channel1)
    if (idx == 9) {
        p_info->irqn = BDMA1_IRQn;  
        p_info->instance = BDMA1_Channel1; 
        return 0;
    }
#endif /*defined(BDMA1_Channel1)*/
#if defined(BDMA1_Channel2)
    if (idx == 10) {
        p_info->irqn = BDMA1_IRQn;  
        p_info->instance = BDMA1_Channel2; 
        return 0;
    }
#endif /*defined(BDMA1_Channel2)*/
#if defined(BDMA1_Channel3)
    if (idx == 11) {
        p_info->irqn = BDMA1_IRQn;  
        p_info->instance = BDMA1_Channel3; 
        return 0;
    }
#endif /*defined(BDMA1_Channel3)*/
#if defined(BDMA1_Channel4)
    if (idx == 12) {
        p_info->irqn = BDMA1_IRQn;  
        p_info->instance = BDMA1_Channel4; 
        return 0;
    }
#endif /*defined(BDMA1_Channel4)*/
#if defined(BDMA1_Channel5)
    if (idx == 13) {
        p_info->irqn = BDMA1_IRQn;  
        p_info->instance = BDMA1_Channel5; 
        return 0;
    }
#endif /*defined(BDMA1_Channel5)*/
#if defined(BDMA1_Channel6)
    if (idx == 14) {
        p_info->irqn = BDMA1_IRQn;  
        p_info->instance = BDMA1_Channel6; 
        return 0;
    }
#endif /*defined(BDMA1_Channel6)*/
#if defined(BDMA1_Channel7)
    if (idx == 15) {
        p_info->irqn = BDMA1_IRQn;  
        p_info->instance = BDMA1_Channel7; 
        return 0;
    }
#endif /*defined(BDMA1_Channel7)*/

    ASSERT(0);
    return -(ERROR_CODE_INVALID_PARAM);
}

// @brief 注销通道号
static void bdma_ch_unregister(dma_irq_handle_t handle)
{
    for (uint32_t idx=0; idx < ARRAY_COUNT(bdma_irq_handle_table); ++idx) {
        if (bdma_irq_handle_table[idx] == handle) {
            bdma_irq_handle_table[idx] = NULL;
            return;
        }
    }
}

// @brief 中断回调
static void bdma_stream_irq_handle(uint8_t idx)
{
    if (bdma_irq_handle_table[idx]) {
        bdma_irq_handle_table[idx]();
    }
}

#if defined(BDMA_Channel0)
void BDMA_Channel0_IRQHandler(void) {bdma_stream_irq_handle(0);}
#endif /*defined(BDMA_Channel0)*/
#if defined(BDMA_Channel1)
void BDMA_Channel1_IRQHandler(void) {bdma_stream_irq_handle(1);}
#endif /*defined(BDMA_Channel1)*/
#if defined(BDMA_Channel2)
void BDMA_Channel2_IRQHandler(void) {bdma_stream_irq_handle(2);}
#endif /*defined(BDMA_Channel2)*/
#if defined(BDMA_Channel3)
void BDMA_Channel3_IRQHandler(void) {bdma_stream_irq_handle(3);}
#endif /*defined(BDMA_Channel3)*/
#if defined(BDMA_Channel4)
void BDMA_Channel4_IRQHandler(void) {bdma_stream_irq_handle(4);}
#endif /*defined(BDMA_Channel4)*/
#if defined(BDMA_Channel5)
void BDMA_Channel5_IRQHandler(void) {bdma_stream_irq_handle(5);}
#endif /*defined(BDMA_Channel5)*/
#if defined(BDMA_Channel6)
void BDMA_Channel6_IRQHandler(void) {bdma_stream_irq_handle(6);}
#endif /*defined(BDMA_Channel6)*/
#if defined(BDMA_Channel7)
void BDMA_Channel7_IRQHandler(void) {bdma_stream_irq_handle(7);}
#endif /*defined(BDMA_Channel7)*/
#if defined(BDMA1)
void BDMA1_IRQHandler(void)
{
    if (READ_BIT(BDMA1_Channel0->ISR, BDMA_ISR_GIF0)) {
        bdma_stream_irq_handle(8);
    }
    if (READ_BIT(BDMA1_Channel1->ISR, BDMA_ISR_GIF1)) {
        bdma_stream_irq_handle(9);
    }
    if (READ_BIT(BDMA2_Channel2->ISR, BDMA_ISR_GIF2)) {
        bdma_stream_irq_handle(10);
    }
    if (READ_BIT(BDMA2_Channel3->ISR, BDMA_ISR_GIF3)) {
        bdma_stream_irq_handle(11);
    }
    if (READ_BIT(BDMA2_Channel4->ISR, BDMA_ISR_GIF4)) {
        bdma_stream_irq_handle(12);
    }
    if (READ_BIT(BDMA2_Channel5->ISR, BDMA_ISR_GIF5)) {
        bdma_stream_irq_handle(13);
    }
    if (READ_BIT(BDMA2_Channel6->ISR, BDMA_ISR_GIF6)) {
        bdma_stream_irq_handle(14);
    }
    if (READ_BIT(BDMA2_Channel7->ISR, BDMA_ISR_GIF7)) {
        bdma_stream_irq_handle(15);
    }
}
#endif /*defined(BDMA1_Channel0)*/


static dma_irq_handle_t dma_irq_handle_table[DMA_CH_MAX] = {NULL};

// @brief 获取未使用的 DMA 通道信息 
static int32_t dma_ch_register(stm32_dma_ch_t* p_info, dma_irq_handle_t handle)
{
    ASSERT(p_info);
    ASSERT(handle);

    uint32_t idx;
    for (idx=0; idx<ARRAY_COUNT(dma_irq_handle_table); ++idx) {
        if (dma_irq_handle_table[idx] == NULL) {
            dma_irq_handle_table[idx] = handle;
            break;
        }
    }
    if (idx >= ARRAY_COUNT(dma_irq_handle_table)) {
        ASSERT(0);
        return -(ERROR_CODE_INVALID_PARAM);
    }
    //p_info->idx = idx;
    //p_info->ch = idx & 0x07;

#if defined(DMA1_Stream0)
    if (idx == 0) {
        p_info->irqn = DMA1_Stream0_IRQn;
        p_info->instance = DMA1_Stream0;
        return 0;
    }
#endif /*defined(DMA1_Stream0)*/
#if defined(DMA1_Stream1)
    if (idx == 1) {
        p_info->irqn = DMA1_Stream1_IRQn;
        p_info->instance = DMA1_Stream1;
        return 0;
    }
#endif /*defined(DMA1_Stream1)*/
#if defined(DMA1_Stream2)
    if (idx == 2) {
        p_info->irqn = DMA1_Stream2_IRQn;
        p_info->instance = DMA1_Stream2;
        return 0;
    }
#endif /*defined(DMA1_Stream2)*/
#if defined(DMA1_Stream3)
    if (idx == 3) {
        p_info->irqn = DMA1_Stream3_IRQn;
        p_info->instance = DMA1_Stream3;
        return 0;
    }
#endif /*defined(DMA1_Stream3)*/
#if defined(DMA1_Stream4)
    if (idx == 4) {
        p_info->irqn = DMA1_Stream4_IRQn;
        p_info->instance = DMA1_Stream4;
        return 0;
    }
#endif /*defined(DMA1_Stream4)*/
#if defined(DMA1_Stream5)
    if (idx == 5) {
        p_info->irqn = DMA1_Stream5_IRQn;
        p_info->instance = DMA1_Stream5;
        return 0;
    }
#endif /*defined(DMA1_Stream5)*/
#if defined(DMA1_Stream6)
    if (idx == 6) {
        p_info->irqn = DMA1_Stream6_IRQn;
        p_info->instance = DMA1_Stream6;
        return 0;
    }
#endif /*defined(DMA1_Stream6)*/
#if defined(DMA1_Stream7)
    if (idx == 7) {
        p_info->irqn = DMA1_Stream7_IRQn;
        p_info->instance = DMA1_Stream7;
        return 0;
    }
#endif /*defined(DMA1_Stream7)*/
#if defined(DMA2_Stream0)
    if (idx == 8) {
        p_info->irqn = DMA2_Stream0_IRQn;
        p_info->instance = DMA2_Stream0;
        return 0;
    }
#endif /*defined(DMA2_Stream0)*/
#if defined(DMA2_Stream1)
    if (idx == 9) {
        p_info->irqn = DMA2_Stream1_IRQn;
        p_info->instance = DMA2_Stream1;
        return 0;
    }
#endif /*defined(DMA2_Stream1)*/
#if defined(DMA2_Stream2)
    if (idx == 10) {
        p_info->irqn = DMA2_Stream2_IRQn;
        p_info->instance = DMA2_Stream2;
        return 0;
    }
#endif /*defined(DMA2_Stream2)*/
#if defined(DMA2_Stream3)
    if (idx == 11) {
        p_info->irqn = DMA2_Stream3_IRQn;
        p_info->instance = DMA2_Stream3;
        return 0;
    }
#endif /*defined(DMA2_Stream3)*/
#if defined(DMA2_Stream4)
    if (idx == 12) {
        p_info->irqn = DMA2_Stream4_IRQn;
        p_info->instance = DMA2_Stream4;
        return 0;
    }
#endif /*defined(DMA2_Stream4)*/
#if defined(DMA2_Stream5)
    if (idx == 13) {
        p_info->irqn = DMA2_Stream5_IRQn;
        p_info->instance = DMA2_Stream5;
        return 0;
    }
#endif /*defined(DMA2_Stream5)*/
#if defined(DMA2_Stream6)
    if (idx == 14) {
        p_info->irqn = DMA2_Stream6_IRQn;
        p_info->instance = DMA2_Stream6;
        return 0;
    }
#endif /*defined(DMA2_Stream6)*/
#if defined(DMA2_Stream7)
    if (idx == 15) {
        p_info->irqn = DMA2_Stream7_IRQn;
        p_info->instance = DMA2_Stream7;
        return 0;
    }
#endif /*defined(DMA2_Stream7)*/
    ASSERT(0);
    return -(ERROR_CODE_INVALID_PARAM);
}

// @brief 注销通道号
static void dma_ch_unregister(dma_irq_handle_t handle)
{
    for (uint32_t idx=0; idx < ARRAY_COUNT(dma_irq_handle_table); ++idx) {
        if (dma_irq_handle_table[idx] == handle) {
            dma_irq_handle_table[idx] = NULL;
            return;
        }
    }
}

// @brief DMA 中断句柄
static void dma_stream_irq_handle(uint32_t idx)
{
    if (idx < ARRAY_COUNT(dma_irq_handle_table) && dma_irq_handle_table[idx]) {
        dma_irq_handle_table[idx]();
    }
}

#if defined(DMA1_Stream0)
// @brief DMA1 stream0 中断
void DMA1_Stream0_IRQHandler(void){dma_stream_irq_handle(0);}
#endif /*defined(DMA1_Stream0)*/
#if defined(DMA1_Stream1)
// @brief DMA1 stream1 中断
void DMA1_Stream1_IRQHandler(void){dma_stream_irq_handle(1);}
#endif /*defined(DMA1_Stream1)*/
#if defined(DMA1_Stream2)
// @brief DMA1 stream2 中断
void DMA1_Stream2_IRQHandler(void){dma_stream_irq_handle(2);}
#endif /*defined(DMA1_Stream2)*/
#if defined(DMA1_Stream3)
// @brief DMA1 stream3 中断
void DMA1_Stream3_IRQHandler(void){dma_stream_irq_handle(3);}
#endif /*defined(DMA1_Stream3)*/
#if defined(DMA1_Stream4)
// @brief DMA1 stream4 中断
void DMA1_Stream4_IRQHandler(void){dma_stream_irq_handle(4);}
#endif /*defined(DMA1_Stream4)*/
#if defined(DMA1_Stream5)
// @brief DMA1 stream5 中断
void DMA1_Stream5_IRQHandler(void){dma_stream_irq_handle(5);}
#endif /*defined(DMA1_Stream5)*/
#if defined(DMA1_Stream6)
// @brief DMA1 stream6 中断
void DMA1_Stream6_IRQHandler(void){dma_stream_irq_handle(6);}
#endif /*defined(DMA1_Stream6)*/
#if defined(DMA1_Stream7)
// @brief DMA1 stream7 中断
void DMA1_Stream7_IRQHandler(void){dma_stream_irq_handle(7);}
#endif /*defined(DMA1_Stream7)*/
#if defined(DMA2_Stream0)
// @brief DMA2 stream0 中断
void DMA2_Stream0_IRQHandler(void){dma_stream_irq_handle(8);}
#endif /*defined(DMA2_Stream0)*/
#if defined(DMA2_Stream1)
// @brief DMA2 stream1 中断
void DMA2_Stream1_IRQHandler(void){dma_stream_irq_handle(9);}
#endif /*defined(DMA2_Stream1)*/
#if defined(DMA2_Stream2)
// @brief DMA2 stream2 中断
void DMA2_Stream2_IRQHandler(void){dma_stream_irq_handle(10);}
#endif /*defined(DMA2_Stream2)*/
#if defined(DMA2_Stream3)
// @brief DMA2 stream3 中断
void DMA2_Stream3_IRQHandler(void){dma_stream_irq_handle(11);}
#endif /*defined(DMA2_Stream3)*/
#if defined(DMA2_Stream4)
// @brief DMA2 stream4 中断
void DMA2_Stream4_IRQHandler(void){dma_stream_irq_handle(12);}
#endif /*defined(DMA2_Stream4)*/
#if defined(DMA2_Stream5)
// @brief DMA2 stream5 中断
void DMA2_Stream5_IRQHandler(void){dma_stream_irq_handle(13);}
#endif /*defined(DMA2_Stream5)*/
#if defined(DMA2_Stream6)
// @brief DMA2 stream6 中断
void DMA2_Stream6_IRQHandler(void){dma_stream_irq_handle(14);}
#endif /*defined(DMA2_Stream6)*/
#if defined(DMA2_Stream7)
// @brief DMA2 stream7 中断
void DMA2_Stream7_IRQHandler(void){dma_stream_irq_handle(15);}
#endif /*defined(DMA2_Stream7)*/


// @brief 获取未使用的 DMA 通道信息 
int32_t drv_dma_ch_register(bool is_bdma, stm32_dma_ch_t* p_info, void* handle)
{
    if (is_bdma) {
        return bdma_ch_register(p_info, (dma_irq_handle_t)handle);
    }
    return dma_ch_register(p_info, (dma_irq_handle_t)handle);
}
// @brief 清空通道号
void drv_dma_ch_unregister(void* handle)
{
    bdma_ch_unregister((dma_irq_handle_t)handle);
    dma_ch_unregister((dma_irq_handle_t)handle);
}

#else
//////////////////////////////////////////////////////////////////////
// @brief 获取未使用的 DMA 通道信息 
int32_t drv_dma_ch_register(bool is_bdma, stm32_dma_ch_t* p_info, void* handle)
{
    DMA_Stream_TypeDef_t* p_instance = (DMA_Stream_TypeDef_t*)handle;
    p_info->instance = p_instance;

#ifdef DMA1_Stream0
    if (p_instance == DMA1_Stream0) {
        p_info->irqn = DMA1_Stream0_IRQn;
        return 0;
    }
#endif
#ifdef DMA1_Stream1
    if (p_instance == DMA1_Stream1) {
        p_info->irqn = DMA1_Stream1_IRQn;
        return 0;
    }
#endif
#ifdef DMA1_Stream2
    if (p_instance == DMA1_Stream2) {
        p_info->irqn = DMA1_Stream2_IRQn;
        return 0;
    }
#endif
#ifdef DMA1_Stream3
    if (p_instance == DMA1_Stream3) {
        p_info->irqn = DMA1_Stream3_IRQn;
        return 0;
    }
#endif
#ifdef DMA1_Stream4
    if (p_instance == DMA1_Stream4) {
        p_info->irqn = DMA1_Stream4_IRQn;
        return 0;
    }
#endif
#ifdef DMA1_Stream5
    if (p_instance == DMA1_Stream5) {
        p_info->irqn = DMA1_Stream5_IRQn;
        return 0;
    }
#endif
#ifdef DMA1_Stream6
    if (p_instance == DMA1_Stream6) {
        p_info->irqn = DMA1_Stream6_IRQn;
        return 0;
    }
#endif
#ifdef DMA1_Stream7
    if (p_instance == DMA1_Stream7) {
        p_info->irqn = DMA1_Stream7_IRQn;
        return 0;
    }
#endif
#ifdef DMA2_Stream0
    if (p_instance == DMA2_Stream0) {
        p_info->irqn = DMA2_Stream0_IRQn;
        return 0;
    }
#endif
#ifdef DMA2_Stream1
    if (p_instance == DMA2_Stream1) {
        p_info->irqn = DMA2_Stream1_IRQn;
        return 0;
    }
#endif
#ifdef DMA2_Stream2
    if (p_instance == DMA2_Stream2) {
        p_info->irqn = DMA2_Stream2_IRQn;
        return 0;
    }
#endif
#ifdef DMA2_Stream3
    if (p_instance == DMA2_Stream3) {
        p_info->irqn = DMA2_Stream3_IRQn;
        return 0;
    }
#endif
#ifdef DMA2_Stream4
    if (p_instance == DMA2_Stream4) {
        p_info->irqn = DMA2_Stream4_IRQn;
        return 0;
    }
#endif
#ifdef DMA2_Stream5
    if (p_instance == DMA2_Stream5) {
        p_info->irqn = DMA2_Stream5_IRQn;
        return 0;
    }
#endif
#ifdef DMA2_Stream6
    if (p_instance == DMA2_Stream6) {
        p_info->irqn = DMA2_Stream6_IRQn;
        return 0;
    }
#endif
#ifdef DMA2_Stream7
    if (p_instance == DMA2_Stream7) {
        p_info->irqn = DMA2_Stream7_IRQn;
        return 0;
    }
#endif
#ifdef DMA1_Channel1
    if (p_instance == DMA1_Channel1) {
        p_info->irqn = DMA1_Channel1_IRQn;
        return 0;
    }
#endif
#ifdef DMA1_Channel2
    if (p_instance == DMA1_Channel2) {
        p_info->irqn = DMA1_Channel2_IRQn;
        return 0;
    }
#endif
#ifdef DMA1_Channel3
    if (p_instance == DMA1_Channel3) {
        p_info->irqn = DMA1_Channel3_IRQn;
        return 0;
    }
#endif
#ifdef DMA1_Channel4
    if (p_instance == DMA1_Channel4) {
        p_info->irqn = DMA1_Channel4_IRQn;
        return 0;
    }
#endif
#ifdef DMA1_Channel5
    if (p_instance == DMA1_Channel5) {
        p_info->irqn = DMA1_Channel5_IRQn;
        return 0;
    }
#endif
#ifdef DMA1_Channel6
    if (p_instance == DMA1_Channel6) {
        p_info->irqn = DMA1_Channel6_IRQn;
        return 0;
    }
#endif
#ifdef DMA1_Channel7
    if (p_instance == DMA1_Channel7) {
        p_info->irqn = DMA1_Channel7_IRQn;
        return 0;
    }
#endif
#ifdef DMA2_Channel1
    if (p_instance == DMA2_Channel1) {
        p_info->irqn = DMA2_Channel1_IRQn;
        return 0;
    }
#endif
#ifdef DMA2_Channel2
    if (p_instance == DMA2_Channel2) {
        p_info->irqn = DMA2_Channel2_IRQn;
        return 0;
    }
#endif
#ifdef DMA2_Channel3
    if (p_instance == DMA2_Channel3) {
        p_info->irqn = DMA2_Channel3_IRQn;
        return 0;
    }
#endif
#ifdef DMA2_Channel4
    if (p_instance == DMA2_Channel4) {
        p_info->irqn = DMA2_Channel4_IRQn;
        return 0;
    }
#endif
#ifdef DMA2_Channel5
    if (p_instance == DMA2_Channel5) {
        p_info->irqn = DMA2_Channel5_IRQn;
        return 0;
    }
#endif
    return 0;
}
// @brief 清空通道号
void drv_dma_ch_unregister(void* handle)
{
    
}
#endif 


