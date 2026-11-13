#define DEBUG_LVL   4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG   "drv.spim"
#include "htd_drv_dma.h"
#include "htd_drv_clock.h"
#include "htd_drv_common.h"
#include "htd_device.h"


#if (defined(BSP_USING_SPIM1) || defined(BSP_USING_SPIM2) || defined(BSP_USING_SPIM3) || \
    defined(BSP_USING_SPIM4) || defined(BSP_USING_SPIM5) || defined(BSP_USING_SPIM6))
//////////////////////////////////////////////////////////////////////
#define SPIM_DMA_MIN_DATA_LEN        32
#define SPIM_DMA_MAX_DATA_LEN        ALIGN_DOWN(65535, sizeof(void*))
enum {
    #if defined(BSP_USING_SPIM1) && defined(SPI1)
        SPIM1_INDEX,
    #endif
    #if defined(BSP_USING_SPIM2) && defined(SPI2)
        SPIM2_INDEX,
    #endif
    #if defined(BSP_USING_SPIM3) && defined(SPI3)
        SPIM3_INDEX,
    #endif
    #if defined(BSP_USING_SPIM4) && defined(SPI4)
        SPIM4_INDEX,
    #endif
    #if defined(BSP_USING_SPIM5) && defined(SPI5)
        SPIM5_INDEX,
    #endif
    #if defined(BSP_USING_SPIM6) && defined(SPI6)
        SPIM6_INDEX,
    #endif 
        SPIM_ENABLE_COUNT,
};

typedef struct {
    SPI_TypeDef *Instance;
    pin_num_t pin_sck;
    pin_num_t pin_mosi;
    pin_num_t pin_miso;
    IRQn_Type irq_type;

    uint32_t dma_tx_request;
    uint32_t dma_rx_request; 
    void* dma_tx_handle;
    void* dma_rx_handle;
    
    int32_t (*pin_af_handle)(pin_num_t pin_sck, pin_num_t pin_mosi, pin_num_t pin_miso);
}stm_spim_config_t;

typedef struct {
    DMA_HandleTypeDef dma_handle;
    stm32_dma_ch_t dma_ch;
}stm_spim_dma_info_t;

typedef struct {
    uint8_t reference;
    uint8_t is_xfer_last;
    
    htd_sem_t *dma_sem;                     // 异步同步信号
    stm_spim_dma_info_t* dma_tx_info;
    stm_spim_dma_info_t* dma_rx_info;
}stm_spim_info_t;

typedef struct {
    SPI_HandleTypeDef handle;              // 这个变量必须放在最前面
    spim_handle_t* who;
    const stm_spim_config_t* spim_config;
    stm_spim_info_t* p_info;
    htd_mutex_lock_t *lock;
}spim_object_t;
typedef int32_t (*dma_xfer_handle_t)(spim_object_t* p_object, uint8_t* p_data, uint32_t data_len);
static spim_object_t m_spim_object[SPIM_ENABLE_COUNT];

// @brief 控制 CS 线
static void spim_cs_enable_set(spim_handle_t* p_handle, bool active)
{
    const spim_user_config_t* p_user_config = p_handle->p_user_config;
    if (p_user_config->pin_cs != PIN_NUM_INVALID) {
        if (active) {
            //print_debug("===>cs enable. cs=%d", p_user_config->cs_active_level ? 1 : 0);
            htd_pin_write(p_user_config->pin_cs, p_user_config->cs_active_level ? 1 : 0);
        } else {
            htd_pin_write(p_user_config->pin_cs, p_user_config->cs_active_level ? 0 : 1);
            //print_debug("===>cs disable. cs=%d", p_user_config->cs_active_level ? 0 : 1);
        }
    }
}

// @brief 判断总线是否忙
static bool spim_bus_is_busy(spim_object_t* p_object)
{
    return p_object->who->data_len ? true : false;
}

// @brief 等待总线空闲
static bool spim_wait_for_bus_idle(spim_object_t* p_object, uint32_t timeout_ms)
{
    uint32_t start_ms = htd_time_ms_get();
    // 等待总线是否已经空闲；有可能是其他的 handle 占用
    for (;;) {
        if (spim_bus_is_busy(p_object) == false) {
            break;
        }
        htd_sleep_ms(1);
        if (htd_time_ms_diff(htd_time_ms_get(), start_ms) >= timeout_ms) {
            print_error("waite bus idle timeout. timeout_ms=%u", timeout_ms);
            ASSERT(0);
            return false;
        }
    }
    //print_debug("wait finish. %u", htd_time_ms_diff(htd_time_ms_get(), start_ms));
    return true;
}

// @brief 判断是否为 BDMA 通道
static bool spim_is_bdma(SPI_TypeDef *p_instance)
{
#if defined(SPI6)
    if (p_instance == SPI6) {
        return true;
    }
#endif /*defined(SPI6)*/
    return false;
}

