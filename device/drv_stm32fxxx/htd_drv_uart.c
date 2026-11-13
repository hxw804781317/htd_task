// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL   1       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG   "drv.uart"
#include "htd_device.h"
#include "htd_drv_common.h"


#if defined(BSP_USING_UART1) || defined(BSP_USING_UART2) || defined(BSP_USING_UART3) || \
    defined(BSP_USING_UART4) || defined(BSP_USING_UART5) || defined(BSP_USING_UART6) || \
    defined(BSP_USING_UART7) || defined(BSP_USING_UART8) || defined(BSP_USING_UART9)
//////////////////////////////////////////////////////////////////////
#if (!defined(UART_RX_DMA_CACHE_SIZE))
    #define UART_RX_DMA_CACHE_SIZE      64
#endif
#if (!defined(UART_TX_DMA_CACHE_SIZE))
    #define UART_TX_DMA_CACHE_SIZE      64
#endif

enum {
    #if defined(BSP_USING_UART1) && defined(UART1)
        UART1_INDEX,
    #endif
    #if defined(BSP_USING_UART2) && defined(UART2)
        UART2_INDEX,
    #endif
    #if defined(BSP_USING_UART3) && defined(UART3)
        UART3_INDEX,
    #endif
    #if defined(BSP_USING_UART4) && defined(UART4)
        UART4_INDEX,
    #endif
    #if defined(BSP_USING_UART5) && defined(UART5)
        UART5_INDEX,
    #endif
    #if defined(BSP_USING_UART6) && defined(UART6)
        UART6_INDEX,
    #endif
    #if defined(BSP_USING_UART7) && defined(UART7)
        UART7_INDEX,
    #endif
    #if defined(BSP_USING_UART8) && defined(UART8)
        UART8_INDEX,
    #endif
    #if defined(BSP_USING_UART9) && defined(UART9)
        UART9_INDEX,
    #endif
        UART_ENABLE_COUNT,
};

typedef struct {
    USART_TypeDef *Instance; 
    pin_num_t pin_tx;
    pin_num_t pin_rx;
    IRQn_Type irq_type;

    uint32_t dma_tx_request;
    uint32_t dma_rx_request; 
    void* dma_tx_handle;
    void* dma_rx_handle;
    
    int32_t (*pin_af_handle)(pin_num_t pin_tx, pin_num_t pin_rx, pin_num_t pin_rts, pin_num_t pin_cts);
} stm_uart_config_t;

typedef struct {
    DMA_HandleTypeDef dma_handle;
    stm32_dma_ch_t dma_ch;
}stm_uart_dma_info_t;

typedef struct {
    htd_fifo_t read_fifo;
    htd_fifo_t write_fifo;
    uint8_t* tx_cache;
    uint8_t* rx_cache;
    stm_uart_dma_info_t *dma_tx_info;
    stm_uart_dma_info_t *dma_rx_info;
    volatile uintptr_t dma_rx_rpos;
    uint16_t tx_cache_size;
    uint16_t rx_cache_size;
} stm_uart_info_t;

// 支持 DMA 和中断两种模式
typedef struct {
    UART_HandleTypeDef handle;              // 这个变量必须放在最前面
    htd_serial_handle_t *who;
    const stm_uart_config_t* uart_config;
    stm_uart_info_t* p_info;
}uart_object_t;
static uart_object_t m_uart_object[UART_ENABLE_COUNT] = {0};

// @brief 是否为 BDMA
static bool uart_is_bdma(USART_TypeDef *Instance)
{
#ifdef LPUART1
    if (Instance == LPUART1) {
        return true;
    }
#endif
    return false;
}

// @brief 初始化 DMA
static void stm32_uart_dma_init(uart_object_t *p_object)
{
    stm_uart_dma_info_t* p_dma_info;
    stm_uart_info_t* p_info = p_object->p_info;
    if (p_object->uart_config->dma_rx_handle != NULL) {
        p_dma_info = (stm_uart_dma_info_t*)htd_drv_malloc(sizeof(stm_uart_dma_info_t) + UART_RX_DMA_CACHE_SIZE);
        ASSERT(p_dma_info);
        memset(p_dma_info, 0, sizeof(stm_uart_dma_info_t));
        p_info->rx_cache = (uint8_t*)(p_dma_info + 1);
        p_info->rx_cache_size = UART_RX_DMA_CACHE_SIZE;
        // 注册DMA
        drv_dma_ch_register(uart_is_bdma(p_object->uart_config->Instance), &p_dma_info->dma_ch, p_object->uart_config->dma_rx_handle);
        // 使能时钟
        drv_clock_dma_enable(p_dma_info->dma_ch.instance, true);
        // 配置
        p_dma_info->dma_handle.Instance = p_dma_info->dma_ch.instance;
    #if defined(IS_DMA_CHANNEL)
        p_dma_info->dma_handle.Init.Channel = p_object->uart_config->dma_rx_request;
    #endif
        p_dma_info->dma_handle.Init.Direction = DMA_PERIPH_TO_MEMORY;
        p_dma_info->dma_handle.Init.PeriphInc = DMA_PINC_DISABLE;
        p_dma_info->dma_handle.Init.MemInc = DMA_MINC_ENABLE;
        p_dma_info->dma_handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        p_dma_info->dma_handle.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        p_dma_info->dma_handle.Init.Mode = DMA_CIRCULAR;  // 循环模式
        p_dma_info->dma_handle.Init.Priority = DMA_PRIORITY_MEDIUM;
    #if defined(DMA_FIFOMODE_DISABLE)
        p_dma_info->dma_handle.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    #endif
        if (HAL_DMA_Init(&p_dma_info->dma_handle) != HAL_OK) {
            ASSERT(0);
        }
        // 关联
        p_object->handle.hdmarx = &p_dma_info->dma_handle;
        p_dma_info->dma_handle.Parent = &p_object->handle;
        // 使能中断
        HAL_NVIC_SetPriority(p_dma_info->dma_ch.irqn, 3, 0);
        HAL_NVIC_EnableIRQ(p_dma_info->dma_ch.irqn);
        p_info->dma_rx_info = p_dma_info;
    }
    
    if (p_object->uart_config->dma_tx_handle != NULL) {
        p_dma_info = (stm_uart_dma_info_t*)htd_drv_malloc(sizeof(stm_uart_dma_info_t) + UART_TX_DMA_CACHE_SIZE);
        ASSERT(p_dma_info);
        memset(p_dma_info, 0, sizeof(stm_uart_dma_info_t));
        p_info->tx_cache = (uint8_t*)(p_dma_info + 1);
        p_info->tx_cache_size = UART_TX_DMA_CACHE_SIZE;
        // 注册DMA
        drv_dma_ch_register(uart_is_bdma(p_object->uart_config->Instance), &p_dma_info->dma_ch, p_object->uart_config->dma_tx_handle);
        // 使能时钟
        drv_clock_dma_enable(p_dma_info->dma_ch.instance, true);
        // 配置
        p_dma_info->dma_handle.Instance = p_dma_info->dma_ch.instance;
    #if defined(IS_DMA_CHANNEL)
        p_dma_info->dma_handle.Init.Channel = p_object->uart_config->dma_tx_request;
    #endif
        p_dma_info->dma_handle.Init.Direction = DMA_MEMORY_TO_PERIPH;
        p_dma_info->dma_handle.Init.PeriphInc = DMA_PINC_DISABLE;
        p_dma_info->dma_handle.Init.MemInc = DMA_MINC_ENABLE;
        p_dma_info->dma_handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        p_dma_info->dma_handle.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        p_dma_info->dma_handle.Init.Mode = DMA_NORMAL;
        p_dma_info->dma_handle.Init.Priority = DMA_PRIORITY_MEDIUM;
    #if defined(DMA_FIFOMODE_DISABLE)
        p_dma_info->dma_handle.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    #endif
        if (HAL_DMA_Init(&p_dma_info->dma_handle) != HAL_OK) {
            ASSERT(0);
        }
        // 关联
        p_object->handle.hdmatx = &p_dma_info->dma_handle;
        p_dma_info->dma_handle.Parent = &p_object->handle;
        // 使能中断
        HAL_NVIC_SetPriority(p_dma_info->dma_ch.irqn, 3, 0);
        HAL_NVIC_EnableIRQ(p_dma_info->dma_ch.irqn);
        p_info->dma_tx_info = p_dma_info;
        print_debug("dma tx init finish.");
    }
}

