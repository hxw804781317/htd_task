#define DEBUG_LVL   4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG   "drv.crc"
#include "htd_drv_common.h"
#include "htd_device.h"


#if (defined(BSP_USING_CRC) && defined(CRC) && 1)
#if defined(SOC_SERIES_STM32F1) || defined(SOC_SERIES_STM32F2) || defined(SOC_SERIES_STM32F4)
//////////////////////////////////////////////////////////////////////
// @brief 软件模拟 stm32F1/gd32F1 系列的crc32
uint32_t crc32_stm32_hw(uint32_t data)
{
    uint32_t crc = 0xFFFFFFFF; // 初始值
    uint32_t poly = 0x04C11DB7; // 多项式 (MSB first, 不含最高位1)
    // 将32位数据送入计算 (MSB first, 逐位处理)
    for (int bit = 31; bit >= 0; bit--) {
        if (((crc >> 31) & 1) ^ ((data >> bit) & 1)) {
            crc = (crc << 1) ^ poly;
        } else {
            crc = crc << 1;
        }
    }
    return crc;
}

// @brief 开始 
int32_t htd_dev_crc32_init(htd_crc32_ctx_t* p_ctx)
{
    ASSERT(p_ctx);
    
    // 硬件不支持，返回 ERROR_CODE_BUSY 使用软件进行处理
    if (p_ctx->result != 0xffffffff) {
        return -ERROR_CODE_NOT_SUPPORTED;
    }
    
    // 判断空闲
    if (LL_CRC_Read_IDR(CRC) == 0x55) {
        return -ERROR_CODE_BUSY;
    }
    LL_CRC_Write_IDR(CRC, 0x55);

    // 使能时钟
    __HAL_RCC_CRC_CLK_ENABLE();
    // 复位CRC计算单元
    LL_CRC_ResetCRCCalculationUnit(CRC);
    return ERROR_CODE_SUCCESS;
}

// @brief 更新
static uint32_t drv_crc32_update(const uint8_t* p_data, uint32_t data_len)
{
    uint32_t crcval;
    uint32_t len = data_len >> 2;
    while (len--) {
        crcval = *(uint32_t*)p_data;
        //crcval = __REV(*(uint32_t*)p_data);   // 大小端转换
        crcval = __RBIT(crcval);                  // 反转
        LL_CRC_FeedData32(CRC, crcval);
        p_data += 4;
    }
    crcval = LL_CRC_ReadData32(CRC);
    crcval = __RBIT(crcval);                    // 结果反转

    // 剩下部分使用软件实现
    len = data_len & 0x03;
    while (len--) {
        crcval ^= (uint32_t)*p_data++;
        for (uint32_t i=0; i<8; ++i) {
            if (crcval & 0x01) {
                crcval = (crcval >> 1) ^ 0xEDB88320;
            } else {
                crcval >>= 1;
            }
        }
    }
    return ~crcval;
}

// @brief 更新
static uint32_t drv_crc32_mpeg2_update(const uint8_t* p_data, uint32_t data_len)
{
    uint32_t crcval;
    uint32_t len = data_len >> 2;
    while (len--) {
        crcval = __REV(*(uint32_t*)p_data);
        LL_CRC_FeedData32(CRC, crcval);
        p_data += 4;
    }
    crcval = LL_CRC_ReadData32(CRC);
    
    // 剩下的使用软件计算
    uint32_t i;
    len = data_len & 0x03;
    while (len--) {
        crcval ^= (uint32_t)(*p_data++) << 24;
        for (i=0; i<8; i++) {
            if (crcval & 0x80000000) {
                crcval = (crcval << 1) ^ 0x04C11DB7;
            } else {
                crcval <<= 1;
            }
        }
    }
    return crcval;
}

// @brief 更新
uint32_t htd_dev_crc32_update(htd_crc32_ctx_t* p_ctx, const uint8_t* p_data, uint32_t data_len)
{
    if (p_ctx->crc32_type == HTD_CRC32_GEN) {
        p_ctx->result = drv_crc32_update(p_data, data_len);
    } else {
        p_ctx->result = drv_crc32_mpeg2_update(p_data, data_len);
    }

    // 后续只能采用软件进行计算了
    if (data_len & 0x03) {
        p_ctx->user_hw_flag = 0;
        __HAL_RCC_CRC_CLK_DISABLE();
        LL_CRC_Write_IDR(CRC, 0);
    }
    return p_ctx->result;
}