// @brief 初始化 DMA
static void stm32_spim_dma_init(spim_object_t *p_object)
{
    stm_spim_dma_info_t* p_dma_info;
    stm_spim_info_t* p_info = p_object->p_info;
    bool use_dma_sem = false;
    
    if (p_object->spim_config->dma_rx_handle != NULL) {
        use_dma_sem = true;
        p_dma_info = (stm_spim_dma_info_t*)htd_drv_malloc(sizeof(stm_spim_dma_info_t));
        ASSERT(p_dma_info);
        htd_memset(p_dma_info, 0, sizeof(stm_spim_dma_info_t));
        // 注册DMA
        drv_dma_ch_register(spim_is_bdma(p_object->spim_config->Instance), &p_dma_info->dma_ch, p_object->spim_config->dma_rx_handle);
        // 使能时钟
        drv_clock_dma_enable(p_dma_info->dma_ch.instance, true);
        // 配置
        p_dma_info->dma_handle.Instance = p_dma_info->dma_ch.instance;
    #if defined(IS_DMA_CHANNEL)
        p_dma_info->dma_handle.Init.Channel = p_object->spim_config->dma_rx_request;
    #endif
        p_dma_info->dma_handle.Init.Direction = DMA_PERIPH_TO_MEMORY;
        p_dma_info->dma_handle.Init.PeriphInc = DMA_PINC_DISABLE;
        p_dma_info->dma_handle.Init.MemInc = DMA_MINC_ENABLE;
        p_dma_info->dma_handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        p_dma_info->dma_handle.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        p_dma_info->dma_handle.Init.Mode = DMA_NORMAL;
        p_dma_info->dma_handle.Init.Priority = DMA_PRIORITY_HIGH;
    #if defined(DMA_FIFOMODE_DISABLE)
        p_dma_info->dma_handle.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    #endif
        if (HAL_OK != HAL_DMA_Init(&p_dma_info->dma_handle)) {
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
    
    if (p_object->spim_config->dma_tx_handle != NULL) {
        use_dma_sem = true;
        p_dma_info = (stm_spim_dma_info_t*)htd_drv_malloc(sizeof(stm_spim_dma_info_t));
        ASSERT(p_dma_info);
        htd_memset(p_dma_info, 0, sizeof(stm_spim_dma_info_t));
        // 注册DMA
        drv_dma_ch_register(spim_is_bdma(p_object->spim_config->Instance), &p_dma_info->dma_ch, p_object->spim_config->dma_tx_handle);
        // 使能时钟
        drv_clock_dma_enable(p_dma_info->dma_ch.instance, true);
        // 配置
        p_dma_info->dma_handle.Instance = p_dma_info->dma_ch.instance;
    #if defined(IS_DMA_CHANNEL)
        p_dma_info->dma_handle.Init.Channel = p_object->spim_config->dma_tx_request;
    #endif
        p_dma_info->dma_handle.Init.Direction = DMA_MEMORY_TO_PERIPH;
        p_dma_info->dma_handle.Init.PeriphInc = DMA_PINC_DISABLE;
        p_dma_info->dma_handle.Init.MemInc = DMA_MINC_ENABLE;
        p_dma_info->dma_handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        p_dma_info->dma_handle.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        p_dma_info->dma_handle.Init.Mode = DMA_NORMAL;
        p_dma_info->dma_handle.Init.Priority = DMA_PRIORITY_LOW;
    #if defined(DMA_FIFOMODE_DISABLE)
        p_dma_info->dma_handle.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    #endif
        if (HAL_OK != HAL_DMA_Init(&p_dma_info->dma_handle)) {
            ASSERT(0);
        }
        // 关联
        p_object->handle.hdmatx = &p_dma_info->dma_handle;
        p_dma_info->dma_handle.Parent = &p_object->handle;
        // 使能中断
        HAL_NVIC_SetPriority(p_dma_info->dma_ch.irqn, 3, 0);
        HAL_NVIC_EnableIRQ(p_dma_info->dma_ch.irqn);
        p_info->dma_tx_info = p_dma_info;
    }
    
    // 创建信号量
    if (use_dma_sem) {
        p_info->dma_sem  = htd_sem_create(p_object->who->p_dev->p_name, 0, 1);
        ASSERT(p_info->dma_sem);
    }
}

// @brief 去初始化dma
static int32_t stm32_spim_dma_uninit(spim_object_t *p_object)
{
    stm_spim_dma_info_t* p_dma_info;
    stm_spim_info_t* p_info = p_object->p_info;
    if (p_object->spim_config->dma_rx_handle != NULL) {
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
        drv_dma_ch_unregister(p_object->spim_config->dma_rx_handle);
    }
    if (p_object->spim_config->dma_tx_handle != NULL) {
        p_dma_info = p_info->dma_tx_info;
        HAL_NVIC_DisableIRQ(p_dma_info->dma_ch.irqn);
        if (HAL_DMA_DeInit(&p_dma_info->dma_handle) != HAL_OK) {
            ASSERT(0);
        }
        drv_clock_dma_enable(p_dma_info->dma_ch.instance, false);
        // 注销dma通道
        drv_dma_ch_unregister(p_object->spim_config->dma_tx_handle);
    }
    if (p_info->dma_tx_info) {
        htd_drv_free(p_info->dma_tx_info);
        p_info->dma_tx_info = NULL;
    }
    if (p_info->dma_rx_info) {
        htd_drv_free(p_info->dma_rx_info);
        p_info->dma_rx_info = NULL;
    }
    if (p_info->dma_sem) {
        htd_sem_destroy(p_info->dma_sem);
        p_info->dma_sem = NULL;
    }
    return 0;
}

// @brief 获取SPI分频系数
static uint32_t stm32_spim_prescaler_get(spim_object_t *p_object, uint32_t freq_hz)
{
    uint32_t SPI_CLOCK = 0UL;
#if defined(APBPERIPH_BASE)
    SPI_CLOCK = HAL_RCC_GetPCLK1Freq();
#elif defined(APB1PERIPH_BASE) || defined(APB2PERIPH_BASE)
    if ((uint32_t)p_object->spim_config->Instance >= APB2PERIPH_BASE) {
        SPI_CLOCK = HAL_RCC_GetPCLK2Freq();
    } else {
        SPI_CLOCK = HAL_RCC_GetPCLK1Freq();
    }
#endif /* APBPERIPH_BASE */

    if (freq_hz >= SPI_CLOCK / 2) {
        return SPI_BAUDRATEPRESCALER_2;
    } 
    if (freq_hz >= SPI_CLOCK / 4) {
        return SPI_BAUDRATEPRESCALER_4;
    } 
    if (freq_hz >= SPI_CLOCK / 8) {
        return SPI_BAUDRATEPRESCALER_8;
    } 
    if (freq_hz >= SPI_CLOCK / 16) {
        return SPI_BAUDRATEPRESCALER_16;
    } 
    if (freq_hz >= SPI_CLOCK / 32) {
        return SPI_BAUDRATEPRESCALER_32;
    } 
    if (freq_hz >= SPI_CLOCK / 64) {
        return SPI_BAUDRATEPRESCALER_64;
    } 
    if (freq_hz >= SPI_CLOCK / 128) {
        return SPI_BAUDRATEPRESCALER_128;
    } 
    return SPI_BAUDRATEPRESCALER_256;
}

// @brief 发送数据(阻塞)
static int32_t stm32_spim_write_block(spim_object_t* p_object, uint8_t* p_data, uint32_t data_len)
{
    HAL_StatusTypeDef err_code;
    uint32_t max_wlen = ALIGN_DOWN(65535, 4);
    while (data_len) {
        if (max_wlen > data_len) {
            max_wlen = data_len;
        }
        err_code = HAL_SPI_Transmit(&p_object->handle, p_data, max_wlen, 3000);
        ASSERT(err_code == HAL_OK);
        data_len -= max_wlen;
        p_data += max_wlen;
    }
    return err_code;
}

// @brief 接收数据(阻塞)
static int32_t stm32_spim_read_block(spim_object_t* p_object, uint8_t* p_data, uint32_t data_len)
{
    HAL_StatusTypeDef err_code;
    uint32_t max_rlen = ALIGN_DOWN(65535, 4);
    while (data_len) {
        if (max_rlen > data_len) {
            max_rlen = data_len;
        }
        // 数据比较少，使用普通阻塞接收即可
        __HAL_SPI_CLEAR_OVRFLAG(&p_object->handle);
        err_code = HAL_SPI_Receive(&p_object->handle, p_data, max_rlen, 5000);
        ASSERT(err_code == HAL_OK);
        data_len -= max_rlen;
        p_data += max_rlen;
    }
    return err_code;
}

// @brief DMA发送数据(非阻塞)
static int32_t stm32_spim_write_dma(spim_object_t* p_object, uint8_t* p_data, uint32_t data_len)
{
    HAL_StatusTypeDef err_code;
    ASSERT((((uintptr_t)p_data) & (sizeof(void*)-1)) == 0);
    ASSERT(data_len <= SPIM_DMA_MAX_DATA_LEN);
    err_code = HAL_SPI_Transmit_DMA(&p_object->handle, (uint8_t*)p_data, data_len);
    //print_debug("stm32 spim dma write. err_code=%d, data_len=%u", err_code, data_len);
    ASSERT(err_code == HAL_OK);
    return err_code;
}

// @brief DMA接收数据(非阻塞)
static int32_t stm32_spim_read_dma(spim_object_t* p_object, uint8_t* p_data, uint32_t data_len)
{
    HAL_StatusTypeDef err_code;
    ASSERT((((uintptr_t)p_data) & (sizeof(void*)-1)) == 0);
    err_code = HAL_SPI_Receive_DMA(&p_object->handle, p_data, data_len);
    ASSERT(err_code == HAL_OK);
    return err_code;
}

// @brief dma 收发句柄; xfer_handle(stm32_spim_write_dma, stm32_spim_read_dma)
static void stm32_spim_dma_int_handle(spim_object_t* p_object, dma_xfer_handle_t xfer_handle)
{
    spim_handle_t* p_handle = p_object->who;
    stm_spim_info_t* p_info = p_object->p_info;
    if (p_handle->data_pos >= p_handle->data_len) {
        // 释放总线
        if (p_info->is_xfer_last) {
            spim_cs_enable_set(p_handle, false);
            p_info->is_xfer_last = false;
            // 通告应用层
            if (p_handle->p_user_config->xfer_finish) {
                p_handle->p_user_config->xfer_finish(p_handle, p_handle->p_data, p_handle->data_len);
            }
            p_handle->data_len = 0;
            //print_warning("xfer finish.");
        } 
        // 释放结束信号
        htd_sem_give_fisr(p_info->dma_sem);
    } else {
        // 自动收发未完成的数据
        uint32_t len = p_handle->data_len - p_handle->data_pos;
        if (len > SPIM_DMA_MAX_DATA_LEN) {
            len = SPIM_DMA_MAX_DATA_LEN;
        }
        xfer_handle(p_object, &p_handle->p_data[p_handle->data_pos], len);
        p_handle->data_pos += len;
    }
}

// @brief 打开
static int32_t drv_spim_open(spim_handle_t *p_handle)
{
    ASSERT(p_handle != NULL);
    
    // 创建锁
    spim_object_t* p_object = (spim_object_t*)p_handle->p_dev->p_object;
    if (p_object->lock == NULL) {
        p_object->lock = htd_mutex_lock_create(p_handle->p_dev->p_name);
        ASSERT(p_object->lock);
    }

    htd_mutex_lock(p_object->lock);
    // 控制CS线
    SPI_HandleTypeDef *spi_handle = &p_object->handle;
    const spim_user_config_t* p_user_config = p_handle->p_user_config;
    if (p_user_config->pin_cs != PIN_NUM_INVALID) {
        htd_pin_output_pp_set(p_user_config->pin_cs);
        htd_pin_write(p_user_config->pin_cs, p_user_config->cs_active_level ? 0 : 1);
    }

    spim_handle_t* p_prev = p_object->who;
    if (p_prev) {
        ASSERT(p_object->p_info);
        if (p_prev == p_handle) {
            print_error("spi slave device is opened.");
            htd_mutex_unlock(p_object->lock);
            return -1;
        }
        p_object->p_info->reference++;
        ASSERT(p_prev->p_user_config->pin_miso == p_user_config->pin_miso);
        ASSERT(p_prev->p_user_config->pin_mosi == p_user_config->pin_mosi);
        ASSERT(p_prev->p_user_config->pin_sck == p_user_config->pin_sck);
        htd_mutex_unlock(p_object->lock);
        return 0;
    }
    ASSERT(p_user_config->pin_miso == p_object->spim_config->pin_miso);
    ASSERT(p_user_config->pin_mosi == p_object->spim_config->pin_mosi);
    ASSERT(p_user_config->pin_sck == p_object->spim_config->pin_sck);

    // 使能时钟
    drv_clock_spi_enable(p_object->spim_config->Instance, true);
    p_object->spim_config->pin_af_handle(p_user_config->pin_sck, p_user_config->pin_mosi, p_user_config->pin_miso);

    // CS 引脚必须由外部控制
    htd_memset(spi_handle, 0, sizeof(SPI_HandleTypeDef));
    spi_handle->Instance = p_object->spim_config->Instance;
    spi_handle->Init.NSS = SPI_NSS_SOFT;
    // 主/从
    spi_handle->Init.Mode = SPI_MODE_MASTER;
    // 线数量，单线还是双线
    spi_handle->Init.Direction = p_user_config->is_1line ? SPI_DIRECTION_1LINE : SPI_DIRECTION_2LINES;
    // 位宽
    spi_handle->Init.DataSize = p_user_config->data_width == 16 ? SPI_DATASIZE_16BIT : SPI_DATASIZE_8BIT;
    // 模式
    if (p_user_config->mode == SPIM_MODE_0) {         // CPOL=0, CPHA=0
        spi_handle->Init.CLKPolarity = SPI_POLARITY_LOW;
        spi_handle->Init.CLKPhase = SPI_PHASE_1EDGE;
    } else if (p_user_config->mode == SPIM_MODE_1) {  // CPOL=0, CPHA=1
        spi_handle->Init.CLKPolarity = SPI_POLARITY_LOW;
        spi_handle->Init.CLKPhase = SPI_PHASE_2EDGE;
    } else if (p_user_config->mode == SPIM_MODE_2) {  // CPOL=1, CPHA=0
        spi_handle->Init.CLKPolarity = SPI_POLARITY_HIGH;
        spi_handle->Init.CLKPhase = SPI_PHASE_1EDGE;
    } else {                                          // CPOL=1, CPHA=1
        spi_handle->Init.CLKPolarity = SPI_POLARITY_HIGH;
        spi_handle->Init.CLKPhase = SPI_PHASE_2EDGE;
    }
    // 频率
    spi_handle->Init.BaudRatePrescaler = stm32_spim_prescaler_get(p_object, p_user_config->freq_hz);
    print_debug("prescaler=%d", spi_handle->Init.BaudRatePrescaler);
    // MSB/LSB
    spi_handle->Init.FirstBit = p_user_config->is_bit_order_msb ? SPI_FIRSTBIT_MSB : SPI_FIRSTBIT_LSB;
    spi_handle->Init.TIMode = SPI_TIMODE_DISABLE;
    spi_handle->Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    spi_handle->Init.CRCPolynomial = 10;
    spi_handle->State = HAL_SPI_STATE_RESET;
    if (HAL_SPI_Init(spi_handle) != HAL_OK) {
        drv_clock_spi_enable(p_object->spim_config->Instance, false);
        print_error("spi init failed.");
        htd_mutex_unlock(p_object->lock);
        return -1;
    }

    p_object->p_info = (stm_spim_info_t*)htd_drv_malloc(sizeof(stm_spim_info_t));
    ASSERT(p_object->p_info);
    htd_memset(p_object->p_info, 0, sizeof(stm_spim_info_t));
    
    // 初始化DMA
    p_object->who = p_handle;
    stm32_spim_dma_init(p_object);
    if (p_object->spim_config->dma_rx_handle || p_object->spim_config->dma_tx_handle) {
        HAL_NVIC_SetPriority(p_object->spim_config->irq_type, 3, 0);
        HAL_NVIC_EnableIRQ(p_object->spim_config->irq_type);
    }
    
    p_object->p_info->reference = 1;
    p_object->p_info->is_xfer_last = false;
    htd_mutex_unlock(p_object->lock);
    return 0;
}

// @brief 关闭
static int32_t drv_spim_close(spim_handle_t *p_handle)
{
    int32_t ret = ERROR_CODE_SUCCESS;
    spim_object_t* p_object = (spim_object_t*)p_handle->p_dev->p_object;
    htd_mutex_lock(p_object->lock);
    stm_spim_info_t* p_info = p_object->p_info;
    if (p_info) {
        if (p_info->reference <= 1) {
            stm32_spim_dma_uninit(p_object);
            HAL_SPI_DeInit(&p_object->handle);
            // 禁用时钟
            drv_clock_spi_enable(p_object->spim_config->Instance, false);
            p_object->who = NULL;
            p_info->reference = 0;
            p_info->is_xfer_last = false;
            htd_drv_free(p_info);
            p_object->p_info = NULL;
        } else {
            p_info->reference--;
            ret = -ERROR_CODE_COMPLETED;
        }
    }
    htd_mutex_unlock(p_object->lock);
    return ret;
}

// @brief 发送完成回调
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    spim_object_t* p_object = (spim_object_t*)hspi;
    print_warning("SPIM Tx Done. data_pos=%u, data_len=%u", p_object->who->data_pos, p_object->who->data_len);
    stm32_spim_dma_int_handle(p_object, stm32_spim_write_dma);
}

// @brief 写数据
static int32_t drv_spim_write(spim_handle_t* p_handle, const uint8_t* p_wbuff, size_t wbuff_len, bool is_waite_finish)
{
    ASSERT(p_wbuff);
    ASSERT(wbuff_len > 0);

    spim_object_t* p_object = (spim_object_t*)p_handle->p_dev->p_object;
    if (p_object->spim_config->dma_tx_handle == NULL || wbuff_len < SPIM_DMA_MIN_DATA_LEN) {
        // 阻塞发送
        stm32_spim_write_block(p_object, (uint8_t*)p_wbuff, wbuff_len);
        return wbuff_len;
    }
    
    // DMA 发送
    uint32_t max_wlen = SPIM_DMA_MAX_DATA_LEN;
    htd_sem_reset(p_object->p_info->dma_sem);
    p_handle->p_data = (uint8_t*)p_wbuff;
    p_handle->data_len = wbuff_len;
    p_handle->data_pos = 0;
    if (max_wlen > wbuff_len) {
        max_wlen = wbuff_len;
    }
    // 发送数据
    p_handle->data_pos += max_wlen;
    stm32_spim_write_dma(p_object, p_handle->p_data, max_wlen);
    if (is_waite_finish) {
        // 等待发送完毕
        if (false == htd_sem_take(p_object->p_info->dma_sem, 10*1000)) {
            print_error("waite write timeout.");
            return -ERROR_CODE_TIMEOUT;
        }
    }
    return p_handle->data_pos;
}

// @brief 接收完成回调
void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
    spim_object_t* p_object = (spim_object_t*)hspi;
    //print_warning("SPIM Tx Done. data_pos=%u, data_len=%u", p_object->who->data_pos, p_object->who->data_len);
    stm32_spim_dma_int_handle(p_object, stm32_spim_read_dma);
}