// @brief 去初始化DMA
static void stm32_uart_dma_uninit(uart_object_t *p_object)
{
    stm_uart_dma_info_t* p_dma_info;
    stm_uart_info_t* p_info = p_object->p_info;
    if (p_object->uart_config->dma_rx_handle != NULL) {
        p_dma_info = p_info->dma_rx_info;
        HAL_NVIC_DisableIRQ(p_dma_info->dma_ch.irqn);  
        if (HAL_DMA_Abort(&p_dma_info->dma_handle) != HAL_OK) {
            ASSERT(0);
        }
        if (HAL_DMA_DeInit(&p_dma_info->dma_handle) != HAL_OK) {
            ASSERT(0);
        }
        drv_clock_dma_enable(p_dma_info->dma_ch.instance, false);
        // 注销dma通道
        drv_dma_ch_unregister(p_object->uart_config->dma_rx_handle);
    }
    if (p_object->uart_config->dma_tx_handle != NULL) {
        p_dma_info = p_info->dma_tx_info;
        HAL_NVIC_DisableIRQ(p_dma_info->dma_ch.irqn);
        if (HAL_DMA_DeInit(&p_dma_info->dma_handle) != HAL_OK) {
            ASSERT(0);
        }
        drv_clock_dma_enable(p_dma_info->dma_ch.instance, false);
        // 注销dma通道
        drv_dma_ch_unregister(p_object->uart_config->dma_tx_handle);
    }
    
    if (p_info->dma_tx_info) {
        htd_drv_free(p_info->dma_tx_info);
        p_info->dma_tx_info = NULL;
        p_info->tx_cache = NULL;
    }
    if (p_info->dma_rx_info) {
        htd_drv_free(p_info->dma_rx_info);
        p_info->dma_rx_info = NULL;
        p_info->rx_cache = NULL;
    }
}

// @brief 
uint32_t stm32_uart_mask_get(uint32_t word_length, uint32_t parity)
{
    uint32_t mask = 0x00FFU;
    if (word_length == UART_WORDLENGTH_8B) {
        if (parity == UART_PARITY_NONE) {
            mask = 0x00FFU ;
        } else {
            mask = 0x007FU ;
        }
    }
#ifdef UART_WORDLENGTH_9B
    else if (word_length == UART_WORDLENGTH_9B) {
        if (parity == UART_PARITY_NONE) {
            mask = 0x01FFU;
        } else {
            mask = 0x00FFU;
        }
    }
#endif
#ifdef UART_WORDLENGTH_7B
    else if (word_length == UART_WORDLENGTH_7B) {
        if (parity == UART_PARITY_NONE) {
            mask = 0x007FU ;
        } else {
            mask = 0x003FU ;
        }
    } else {
        mask = 0x0000U;
    }
#endif
    return mask;
}

// @brief 获取数据位长度
static uint32_t stm32_uart_databits_get(htd_data_bits_t data_bits, htd_parity_t parity)
{
    uint32_t word_length = UART_WORDLENGTH_8B;
    switch (data_bits) {
        case HTD_DATA_BITS_8: {
            if (parity == UART_PARITY_ODD || parity == UART_PARITY_EVEN) {
                word_length = UART_WORDLENGTH_9B;
            } else {
                word_length = UART_WORDLENGTH_8B;
            }
            break;
        }
        case HTD_DATA_BITS_9: {
            word_length = UART_WORDLENGTH_9B;
            break;
        }
        default: {
            word_length = UART_WORDLENGTH_8B;
            break;
        }
    }
    return word_length;
}

// @brief 获取校验位
static uint32_t stm32_uart_parity_get(htd_parity_t parity)
{
    // 校验位
    switch (parity) {
        case HTD_PARI_ODD: {
            return UART_PARITY_ODD;
        }
        case HTD_PARI_EVEN: {
            return UART_PARITY_EVEN;
        }
        default: {
            break;
        }
    }
    return UART_PARITY_NONE;
}

