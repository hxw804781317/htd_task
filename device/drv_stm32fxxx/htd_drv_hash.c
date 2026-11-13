// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL   4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG   "drv.hash"
#include "htd_device.h"
#include "htd_drv_common.h"



#if defined(BSP_USING_HASH) && defined(HASH) && 1
//////////////////////////////////////////////////////////////////////
static volatile uintptr_t m_hash_busy_flag = 0;
static HASH_HandleTypeDef *m_hash_handle = NULL;


// @brief 错误
void HAL_HASH_ErrorCallback(HASH_HandleTypeDef *hhash)
{
    print_error("error. ErrorCode=%d", hhash->ErrorCode);
}


// @brief 初始化
int32_t htd_dev_sha256_init(htd_sha256_ctx_t *p_ctx)
{
    if (m_hash_busy_flag != 0) {
        return -ERROR_CODE_BUSY;
    }
    m_hash_busy_flag = 0xa54f8899;

    ASSERT(m_hash_handle == NULL);
    m_hash_handle = (HASH_HandleTypeDef*)htd_malloc(sizeof(HASH_HandleTypeDef));
    ASSERT(m_hash_handle);
    htd_memset(m_hash_handle, 0, sizeof(HASH_HandleTypeDef));

    __HAL_RCC_HASH_CLK_ENABLE();
    HAL_HASH_Init(m_hash_handle);
    HAL_HASHEx_SHA256_Start(m_hash_handle, NULL, 0, NULL, 100);
    return ERROR_CODE_SUCCESS;
}
// @brief 更新 sha256
void htd_dev_sha256_update(htd_sha256_ctx_t *p_ctx, const uint8_t* p_data, uint32_t data_len)
{
    ASSERT(m_hash_handle);
    HAL_HASHEx_SHA256_Accmlt(m_hash_handle, p_data, data_len);
}
// @brief 获取 sha256 结果
void htd_dev_sha256_final(htd_sha256_ctx_t *p_ctx, uint8_t output[32])
{
    ASSERT(m_hash_handle);
    HAL_HASHEx_SHA256_Finish(m_hash_handle, output, 1000);
    HAL_HASH_DeInit(m_hash_handle);
    __HAL_RCC_HASH_CLK_DISABLE();

    htd_free(m_hash_handle);
    m_hash_handle = NULL;
    m_hash_busy_flag = 0;
}


//////////////////////////////////////////////////////////////////////
// @brief 初始化
int32_t htd_dev_sha1_init(htd_sha1_ctx_t *p_ctx)
{
    return 0;
}
// @brief 更新 sha1 
void htd_dev_sha1_update(htd_sha1_ctx_t *p_ctx, const uint8_t* p_data, uint32_t data_len)
{

}
// @brief 获取 sha1 结果
void htd_dev_sha1_final(htd_sha1_ctx_t *p_ctx, uint8_t output[20])
{

}


//////////////////////////////////////////////////////////////////////
// @brief 初始化
int32_t htd_dev_md5_init(htd_md5_ctx_t *p_ctx)
{
    return 0;
}
// @brief 更新 md5
void htd_dev_md5_update(htd_md5_ctx_t *p_ctx, const uint8_t* p_data, uint32_t data_len)
{
    
}
// @brief 获取 md5 结果
void htd_dev_md5_final(htd_md5_ctx_t *p_ctx, uint8_t output[16])
{

}
#endif /*defined(BSP_USING_HASH)*/

