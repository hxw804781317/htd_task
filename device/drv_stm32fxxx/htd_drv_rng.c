#define DEBUG_LVL   4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG   "drv.rng"
#include "htd_drv_common.h"
#include "htd_device.h"


#if defined(BSP_USING_RNG) && defined(RNG) && 1
//////////////////////////////////////////////////////////////////////
// @brief 错误回调
void HAL_RNG_ErrorCallback(RNG_HandleTypeDef *hrng)
{
    print_error("error: %d", HAL_RNG_GetError(hrng));
}

// @brief 获取随机种子
uint32_t htd_rng_seed_get(void)
{
    static uint8_t is_busy = 0;
    while (is_busy) {
        htd_delay_us(10);
    }
    is_busy = 0xa5;

    int32_t ret;
    RNG_HandleTypeDef hrng;
    htd_memset(&hrng, 0, sizeof(RNG_HandleTypeDef));
    hrng.Instance = RNG;
    //hrng.Init.ClockErrorDetection = RNG_CED_ENABLE;
    __HAL_RCC_RNG_CLK_ENABLE();
    ret = HAL_RNG_Init(&hrng);
    if (ret != HAL_OK) {
        print_error("rng init failed. ret=%d, ErrorCode=%d, state=%d", ret, hrng.ErrorCode, hrng.State);
        ASSERT(0);
    }

    uint32_t i;
    uint32_t random_number = 0;
    for (i=0; i<100; ++i) {
        random_number = 0;
        if (HAL_RNG_GenerateRandomNumber(&hrng, &random_number) == HAL_OK) {
            break;
        }
        htd_delay_us(10);
    }
    HAL_RNG_DeInit(&hrng);
    __HAL_RCC_RNG_CLK_DISABLE();
    
    is_busy = 0;
    print_debug("rng seed: 0x%08x", random_number);
    return random_number;
}

#endif /*defined(BSP_USING_RNG)*/