// @brief 采样频率
static uint32_t stm32_uart_oversampling_get(htd_oversample_t oversample)
{
#if defined(UART_OVERSAMPLING_8)
    if (oversample == HTD_OVER_SAMPLE_8BIT) {
        return UART_OVERSAMPLING_8;
    } 
#endif
    if (oversample == HTD_OVER_SAMPLE_16BIT) {
        return UART_OVERSAMPLING_16;
    } 
    return UART_OVERSAMPLING_16;
}


//////////////////////////////////////////////////////////////////////
// @brief 打开串口
static int32_t uart_open_handle(htd_serial_handle_t* p_handle)
{
    // 只能打开一次
    uart_object_t *p_object = (uart_object_t*)p_handle->p_dev->p_object;
    if (p_object->who != NULL) {
        print_error("%s is opened.", p_handle->p_dev->p_name);
        return -1;
    }
    
    const htd_serial_param_t* p_user_config = p_handle->p_user_config;
    p_object->who = p_handle;
    ASSERT(p_user_config->wfifo_buffer_len && IS_POWER_OF_TWO(p_user_config->wfifo_buffer_len));
    ASSERT(p_user_config->rfifo_buffer_len && IS_POWER_OF_TWO(p_user_config->rfifo_buffer_len));
    ASSERT(p_user_config->pin_rx == p_object->uart_config->pin_rx);
    ASSERT(p_user_config->pin_tx == p_object->uart_config->pin_tx);
    ASSERT(p_object->uart_config);
    ASSERT(p_object->uart_config->pin_af_handle);

    // 使能串口时钟
    drv_clock_uart_enable(p_object->uart_config->Instance, true);
    // 引脚配置为复用uart模式
    p_object->uart_config->pin_af_handle(p_user_config->pin_tx, p_user_config->pin_rx, PIN_NUM_INVALID, PIN_NUM_INVALID);

    // 配置参数
    memset(&p_object->handle, 0, sizeof(UART_HandleTypeDef));
    p_object->handle.Instance = p_object->uart_config->Instance;
    p_object->handle.Init.BaudRate = p_user_config->baudrate;
    p_object->handle.Init.Mode = UART_MODE_TX_RX;    
    p_object->handle.Init.OverSampling = stm32_uart_oversampling_get(p_user_config->oversample);
    p_object->handle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    if (p_user_config->hwfc == HTD_HWFC_RTS_CTS) {
        p_object->handle.Init.HwFlowCtl = UART_HWCONTROL_RTS_CTS;
    }
    // 数据位数
    p_object->handle.Init.WordLength = stm32_uart_databits_get(p_user_config->data_bits, p_user_config->parity);
    // 停止位
    p_object->handle.Init.StopBits = UART_STOPBITS_1;
    if (p_user_config->stop_bits == HTD_STOP_BITS_2) {
        p_object->handle.Init.StopBits = UART_STOPBITS_2;
    }
#ifdef UART_ONE_BIT_SAMPLE_DISABLE
    p_object->handle.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
#endif
#ifdef UART_PRESCALER_DIV1
    p_object->handle.Init.ClockPrescaler = UART_PRESCALER_DIV1;
#endif
#ifdef UART_ADVFEATURE_NO_INIT
    p_object->handle.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
#endif
    // 校验位
    p_object->handle.Init.Parity = stm32_uart_parity_get(p_user_config->parity);
    // 初始化
    if (HAL_UART_Init(&p_object->handle) != HAL_OK) {
        p_object->who = NULL;
        drv_clock_uart_enable(p_object->uart_config->Instance, false);
        return -(ERROR_CODE_FAILED);
    }

    uint8_t* p_ptr;
    stm_uart_info_t* p_info = (stm_uart_info_t*)htd_drv_malloc(sizeof(stm_uart_info_t) + p_user_config->wfifo_buffer_len + p_user_config->rfifo_buffer_len);
    ASSERT(p_info);
    memset(p_info, 0, sizeof(stm_uart_info_t));
    p_ptr = (uint8_t*)(p_info + 1);
    htd_fifo_init(&p_info->write_fifo, "uart-tx", p_ptr, p_user_config->wfifo_buffer_len);
    p_ptr += p_user_config->wfifo_buffer_len;
    htd_fifo_init(&p_info->read_fifo, "uart-rx", p_ptr, p_user_config->rfifo_buffer_len);
    p_object->p_info = p_info;
    //uart_mask = stm32_uart_mask_get(p_object->handle.Init.WordLength, p_object->handle.Init.Parity);
    
    // 初始化 DMA
    stm32_uart_dma_init(p_object);
    if (p_info->tx_cache == NULL) {
        p_info->tx_cache_size = 32;
        p_info->tx_cache = (uint8_t*)htd_drv_malloc(p_info->tx_cache_size);
        ASSERT(p_info->tx_cache);
    }
    
    // 使能 UART 中断
    HAL_NVIC_SetPriority(p_object->uart_config->irq_type, 3, 0);
    HAL_NVIC_EnableIRQ(p_object->uart_config->irq_type);
    
    // 没有启动DMA接收，需要调用中断接收
    if (p_object->uart_config->dma_rx_handle == NULL) { 
        __HAL_UART_ENABLE_IT(&p_object->handle, UART_IT_RXNE);
    } else {
        if (HAL_OK != HAL_UARTEx_ReceiveToIdle_DMA(&p_object->handle, p_info->rx_cache, p_info->rx_cache_size)) {
            ASSERT(0);
        }
    }
    return 0;
}

// @brief 关闭串口
static int32_t uart_close_handle(htd_serial_handle_t* p_handle)
{
    int32_t ret = 0;
    uart_object_t *p_object = (uart_object_t*)p_handle->p_dev->p_object;
    if (p_object->who) {
        stm32_uart_dma_uninit(p_object);
        ret = HAL_UART_DeInit(&p_object->handle);
        if (ret != HAL_OK) {
            print_error("p_object close failed. ret=%d", ret);
            ASSERT(0);
        }
        // 禁用串口时钟
        drv_clock_uart_enable(p_object->uart_config->Instance, false);
        if (p_object->p_info) {
            if (p_object->p_info->tx_cache) {
                htd_drv_free(p_object->p_info->tx_cache);
                p_object->p_info->tx_cache = NULL;
            }
            if (p_object->p_info->rx_cache) {
                htd_drv_free(p_object->p_info->rx_cache);
                p_object->p_info->rx_cache = NULL;
            }
            htd_drv_free(p_object->p_info);
            p_object->p_info = NULL;
        }
        p_object->who = NULL;
        print_warning("uart close success.");
    }
    return ret;
}