// @brief 读数据
static int32_t drv_spim_read(spim_handle_t* p_handle, uint8_t* p_rbuff, size_t rbuff_len, bool is_waite_finish)
{
    ASSERT(p_rbuff);
    ASSERT(rbuff_len > 0);
    
    spim_object_t* p_object = (spim_object_t*)p_handle->p_dev->p_object;
    htd_memset(p_rbuff, 0xff, rbuff_len);
    if (p_object->spim_config->dma_rx_handle == NULL || rbuff_len < SPIM_DMA_MIN_DATA_LEN) {
        // 阻塞接收
        stm32_spim_read_block(p_object, p_rbuff, rbuff_len);
        return rbuff_len;
    }

    // DMA 接收
    uint32_t max_rlen = SPIM_DMA_MAX_DATA_LEN;
    htd_sem_reset(p_object->p_info->dma_sem);
    p_handle->p_data = p_rbuff;
    p_handle->data_len = rbuff_len;
    p_handle->data_pos = 0;
    if (max_rlen > rbuff_len) {
        max_rlen = rbuff_len;
    }
    p_handle->data_pos += max_rlen;
    stm32_spim_read_dma(p_object, p_handle->p_data, max_rlen);
    if (is_waite_finish) {
        // 等待接收完成
        if (false == htd_sem_take(p_object->p_info->dma_sem, 10*1000)) {        // 等待上一个接收完毕
            print_error("waite read timeout.");
            return -ERROR_CODE_TIMEOUT;
        }
    }
    return p_handle->data_pos;
}