// @brief 结束
int32_t htd_dev_crc32_final(htd_crc32_ctx_t* p_ctx)
{
    LL_CRC_ResetCRCCalculationUnit(CRC);
    LL_CRC_Write_IDR(CRC, 0);
    __HAL_RCC_CRC_CLK_DISABLE();
    return 0;
}

#elif defined(SOC_SERIES_STM32F0) || defined(SOC_SERIES_STM32F3) || defined(SOC_SERIES_STM32F7)
//////////////////////////////////////////////////////////////////////
// @brief 开始 
int32_t htd_dev_crc32_init(htd_crc32_ctx_t* p_ctx)
{
    ASSERT(p_ctx);
    
    // 判断空闲
    if (LL_CRC_Read_IDR(CRC) == 0x55) {
        return -ERROR_CODE_BUSY;
    }
    LL_CRC_Write_IDR(CRC, 0x55);

    // 使能时钟
    __HAL_RCC_CRC_CLK_ENABLE();
    // 设置数据反转模式
    if (p_ctx->crc32_type == HTD_CRC32_GEN) {
        LL_CRC_SetInputDataReverseMode(CRC, LL_CRC_INDATA_REVERSE_BYTE);
        LL_CRC_SetOutputDataReverseMode(CRC, LL_CRC_OUTDATA_REVERSE_BIT);
    } else {
        LL_CRC_SetInputDataReverseMode(CRC, LL_CRC_INDATA_REVERSE_NONE);
        LL_CRC_SetOutputDataReverseMode(CRC, LL_CRC_OUTDATA_REVERSE_NONE);
    }
    // 设置多项式系数和位宽
#if defined(CRC_POL_POL)
    LL_CRC_SetPolynomialCoef(CRC, LL_CRC_DEFAULT_CRC32_POLY);
    LL_CRC_SetPolynomialSize(CRC, LL_CRC_POLYLENGTH_32B);
#endif
    // 设置初始化值
    LL_CRC_SetInitialData(CRC, p_ctx->result);

    // 复位CRC计算单元
    LL_CRC_ResetCRCCalculationUnit(CRC);
    return ERROR_CODE_SUCCESS;
}

// @brief 更新
uint32_t htd_dev_crc32_update(htd_crc32_ctx_t* p_ctx, const uint8_t* p_data, uint32_t data_len)
{
    while (data_len--) {
        LL_CRC_FeedData8(CRC, *p_data++);
    }
    p_ctx->result = LL_CRC_ReadData32(CRC);
    
    if (p_ctx->crc32_type == HTD_CRC32_GEN) {
        p_ctx->result = ~p_ctx->result;
    } 
    return p_ctx->result;
}

// @brief 结束
int32_t htd_dev_crc32_final(htd_crc32_ctx_t* p_ctx)
{
    LL_CRC_ResetCRCCalculationUnit(CRC);
    LL_CRC_Write_IDR(CRC, 0);
    __HAL_RCC_CRC_CLK_DISABLE();
    return 0;
}


// @brief 开始
int32_t htd_dev_crc16_init(htd_crc16_ctx_t* p_ctx)
{
    ASSERT(p_ctx);
    // 判断空闲
    if (LL_CRC_Read_IDR(CRC) == 0x55) {
        return -ERROR_CODE_BUSY;
    }
    LL_CRC_Write_IDR(CRC, 0x55);

    // 使能时钟
    __HAL_RCC_CRC_CLK_ENABLE();

    // 设置数据反转模式
    uint32_t PolynomCoef = 0x1021;
    if (p_ctx->crc16_type == HTD_CRC16_CCITT || p_ctx->crc16_type == HTD_CRC16_X25) {
        LL_CRC_SetInputDataReverseMode(CRC, LL_CRC_INDATA_REVERSE_BYTE);
        LL_CRC_SetOutputDataReverseMode(CRC, LL_CRC_OUTDATA_REVERSE_BIT);
        
    } else if (p_ctx->crc16_type == HTD_CRC16_MODBUS) {
        PolynomCoef = 0x8005;
        LL_CRC_SetInputDataReverseMode(CRC, LL_CRC_INDATA_REVERSE_BYTE);
        LL_CRC_SetOutputDataReverseMode(CRC, LL_CRC_OUTDATA_REVERSE_BIT);
        
    } else {
        // HTD_CRC16_XMODEM, HTD_CRC16_CCITT_FALSE
        LL_CRC_SetInputDataReverseMode(CRC, LL_CRC_INDATA_REVERSE_NONE);
        LL_CRC_SetOutputDataReverseMode(CRC, LL_CRC_OUTDATA_REVERSE_NONE);
    }

    // 设置多项式系数和位宽
#if defined(CRC_POL_POL)
    LL_CRC_SetPolynomialCoef(CRC, PolynomCoef);
    LL_CRC_SetPolynomialSize(CRC, LL_CRC_POLYLENGTH_16B);
#endif
    
    // 设置初始化值
    LL_CRC_SetInitialData(CRC, p_ctx->result);
    // 复位CRC计算单元
    LL_CRC_ResetCRCCalculationUnit(CRC);
    return ERROR_CODE_SUCCESS;
}