// @brief DMA 发送
static int32_t stm32_uart_write(uart_object_t *p_object)
{
    int32_t data_len;
    stm_uart_info_t* p_info = p_object->p_info;

    if (p_object->handle.gState != HAL_UART_STATE_READY) {
        //print_debug("uart state isnot ready. gstate=0x%02x", p_object->handle.gState);
        return 0;
    }
    
    if (p_object->uart_config->dma_tx_handle == NULL) {
        // 中断发送
        data_len = htd_fifo_read(&p_info->write_fifo, p_info->tx_cache, p_info->tx_cache_size);
        if (data_len) {
            HAL_UART_Transmit_IT(&p_object->handle, p_info->tx_cache, data_len);
            print_debug("uart tx it. data_len=%d", data_len);
        }
    } else {
        // 使用DMA发送
        HAL_DMA_StateTypeDef state = HAL_DMA_GetState(&p_info->dma_tx_info->dma_handle);
        if (state == HAL_DMA_STATE_BUSY) {
            print_debug("dma state isnot ready. state=%d", state);
            return 0;
        }
        // 读取数据, 使用 DMA 发送
        data_len = htd_fifo_read(&p_info->write_fifo, p_info->tx_cache, p_info->tx_cache_size);
        if (data_len) {
            print_debug("--write--data_len=%d", data_len);
            HAL_UART_Transmit_DMA(&p_object->handle, p_info->tx_cache, data_len);
            print_debug("uart tx dma. data_len=%d", data_len);
        }
    }
    return data_len;
}

// @brief 写入数据; 先写入缓冲区，缓冲区满了，触发发送
static int32_t uart_write_handle(htd_serial_handle_t* p_handle, const uint8_t* p_data, uint32_t data_len)
{
    ASSERT(p_handle);
    
    uint32_t write_len;
    const uint8_t* p_start = p_data;
    uart_object_t *p_object = (uart_object_t*)p_handle->p_dev->p_object;
    stm_uart_info_t* p_info = p_object->p_info;
    
    // 打开发送
    while (data_len) {
        write_len = htd_fifo_write(&p_info->write_fifo, p_data, data_len);
        if (write_len == 0) {
            stm32_uart_write(p_object);
            continue;
        }
        p_data += write_len;
        data_len -= write_len;
    }
    stm32_uart_write(p_object);
    return p_data - p_start;
}

// @brief 读取数据; 直接从缓冲区读取数据
static int32_t uart_read_handle(htd_serial_handle_t* p_handle, uint8_t* p_data, uint32_t data_size, uint32_t timeout_ms)
{
    ASSERT(p_handle);
    
    uint32_t read_len;
    uint8_t* p_start = p_data;
    uint32_t start_ms = htd_time_ms_get();
    uart_object_t *p_object = (uart_object_t*)p_handle->p_dev->p_object;
    stm_uart_info_t* p_info = p_object->p_info;
    while (data_size) {
        read_len = htd_fifo_read(&p_info->read_fifo, p_data, data_size);
        if (read_len == 0) {
            if (htd_time_ms_diff(htd_time_ms_get(), start_ms) >= timeout_ms) {
                break;
            }
            htd_sleep_ms(5);
            continue;
        }
        p_data += read_len;
        data_size -= read_len;
    }
    return p_data - p_start;
}

// @brief 控制接口
static int32_t uart_ctrl_handle(void* handle, uint32_t cmd, void* p_param, uint32_t param_len)
{
    htd_serial_handle_t* p_handle = (htd_serial_handle_t*)handle;
    if (cmd == HTD_SERIAL_CMD_OPEN) {
        return uart_open_handle(p_handle);
    } else if (cmd == HTD_SERIAL_CMD_CLOSE) {
        return uart_close_handle(p_handle);
    } else if (cmd == HTD_SERIAL_CMD_READ) {
        htd_serial_xfer_t* p_xfer = (htd_serial_xfer_t*)p_param;
        return uart_read_handle(p_handle, p_xfer->p_data, p_xfer->data_len, p_xfer->timeout_ms);
    } else if (cmd == HTD_SERIAL_CMD_WRITE) {
        htd_serial_xfer_t* p_xfer = (htd_serial_xfer_t*)p_param;
        return uart_write_handle(p_handle, p_xfer->p_data, p_xfer->data_len);
    }
    return (-ERROR_CODE_NOT_SUPPORTED);
}


//////////////////////////////////////////////////////////////////////
// @brief dma接收数据
static void uart_dma_rx_int_handle(uart_object_t *p_object, uint16_t recv_size)
{
    stm_uart_info_t* p_info = p_object->p_info;
    uint8_t* p_addr = &p_info->rx_cache[recv_size];
    if (p_addr > p_info->rx_cache + p_info->rx_cache_size) {
        return;
    }
    if (p_addr == p_info->rx_cache + p_info->rx_cache_size) {
        p_addr = p_info->rx_cache;
    }
    
    // 计算接收长度，并将数据读到缓冲中
    uint32_t recv_len, write_len;
    uint8_t* p_start = p_info->rx_cache + p_info->dma_rx_rpos;
    print_debug("recv_size=%u, p_addr=%p, p_start=%p", recv_size, p_addr, p_start);
    if (p_addr == p_start) {
        return;
    } else if (p_addr > p_start) {
        recv_len = p_addr - p_start;
        write_len = htd_fifo_write(&p_info->read_fifo, p_start, recv_len);
        ASSERT(write_len == recv_len);
    } else {
        recv_len = p_info->rx_cache + p_info->rx_cache_size - p_start;
        write_len = htd_fifo_write(&p_info->read_fifo, p_start, recv_len);
        recv_len = p_addr - p_info->rx_cache;
        write_len += htd_fifo_write(&p_info->read_fifo, p_info->rx_cache, recv_len);
        // 计算总共接收到的数据长度
        recv_len = p_addr + p_info->rx_cache_size - p_start;
        ASSERT(write_len == recv_len);
    }
    p_info->dma_rx_rpos += recv_len;
    if (p_info->dma_rx_rpos >= p_info->rx_cache_size) {
        p_info->dma_rx_rpos -= p_info->rx_cache_size;
    }

    if (p_object->who->p_user_config->evt_notify_func) {
        p_object->who->p_user_config->evt_notify_func(p_object->who, HTD_SERIAL_EVT_RX, NULL, 0, true);
    }
}