// @brief 收发控制, 返回发送的个数
static int32_t drv_spim_xfer(spim_handle_t* p_handle, spim_msg_t* p_msg, uint32_t msg_count)
{
    bool is_waite_finish = true;
    uint32_t count = msg_count;

    spim_object_t* p_object = (spim_object_t*)p_handle->p_dev->p_object;
    htd_mutex_lock(p_object->lock);
    if (p_object->spim_config->dma_tx_handle || p_object->spim_config->dma_rx_handle) {
        spim_wait_for_bus_idle(p_object, 5*1000);
    }
    p_object->who = p_handle;
    p_handle->data_len = 0;
    
    spim_cs_enable_set(p_handle, true);
    p_object->p_info->is_xfer_last = false;
    while (msg_count--) {
        if (msg_count == 0) {
            is_waite_finish = (p_msg->flags & SPIM_MSG_FLAG_BLOCK) ? true : false;
            p_object->p_info->is_xfer_last = true;
        }
        if (p_msg->flags & SPIM_MSG_FLAG_TX) {
            drv_spim_write(p_handle, p_msg->buff, p_msg->len, is_waite_finish);
        } else {
            drv_spim_read(p_handle, p_msg->buff, p_msg->len, is_waite_finish);
        }
        p_msg++;
    }
    if (spim_bus_is_busy(p_object) == false) {
        spim_cs_enable_set(p_handle, false);
    } 
    htd_mutex_unlock(p_object->lock);
    return count;
}