// @brief 计算
uint16_t htd_dev_crc16_update(htd_crc16_ctx_t* p_ctx, const uint8_t* p_data, uint32_t data_len)
{
    while (data_len--) {
        LL_CRC_FeedData8(CRC, *p_data++);
    }
    p_ctx->result = LL_CRC_ReadData16(CRC);
    
    if (p_ctx->crc16_type == HTD_CRC16_X25) {
        p_ctx->result = ~p_ctx->result;
    } 
    return p_ctx->result;
}

// @brief 结束
uint16_t htd_dev_crc16_final(htd_crc16_ctx_t* p_ctx)
{
    LL_CRC_ResetCRCCalculationUnit(CRC);
    LL_CRC_Write_IDR(CRC, 0);
    __HAL_RCC_CRC_CLK_DISABLE();
    return 0;
}


// @brief 开始
int32_t htd_dev_crc8_init(htd_crc8_ctx_t* p_ctx)
{
    ASSERT(p_ctx);
    // 判断空闲
    if (LL_CRC_Read_IDR(CRC) == 0x55) {
        return -ERROR_CODE_BUSY;
    }
    LL_CRC_Write_IDR(CRC, 0x55);

    // 使能时钟
    __HAL_RCC_CRC_CLK_ENABLE();

    // 设置数据反转模式
    uint32_t PolynomCoef = 0x07;
    if (p_ctx->crc8_type == HTD_CRC8_ROHC) {
        LL_CRC_SetInputDataReverseMode(CRC, LL_CRC_INDATA_REVERSE_BYTE);
        LL_CRC_SetOutputDataReverseMode(CRC, LL_CRC_OUTDATA_REVERSE_BIT);
        
    } else if (p_ctx->crc8_type == HTD_CRC8_MAXIM) {
        PolynomCoef = 0x31;
        LL_CRC_SetInputDataReverseMode(CRC, LL_CRC_INDATA_REVERSE_BYTE);
        LL_CRC_SetOutputDataReverseMode(CRC, LL_CRC_OUTDATA_REVERSE_BIT);
        
    } else {
        // HTD_CRC8_ITU, HTD_CRC8_DEF
        LL_CRC_SetInputDataReverseMode(CRC, LL_CRC_INDATA_REVERSE_NONE);
        LL_CRC_SetOutputDataReverseMode(CRC, LL_CRC_OUTDATA_REVERSE_NONE);
    }

    // 设置多项式系数和位宽
#if defined(CRC_POL_POL)
    LL_CRC_SetPolynomialCoef(CRC, PolynomCoef);
    LL_CRC_SetPolynomialSize(CRC, LL_CRC_POLYLENGTH_8B);
#endif
    
    // 设置初始化值
    LL_CRC_SetInitialData(CRC, p_ctx->result);
    // 复位CRC计算单元
    LL_CRC_ResetCRCCalculationUnit(CRC);
    return ERROR_CODE_SUCCESS;
}


// @brief 计算
uint16_t htd_dev_crc8_update(htd_crc8_ctx_t* p_ctx, const uint8_t* p_data, uint32_t data_len)
{
    while (data_len--) {
        LL_CRC_FeedData8(CRC, *p_data++);
    }
    p_ctx->result = LL_CRC_ReadData16(CRC);
    
    if (p_ctx->crc8_type == HTD_CRC8_ITU) {
        p_ctx->result = p_ctx->result ^ 0x55;
    } 
    return p_ctx->result;
}

// @brief 结束
int32_t htd_dev_crc8_final(htd_crc8_ctx_t* p_ctx)
{
    LL_CRC_ResetCRCCalculationUnit(CRC);
    LL_CRC_Write_IDR(CRC, 0);
    __HAL_RCC_CRC_CLK_DISABLE();
    return 0;
}



#else
#error "Please select chip."
#endif
#endif /*defined(BSP_USING_CRC) && defined(CRC)*/