#if 0
// @brief 中断接收数据
static void uart_rx_int_handle(uart_object_t *p_object, uint16_t recv_size)
{
    // 接收数据
    int32_t write_len;
    stm_uart_info_t* p_info = p_object->p_info;
    write_len = htd_fifo_write(&p_info->read_fifo, p_info->rx_cache, recv_size);
    ASSERT(write_len == recv_size);
    // 继续接收
    HAL_UARTEx_ReceiveToIdle_IT(&p_object->handle, p_info->rx_cache, p_info->rx_cache_size);
    // 通知应用层
    if (p_object->who->rx_notify_handle) {
        p_object->who->rx_notify_handle(p_object->who, NULL, 0, true);
    }
}
#endif

// @brief 接收事件
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t recv_size) 
{
    if (recv_size == 0 || huart == NULL) {
        return;
    }

    uart_object_t *p_object = (uart_object_t*)huart;
    if (p_object->uart_config->dma_rx_handle) {
        uart_dma_rx_int_handle(p_object, recv_size);
    } 
}

// @brief 串口发送数据完毕回调
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    ASSERT(huart != NULL);

    int32_t ret = 0;
    uint32_t data_len = 0;
    uart_object_t *p_object = (uart_object_t *)huart;
    stm_uart_info_t* p_info = p_object->p_info;

    if (p_object->uart_config->dma_tx_handle == NULL) {
        data_len = htd_fifo_read(&p_info->write_fifo, p_info->tx_cache, p_info->tx_cache_size);
        if (data_len) {
            ret = HAL_UART_Transmit_IT(&p_object->handle, p_info->tx_cache, data_len);
        }
    } else {
        size_t trans_total_index = __HAL_DMA_GET_COUNTER(&p_info->dma_tx_info->dma_handle);
        if (trans_total_index == 0) {
            // 读取数据, 使用 DMA 发送
            data_len = htd_fifo_read(&p_info->write_fifo, p_info->tx_cache, p_info->tx_cache_size);
            if (data_len) {
                ret = HAL_UART_Transmit_DMA(&p_object->handle, p_info->tx_cache, data_len);
            }
        }
    }
    if (ret != 0) {
        print_error("transmit failed. ret=%d, data_len=%d", ret, data_len);
        ASSERT(0);
    } else {
        print_debug("tx finish. data_len=%d", data_len);
    }
}

// @brief 串口错误回调
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    ASSERT(huart != NULL);
    uart_object_t *p_object = (uart_object_t *)huart;
    print_error("%s: %s. err_code=%d", __FUNCTION__, p_object->who->p_dev->p_name, huart->ErrorCode);
    UNUSED(p_object);

    // 错误码为8：上位机发送数据太快，接收的数据没有来得及读取，导致接收数据被覆盖
}

// @brief 中断句柄
static void uart_int_handle(uart_object_t *p_object)
{
    // 中断接收数据
    if (p_object->uart_config->dma_rx_handle == NULL) {
        stm_uart_info_t* p_info = p_object->p_info;
        if ((__HAL_UART_GET_FLAG(&p_object->handle, UART_FLAG_RXNE) != RESET) && 
            (__HAL_UART_GET_IT_SOURCE(&p_object->handle, UART_IT_RXNE) != RESET)) {
            // 这里目前只处理8bit的模式
            uint8_t ch = LL_USART_ReceiveData8(p_object->handle.Instance);
            htd_fifo_put(&p_info->read_fifo, ch);
            if (p_object->who->p_user_config->evt_notify_func) {
                p_object->who->p_user_config->evt_notify_func(p_object->who, HTD_SERIAL_EVT_RX, NULL, 0, true);
            }
            return ;
        }
    }

    //print_debug("uart int");
    HAL_UART_IRQHandler(&p_object->handle);
}


//////////////////////////////////////////////////////////////////////
#if defined(BSP_USING_UART1) && defined(UART1)
    void UART1_IRQHandler(void){uart_int_handle(&m_uart_object[UART1_INDEX]);}
    #if defined(BSP_UART1_RX_USING_DMA)
        void UART1_RX_DMA_IRQHandler(void){HAL_DMA_IRQHandler(&m_uart_object[UART1_INDEX].p_info->dma_rx_info->dma_handle);}
    #endif /*defined(BSP_UART1_RX_USING_DMA)*/
    #if defined(BSP_UART1_TX_USING_DMA)
        void UART1_TX_DMA_IRQHandler(void){HAL_DMA_IRQHandler(&m_uart_object[UART1_INDEX].p_info->dma_tx_info->dma_handle);}
    #endif /*defined(BSP_UART1_TX_USING_DMA)*/
    static const stm_uart_config_t m_uart1_config = {
        .Instance = UART1,
        .pin_tx =  PIN_NUM_GET(A, 9),
        .pin_rx = PIN_NUM_GET(A, 10),
        .irq_type = UART1_IRQn,
    #if defined(BSP_UART1_TX_USING_DMA) && defined(UART1_TX_DMA_INSTANCE)
        .dma_tx_request = UART1_TX_DMA_CHANNEL,
        .dma_tx_handle = UART1_TX_DMA_INSTANCE,
    #else
        .dma_tx_handle = NULL,
    #endif
    #if defined(BSP_UART1_RX_USING_DMA) && defined(UART1_RX_DMA_INSTANCE)
        .dma_rx_request = UART1_RX_DMA_CHANNEL,
        .dma_rx_handle = UART1_RX_DMA_INSTANCE,
    #else
        .dma_rx_handle = NULL,
    #endif
        .pin_af_handle = drv_pin_af_uart1,
    };
    HTD_DEV_REGISTER(uart1, HTD_SERIAL_DEV_IDENTIFY, uart_ctrl_handle, &m_uart_object[UART1_INDEX]);