// @brief 控制句柄
int32_t drv_spim_ctrl(void* handle, uint32_t cmd, void* p_param, uint32_t param_len)
{
    spim_handle_t* p_handle = (spim_handle_t*)handle;
    if (cmd == HTD_SPIM_CMD_OPEN) {
        return drv_spim_open(p_handle);
    } else if (cmd == HTD_SPIM_CMD_CLOSE) {
        return drv_spim_close(p_handle);
    } else if (cmd == HTD_SPIM_CMD_XFER) {
        return drv_spim_xfer(p_handle, (spim_msg_t*)p_param, param_len);
    } else {
        return (-ERROR_CODE_NOT_FOUND);
    }
}


//////////////////////////////////////////////////////////////////////
#if defined(BSP_USING_SPIM1) && defined(SPI1)
    void SPI1_IRQHandler(void){HAL_SPI_IRQHandler(&m_spim_object[SPIM1_INDEX].handle);}
    #if defined(BSP_SPIM1_USING_DMA)
        void SPIM1_RX_DMA_IRQHandler(void){HAL_DMA_IRQHandler(&m_spim_object[SPIM1_INDEX].p_info->dma_rx_info->dma_handle);}
        void SPIM1_TX_DMA_IRQHandler(void){HAL_DMA_IRQHandler(&m_spim_object[SPIM1_INDEX].p_info->dma_tx_info->dma_handle);}
    #endif 
    const static stm_spim_config_t m_spim1_config = {
        .Instance = SPI1,
        .pin_sck = PIN_NUM_GET(A, 5), 
        .pin_miso = PIN_NUM_GET(A, 6), 
        .pin_mosi = PIN_NUM_GET(A, 7), 
        .irq_type = SPI1_IRQn,
    #if defined(BSP_SPIM1_USING_DMA) && defined(SPIM1_TX_DMA_INSTANCE)
        .dma_tx_request = SPIM1_TX_DMA_CHANNEL,
        .dma_tx_handle = SPIM1_TX_DMA_INSTANCE,
    #else
        .dma_tx_handle = NULL,
    #endif 
    #if defined(BSP_SPIM1_USING_DMA) && defined(SPIM1_RX_DMA_INSTANCE)
        .dma_rx_request = SPIM1_RX_DMA_CHANNEL,
        .dma_rx_handle = SPIM1_RX_DMA_INSTANCE,
    #else
        .dma_rx_handle = NULL,
    #endif
        .pin_af_handle = drv_pin_af_spi1,
    };
    HTD_DEV_REGISTER(spim1, HTD_SPIM_DEV_IDENTIFY, drv_spim_ctrl, &m_spim_object[SPIM1_INDEX]);
#endif
#if defined(BSP_USING_SPIM2) && defined(SPI2)
    void SPI2_IRQHandler(void){HAL_SPI_IRQHandler(&m_spim_object[SPIM2_INDEX].handle);}
    #if defined(BSP_SPIM2_USING_DMA)
        void SPIM2_RX_DMA_IRQHandler(void){HAL_DMA_IRQHandler(&m_spim_object[SPIM2_INDEX].p_info->dma_rx_info->dma_handle);}
        void SPIM2_TX_DMA_IRQHandler(void){HAL_DMA_IRQHandler(&m_spim_object[SPIM2_INDEX].p_info->dma_tx_info->dma_handle);}
    #endif 
    const static stm_spim_config_t m_spim2_config = {
        .Instance = SPI2,
        .pin_sck = PIN_NUM_GET(B, 13),
        .pin_miso = PIN_NUM_GET(B, 14),
        .pin_mosi = PIN_NUM_GET(B, 15), 
        .irq_type = SPI2_IRQn,
    #if defined(BSP_SPIM2_USING_DMA) && defined(SPIM2_TX_DMA_INSTANCE)
        .dma_tx_request = SPIM2_TX_DMA_CHANNEL,
        .dma_tx_handle = SPIM2_TX_DMA_INSTANCE,
    #else
        .dma_tx_handle = NULL,
    #endif 
    #if defined(BSP_SPIM2_USING_DMA)  && defined(SPIM2_RX_DMA_INSTANCE)
        .dma_rx_request = SPIM2_RX_DMA_CHANNEL,
        .dma_rx_handle = SPIM2_RX_DMA_INSTANCE,
    #else
        .dma_rx_handle = NULL,
    #endif
        .pin_af_handle = drv_pin_af_spi2,
    };
    HTD_DEV_REGISTER(spim2, HTD_SPIM_DEV_IDENTIFY, drv_spim_ctrl, &m_spim_object[SPIM2_INDEX]);