#endif /* BSP_USING_UART1 */
#if defined(BSP_USING_UART2) && defined(UART2)
    void UART2_IRQHandler(void){uart_int_handle(&m_uart_object[UART2_INDEX]);}
    #if defined(BSP_UART2_RX_USING_DMA)
        void UART2_RX_DMA_IRQHandler(void){HAL_DMA_IRQHandler(&m_uart_object[UART2_INDEX].p_info->dma_rx_info->dma_handle);}
    #endif /*defined(BSP_UART2_RX_USING_DMA)*/
    #if defined(BSP_UART2_TX_USING_DMA)
        void UART2_TX_DMA_IRQHandler(void){HAL_DMA_IRQHandler(&m_uart_object[UART2_INDEX].p_info->dma_tx_info->dma_handle);}
    #endif /*defined(BSP_UART2_TX_USING_DMA)*/
    static const stm_uart_config_t m_uart2_config = {
        .Instance = UART2,
        .pin_tx =  PIN_NUM_GET(A, 2),
        .pin_rx = PIN_NUM_GET(A, 3),
        .irq_type = UART2_IRQn,
    #if defined(BSP_UART2_TX_USING_DMA) && defined(UART2_TX_DMA_INSTANCE)
        .dma_tx_request = UART2_TX_DMA_CHANNEL,
        .dma_tx_handle = UART2_TX_DMA_INSTANCE,
    #else
        .dma_tx_handle = NULL,
    #endif
    #if defined(BSP_UART2_RX_USING_DMA) && defined(UART2_RX_DMA_INSTANCE)
        .dma_rx_request = UART2_RX_DMA_CHANNEL,
        .dma_rx_handle = UART2_RX_DMA_INSTANCE,
    #else
        .dma_rx_handle = NULL,
    #endif
        .pin_af_handle = drv_pin_af_uart2,
    };
    HTD_DEV_REGISTER(uart2, HTD_SERIAL_DEV_IDENTIFY, uart_ctrl_handle, &m_uart_object[UART2_INDEX]);
#endif /* BSP_USING_UART2 */
#if defined(BSP_USING_UART3) && defined(UART3)
    void UART3_IRQHandler(void){uart_int_handle(&m_uart_object[UART3_INDEX]);}
    #if defined(BSP_UART3_RX_USING_DMA)
        void UART3_RX_DMA_IRQHandler(void){HAL_DMA_IRQHandler(&m_uart_object[UART3_INDEX].p_info->dma_rx_info->dma_handle);}
    #endif /*defined(BSP_UART3_RX_USING_DMA)*/
    #if defined(BSP_UART3_TX_USING_DMA)
        void UART3_TX_DMA_IRQHandler(void){HAL_DMA_IRQHandler(&m_uart_object[UART3_INDEX].p_info->dma_tx_info->dma_handle);}
    #endif /*defined(BSP_UART3_TX_USING_DMA)*/
    static const stm_uart_config_t m_uart3_config = {
        .Instance = UART3,
        .pin_tx =  PIN_NUM_GET(B, 10),
        .pin_rx = PIN_NUM_GET(B, 11),
        .irq_type = UART3_IRQn,
    #if defined(BSP_UART3_TX_USING_DMA) && defined(UART3_TX_DMA_INSTANCE)
        .dma_tx_request = UART3_TX_DMA_CHANNEL,
        .dma_tx_handle = UART3_TX_DMA_INSTANCE,
    #else
        .dma_tx_handle = NULL,
    #endif
    #if defined(BSP_UART3_RX_USING_DMA) && defined(UART3_RX_DMA_INSTANCE)
        .dma_rx_request = UART3_RX_DMA_CHANNEL,
        .dma_rx_handle = UART3_RX_DMA_INSTANCE,
    #else
        .dma_rx_handle = NULL,
    #endif
        .pin_af_handle = drv_pin_af_uart3,
    };
    HTD_DEV_REGISTER(uart3, HTD_SERIAL_DEV_IDENTIFY, uart_ctrl_handle, &m_uart_object[UART3_INDEX]);
#endif /* BSP_USING_UART3 */
#if defined(BSP_USING_UART4) && defined(UART4)
    void UART4_IRQHandler(void){uart_int_handle(&m_uart_object[UART4_INDEX]);}
    #if defined(BSP_UART4_RX_USING_DMA)
        void UART4_RX_DMA_IRQHandler(void){HAL_DMA_IRQHandler(&m_uart_object[UART4_INDEX].p_info->dma_rx_info->dma_handle);}
    #endif /*defined(BSP_UART4_RX_USING_DMA)*/
    #if defined(BSP_UART4_TX_USING_DMA)
        void UART4_TX_DMA_IRQHandler(void){HAL_DMA_IRQHandler(&m_uart_object[UART4_INDEX].p_info->dma_tx_info->dma_handle);}
    #endif /*defined(BSP_UART4_TX_USING_DMA)*/
    static const stm_uart_config_t m_uart4_config = {
        .Instance = UART4,
        .pin_tx =  PIN_NUM_GET(A, 0),
        .pin_rx = PIN_NUM_GET(A, 1),
        .irq_type = UART4_IRQn,
    #if defined(BSP_UART4_TX_USING_DMA) && defined(UART4_TX_DMA_INSTANCE)
        .dma_tx_request = UART4_TX_DMA_CHANNEL,
        .dma_tx_handle = UART4_TX_DMA_INSTANCE,
    #else
        .dma_tx_handle = NULL,
    #endif
    #if defined(BSP_UART4_RX_USING_DMA) && defined(UART4_RX_DMA_INSTANCE)
        .dma_rx_request = UART4_RX_DMA_CHANNEL,
        .dma_rx_handle = UART4_RX_DMA_INSTANCE,
    #else
        .dma_rx_handle = NULL,
    #endif
        .pin_af_handle = drv_pin_af_uart4,
    };
    HTD_DEV_REGISTER(uart4, HTD_SERIAL_DEV_IDENTIFY, uart_ctrl_handle, &m_uart_object[UART4_INDEX]);