#endif
#if defined(BSP_USING_SPIM3) && defined(SPI3)
    void SPI3_IRQHandler(void){HAL_SPI_IRQHandler(&m_spim_object[SPIM3_INDEX].handle);}
    #if defined(BSP_SPIM3_USING_DMA)
        void SPIM3_RX_DMA_IRQHandler(void){HAL_DMA_IRQHandler(&m_spim_object[SPIM3_INDEX].p_info->dma_rx_info->dma_handle);}
        void SPIM3_TX_DMA_IRQHandler(void){HAL_DMA_IRQHandler(&m_spim_object[SPIM3_INDEX].p_info->dma_tx_info->dma_handle);}
    #endif 
    const static stm_spim_config_t m_spim3_config = {
        .Instance = SPI3,
        .pin_sck = PIN_NUM_GET(B, 3),
        .pin_miso = PIN_NUM_GET(B, 4),
        .pin_mosi = PIN_NUM_GET(B, 5), 
        .irq_type = SPI3_IRQn,
    #if defined(BSP_SPIM3_USING_DMA) && defined(SPIM3_TX_DMA_INSTANCE)
        .dma_tx_request = SPIM3_TX_DMA_CHANNEL,
        .dma_tx_handle = SPIM3_TX_DMA_INSTANCE,
    #else
        .dma_tx_handle = NULL,
    #endif 
    #if defined(BSP_SPIM3_USING_DMA) && defined(SPIM3_RX_DMA_INSTANCE)
        .dma_rx_request = SPIM3_RX_DMA_CHANNEL,
        .dma_rx_handle = SPIM3_RX_DMA_INSTANCE,
    #else
        .dma_rx_handle = NULL,
    #endif
        .pin_af_handle = drv_pin_af_spi3,
    };
    HTD_DEV_REGISTER(spim3, HTD_SPIM_DEV_IDENTIFY, drv_spim_ctrl, &m_spim_object[SPIM3_INDEX]);
#endif
#if defined(BSP_USING_SPIM4) && defined(SPI4)
    void SPI4_IRQHandler(void){HAL_SPI_IRQHandler(&m_spim_object[SPIM4_INDEX].handle);}
    #if defined(BSP_SPIM4_USING_DMA)
        void SPIM4_RX_DMA_IRQHandler(void){HAL_DMA_IRQHandler(&m_spim_object[SPIM4_INDEX].p_info->dma_rx_info->dma_handle);}
        void SPIM4_TX_DMA_IRQHandler(void){HAL_DMA_IRQHandler(&m_spim_object[SPIM4_INDEX].p_info->dma_tx_info->dma_handle);}
    #endif 
    const static stm_spim_config_t m_spim4_config = {
        .Instance = SPI4,
        .pin_sck = PIN_NUM_GET(A, 1),
        .pin_miso = PIN_NUM_GET(E, 12),
        .pin_mosi = PIN_NUM_GET(E, 13), 
        .irq_type = SPI4_IRQn,
    #if defined(BSP_SPIM4_USING_DMA) && defined(SPIM4_TX_DMA_INSTANCE)
        .dma_tx_request = SPIM4_TX_DMA_CHANNEL,
        .dma_tx_handle = SPIM4_TX_DMA_INSTANCE,
    #else
        .dma_tx_handle = NULL,
    #endif 
    #if defined(BSP_SPIM4_USING_DMA) && defined(SPIM4_RX_DMA_INSTANCE)
        .dma_rx_request = SPIM4_RX_DMA_CHANNEL,
        .dma_rx_handle = SPIM4_RX_DMA_INSTANCE,
    #else
        .dma_rx_handle = NULL,
    #endif
        .pin_af_handle = drv_pin_af_spi4,
    };
    HTD_DEV_REGISTER(spim4, HTD_SPIM_DEV_IDENTIFY, drv_spim_ctrl, &m_spim_object[SPIM4_INDEX]);