#endif /* BSP_USING_UART4 */
#if defined(BSP_USING_UART5) && defined(UART5)
    void UART5_IRQHandler(void){uart_int_handle(&m_uart_object[UART5_INDEX]);}
    #if defined(BSP_UART5_RX_USING_DMA)
        void UART5_RX_DMA_IRQHandler(void){HAL_DMA_IRQHandler(&m_uart_object[UART5_INDEX].p_info->dma_rx_info->dma_handle);}
    #endif /*defined(BSP_UART5_RX_USING_DMA)*/
    #if defined(BSP_UART5_TX_USING_DMA)
        void UART5_TX_DMA_IRQHandler(void){HAL_DMA_IRQHandler(&m_uart_object[UART5_INDEX].p_info->dma_tx_info->dma_handle);}
    #endif /*defined(BSP_UART5_TX_USING_DMA)*/
    static const stm_uart_config_t m_uart5_config = {
        .Instance = UART5,
        .pin_tx =  PIN_NUM_GET(B, 13),
        .pin_rx = PIN_NUM_GET(B, 12),
        .irq_type = UART5_IRQn,
    #if defined(BSP_UART5_TX_USING_DMA) && defined(UART5_TX_DMA_INSTANCE)
        .dma_tx_request = UART5_TX_DMA_CHANNEL,
        .dma_tx_handle = UART5_TX_DMA_INSTANCE,
    #else
        .dma_tx_handle = NULL,
    #endif
    #if defined(BSP_UART5_RX_USING_DMA) && defined(UART5_RX_DMA_INSTANCE)
        .dma_rx_request = UART5_RX_DMA_CHANNEL,
        .dma_rx_handle = UART5_RX_DMA_INSTANCE,
    #else
        .dma_rx_handle = NULL,
    #endif
        .pin_af_handle = drv_pin_af_uart5,
    };
    HTD_DEV_REGISTER(uart5, HTD_SERIAL_DEV_IDENTIFY, uart_ctrl_handle, &m_uart_object[UART5_INDEX]);
#endif /* BSP_USING_UART5 */
#if defined(BSP_USING_UART6) && defined(UART6)
    void UART6_IRQHandler(void){uart_int_handle(&m_uart_object[UART6_INDEX]);}
    #if defined(BSP_UART6_RX_USING_DMA)
        void UART6_RX_DMA_IRQHandler(void){HAL_DMA_IRQHandler(&m_uart_object[UART6_INDEX].p_info->dma_rx_info->dma_handle);}
    #endif /*defined(BSP_UART6_RX_USING_DMA)*/
    #if defined(BSP_UART6_TX_USING_DMA)
        void UART6_TX_DMA_IRQHandler(void){HAL_DMA_IRQHandler(&m_uart_object[UART6_INDEX].p_info->dma_tx_info->dma_handle);}
    #endif /*defined(BSP_UART6_TX_USING_DMA)*/
    static const stm_uart_config_t m_uart6_config = {
        .Instance = UART6,
        .pin_tx =  PIN_NUM_GET(C, 6),
        .pin_rx = PIN_NUM_GET(C, 7),
        .irq_type = UART6_IRQn,
    #if defined(BSP_UART6_TX_USING_DMA) && defined(UART6_TX_DMA_INSTANCE)
        .dma_tx_request = UART6_TX_DMA_CHANNEL,
        .dma_tx_handle = UART6_TX_DMA_INSTANCE,
    #else
        .dma_tx_handle = NULL,
    #endif
    #if defined(BSP_UART6_RX_USING_DMA) && defined(UART6_RX_DMA_INSTANCE)
        .dma_rx_request = UART6_RX_DMA_CHANNEL,
        .dma_rx_handle = UART6_RX_DMA_INSTANCE,
    #else
        .dma_rx_handle = NULL,
    #endif
        .pin_af_handle = drv_pin_af_uart6,
    };
    HTD_DEV_REGISTER(uart6, HTD_SERIAL_DEV_IDENTIFY, uart_ctrl_handle, &m_uart_object[UART6_INDEX]);
#endif /* BSP_USING_UART6 */
#if defined(BSP_USING_UART7) && defined(UART7)
    void UART7_IRQHandler(void){uart_int_handle(&m_uart_object[UART7_INDEX]);}
    #if defined(BSP_UART7_RX_USING_DMA)
        void UART7_RX_DMA_IRQHandler(void){HAL_DMA_IRQHandler(&m_uart_object[UART7_INDEX].p_info->dma_rx_info->dma_handle);}
    #endif /*defined(BSP_UART7_RX_USING_DMA)*/
    #if defined(BSP_UART7_TX_USING_DMA)
        void UART7_TX_DMA_IRQHandler(void){HAL_DMA_IRQHandler(&m_uart_object[UART7_INDEX].p_info->dma_tx_info->dma_handle);}
    #endif /*defined(BSP_UART7_TX_USING_DMA)*/
    static const stm_uart_config_t m_uart7_config = {
        .Instance = UART7,
        .pin_tx =  PIN_NUM_GET(B, 4),
        .pin_rx = PIN_NUM_GET(B, 3),
        .irq_type = UART7_IRQn,
    #if defined(BSP_UART7_TX_USING_DMA) && defined(UART7_TX_DMA_INSTANCE)
        .dma_tx_request = UART7_TX_DMA_CHANNEL,
        .dma_tx_handle = UART7_TX_DMA_INSTANCE,
    #else
        .dma_tx_handle = NULL,
    #endif
    #if defined(BSP_UART7_RX_USING_DMA) && defined(UART7_RX_DMA_INSTANCE)
        .dma_rx_request = UART7_RX_DMA_CHANNEL,
        .dma_rx_handle = UART7_RX_DMA_INSTANCE,
    #else
        .dma_rx_handle = NULL,
    #endif
        .pin_af_handle = drv_pin_af_uart7,
    };
    HTD_DEV_REGISTER(uart7, HTD_SERIAL_DEV_IDENTIFY, uart_ctrl_handle, &m_uart_object[UART7_INDEX]);
#endif /* BSP_USING_UART7 */
#if defined(BSP_USING_UART8) && defined(UART8)
    void UART8_IRQHandler(void){uart_int_handle(&m_uart_object[UART8_INDEX]);}
    #if defined(BSP_UART8_RX_USING_DMA)
        void UART8_RX_DMA_IRQHandler(void){HAL_DMA_IRQHandler(&m_uart_object[UART8_INDEX].p_info->dma_rx_info->dma_handle);}
    #endif /*defined(BSP_UART8_RX_USING_DMA)*/
    #if defined(BSP_UART8_TX_USING_DMA)
        void UART8_TX_DMA_IRQHandler(void){HAL_DMA_IRQHandler(&m_uart_object[UART8_INDEX].p_info->dma_tx_info->dma_handle);}
    #endif /*defined(BSP_UART8_TX_USING_DMA)*/
    static const stm_uart_config_t m_uart8_config = {
        .Instance = UART8,
        .pin_tx =  PIN_NUM_GET(E, 1),
        .pin_rx = PIN_NUM_GET(E, 0),
        .irq_type = UART8_IRQn,
    #if defined(BSP_UART8_TX_USING_DMA) && defined(UART8_TX_DMA_INSTANCE)
        .dma_tx_request = UART8_TX_DMA_CHANNEL,
        .dma_tx_handle = UART8_TX_DMA_INSTANCE,
    #else
        .dma_tx_handle = NULL,
    #endif
    #if defined(BSP_UART8_RX_USING_DMA) && defined(UART8_RX_DMA_INSTANCE)
        .dma_rx_request = UART8_RX_DMA_CHANNEL,
        .dma_rx_handle = UART8_RX_DMA_INSTANCE,
    #else
        .dma_rx_handle = NULL,
    #endif
        .pin_af_handle = drv_pin_af_uart8,
    };
    HTD_DEV_REGISTER(uart8, HTD_SERIAL_DEV_IDENTIFY, uart_ctrl_handle, &m_uart_object[UART8_INDEX]);
#endif /* BSP_USING_UART8 */
#if defined(BSP_USING_UART9) && defined(UART9)
    void UART9_IRQHandler(void){uart_int_handle(&m_uart_object[UART9_INDEX]);}
    #if defined(BSP_UART9_RX_USING_DMA)
        void UART9_RX_DMA_IRQHandler(void){HAL_DMA_IRQHandler(&m_uart_object[UART9_INDEX].p_info->dma_rx_info->dma_handle);}
    #endif /*defined(BSP_UART9_RX_USING_DMA)*/
    #if defined(BSP_UART9_TX_USING_DMA)
        void UART9_TX_DMA_IRQHandler(void){HAL_DMA_IRQHandler(&m_uart_object[UART9_INDEX].p_info->dma_tx_info->dma_handle);}
    #endif /*defined(BSP_UART9_TX_USING_DMA)*/
    static const stm_uart_config_t m_uart9_config = {
        .Instance = UART9,
        .pin_tx =  PIN_NUM_GET(),
        .pin_rx = PIN_NUM_GET(),
        .irq_type = UART9_IRQn,
    #if defined(BSP_UART9_TX_USING_DMA) && defined(UART9_TX_DMA_INSTANCE)
        .dma_tx_request = UART9_TX_DMA_CHANNEL,
        .dma_tx_handle = UART9_TX_DMA_INSTANCE,
    #else
        .dma_tx_handle = NULL,
    #endif
    #if defined(BSP_UART9_RX_USING_DMA) && defined(UART9_RX_DMA_INSTANCE)
        .dma_rx_request = UART9_RX_DMA_CHANNEL,
        .dma_rx_handle = UART9_RX_DMA_INSTANCE,
    #else
        .dma_rx_handle = NULL,
    #endif
        .pin_af_handle = drv_pin_af_uart9,
    };
    HTD_DEV_REGISTER(uart9, HTD_SERIAL_DEV_IDENTIFY, uart_ctrl_handle, &m_uart_object[UART9_INDEX]);
#endif /* BSP_USING_UART9 */

// @brief 初始化
static int32_t drv_uart_init(void)
{
    for (uint32_t i=0; i<ARRAY_COUNT(m_uart_object); ++i) {
        memset(&m_uart_object[i], 0, sizeof(m_uart_object));
        
    #if defined(BSP_USING_UART1) && defined(UART1)
        if (i == UART1_INDEX) {
            m_uart_object[i].uart_config = &m_uart1_config;
        }
    #endif /*defined(BSP_USING_UART1) && defined(USART1)*/
    #if defined(BSP_USING_UART2) && defined(UART2)
        if (i == UART2_INDEX) {
            m_uart_object[i].uart_config = &m_uart2_config;
        }
    #endif /*defined(BSP_USING_UART2) && defined(USART2)*/
    #if defined(BSP_USING_UART3) && defined(UART3)
        if (i == UART3_INDEX) {
            m_uart_object[i].uart_config = &m_uart3_config;
        }
    #endif /*defined(BSP_USING_UART3) && defined(USART3)*/
    #if defined(BSP_USING_UART4) && defined(UART4)
        if (i == UART4_INDEX) {
            m_uart_object[i].uart_config = &m_uart4_config;
        }
    #endif /*defined(BSP_USING_UART4) && defined(UART4)*/
    #if defined(BSP_USING_UART5) && defined(UART5)
        if (i == UART5_INDEX) {
            m_uart_object[i].uart_config = &m_uart5_config;
        }
    #endif /*defined(BSP_USING_UART5) && defined(UART5)*/
    #if defined(BSP_USING_UART6) && defined(UART6)
        if (i == UART6_INDEX) {
            m_uart_object[i].uart_config = &m_uart6_config;
        }
    #endif /*defined(BSP_USING_UART6) && defined(USART6)*/
    #if defined(BSP_USING_UART7) && defined(UART7)
        if (i == UART7_INDEX) {
            m_uart_object[i].uart_config = &m_uart7_config;
        }
    #endif /*defined(BSP_USING_UART7) && defined(UART7)*/
    #if defined(BSP_USING_UART8) && defined(UART8)
        if (i == UART8_INDEX) {
            m_uart_object[i].uart_config = &m_uart8_config;
        }
    #endif /*defined(BSP_USING_UART8) && defined(UART8)*/
    #if defined(BSP_USING_UART9) && defined(UART9)
        if (i == UART9_INDEX) {
            m_uart_object[i].uart_config = &m_uart9_config;
        }
    #endif /*defined(BSP_USING_UART9) && defined(UART9)*/
    }
    return 0;
}
DEVICE_INIT_FUNC_REGISTER(drv_uart_init);
#endif /**/