#endif
#if defined(BSP_USING_SPIM5) && defined(SPI5)
    void SPI5_IRQHandler(void){HAL_SPI_IRQHandler(&m_spim_object[SPIM5_INDEX].handle);}
    #if defined(BSP_SPIM5_USING_DMA)
        void SPIM5_RX_DMA_IRQHandler(void){HAL_DMA_IRQHandler(&m_spim_object[SPIM5_INDEX].p_info->dma_rx_info->dma_handle);}
        void SPIM5_TX_DMA_IRQHandler(void){HAL_DMA_IRQHandler(&m_spim_object[SPIM5_INDEX].p_info->dma_tx_info->dma_handle);}
    #endif 
    const static stm_spim_config_t m_spim5_config = {
        .Instance = SPI5,
        .pin_sck = PIN_NUM_GET(E, 2),
        .pin_miso = PIN_NUM_GET(E, 5),
        .pin_mosi = PIN_NUM_GET(E, 6), 
        .irq_type = SPI5_IRQn,
    #if defined(BSP_SPIM5_USING_DMA) && defined(SPIM5_TX_DMA_INSTANCE)
        .dma_tx_request = SPIM5_TX_DMA_CHANNEL,
        .dma_tx_handle = SPIM5_TX_DMA_INSTANCE,
    #else
        .dma_tx_handle = NULL,
    #endif 
    #if defined(BSP_SPIM5_USING_DMA) && defined(SPIM5_RX_DMA_INSTANCE)
        .dma_rx_request = SPIM5_RX_DMA_CHANNEL,
        .dma_rx_handle = SPIM5_RX_DMA_INSTANCE,
    #else
        .dma_rx_handle = NULL,
    #endif
        .pin_af_handle = drv_pin_af_spi5,
    };
    HTD_DEV_REGISTER(spim5, HTD_SPIM_DEV_IDENTIFY, drv_spim_ctrl, &m_spim_object[SPIM5_INDEX]);
#endif
#if defined(BSP_USING_SPIM6) && defined(SPI6)
    void SPI6_IRQHandler(void){HAL_SPI_IRQHandler(&m_spim_object[SPIM6_INDEX].handle);}
    #if defined(BSP_SPIM6_USING_DMA)
        void SPIM6_RX_DMA_IRQHandler(void){HAL_DMA_IRQHandler(&m_spim_object[SPIM6_INDEX].p_info->dma_rx_info->dma_handle);}
        void SPIM6_TX_DMA_IRQHandler(void){HAL_DMA_IRQHandler(&m_spim_object[SPIM6_INDEX].p_info->dma_tx_info->dma_handle);}
    #endif 
    const static stm_spim_config_t m_spim6_config = {
        .Instance = SPI6,
        .pin_sck = PIN_NUM_GET(G, 13),
        .pin_miso = PIN_NUM_GET(G, 12),
        .pin_mosi = PIN_NUM_GET(G, 14), 
        .irq_type = SPI6_IRQn,
    #if defined(BSP_SPIM6_USING_DMA) && defined(SPIM6_TX_DMA_INSTANCE)
        .dma_tx_request = SPIM6_TX_DMA_CHANNEL,
        .dma_tx_handle = SPIM6_TX_DMA_INSTANCE,
    #else
        .dma_tx_handle = NULL,
    #endif 
    #if defined(BSP_SPIM6_USING_DMA) && defined(SPIM6_RX_DMA_INSTANCE)
        .dma_rx_request = SPIM6_RX_DMA_CHANNEL,
        .dma_rx_handle = SPIM6_RX_DMA_INSTANCE,
    #else
        .dma_rx_handle = NULL,
    #endif
        .pin_af_handle = drv_pin_af_spi6,
    };
    HTD_DEV_REGISTER(spim6, HTD_SPIM_DEV_IDENTIFY, drv_spim_ctrl, &m_spim_object[SPIM6_INDEX]);
#endif

// @brief 初始化
static int32_t drv_spim_init(void)
{
    for (uint32_t i=0; i<ARRAY_COUNT(m_spim_object); ++i) {
        htd_memset(&m_spim_object[i], 0, sizeof(spim_object_t));

    #if defined(BSP_USING_SPIM1) && defined(SPI1)
        if (i == SPIM1_INDEX) {
            m_spim_object[i].spim_config = &m_spim1_config;
        }
    #endif /*defined(BSP_USING_SPIM1) && defined(SPI1)*/
    #if defined(BSP_USING_SPIM2) && defined(SPI2)
        if (i == SPIM2_INDEX) {
            m_spim_object[i].spim_config = &m_spim2_config;
        }
    #endif /*defined(BSP_USING_SPIM2) && defined(SPI2)*/
    #if defined(BSP_USING_SPIM3) && defined(SPI3)
        if (i == SPIM3_INDEX) {
            m_spim_object[i].spim_config = &m_spim3_config;
        }
    #endif  /*defined(BSP_USING_SPIM3) && defined(SPI3)*/
    #if defined(BSP_USING_SPIM4) && defined(SPI4)
        if (i == SPIM4_INDEX) {
            m_spim_object[i].spim_config = &m_spim4_config;
        }
    #endif  /*defined(BSP_USING_SPIM4) && defined(SPI4)*/
    #if defined(BSP_USING_SPIM5) && defined(SPI5)
        if (i == SPIM5_INDEX) {
            m_spim_object[i].spim_config = &m_spim5_config;
        }
    #endif  /*defined(BSP_USING_SPIM5) && defined(SPI5)*/
    #if defined(BSP_USING_SPIM6) && defined(SPI6)
        if (i == SPIM6_INDEX) {
            m_spim_object[i].spim_config = &m_spim6_config;
        }
    #endif  /*defined(BSP_USING_SPIM6) && defined(SPI6)*/
    }
    print_debug("drv spim init.");
    return 0;
}
DEVICE_INIT_FUNC_REGISTER(drv_spim_init);
#endif /*(defined(BSP_USING_SPIM1) || defined(BSP_USING_SPIM2) || defined(BSP_USING_SPIM3))*/

