// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL   4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG   "drv.wdt"
#include "htd_device.h"
#include "htd_drv_common.h"



#if defined(BSP_USING_WDT) && defined(IWDG) && 0
//////////////////////////////////////////////////////////////////////
#if 0
    IWDG 独立看门狗，使用内部时钟LSI，40000HZ；计数器递减至0时触发复位
    WWDG 窗口看门狗，使用PCLK1时钟，默认36*1000*1000HZ；计数器递减超出窗口则复位
    WWDG在计数器降至0x40时支持提前唤醒中断，可用于紧急数据备份；IWDG无此功能
    IWDG适合监控系统级异常（如死循环、硬件故障），WWDG更适合软件时效性监测（如防止程序长时间阻塞）
#endif

static IWDG_HandleTypeDef m_hiwdg = {0};

// @brief 启动看门狗
int32_t drv_wdt_init(uint32_t timeout_ms)
{
    ASSERT(LSI_VALUE > 0);
    m_hiwdg.Init.Reload = timeout_ms * LSI_VALUE / 256;
    int32_t ret = HAL_IWDG_Init(&m_hiwdg);
    if (ret != HAL_OK) {
        print_error("wdt start failed. ret=%d", ret);
        return ret;
    }
    return 0;
}

// @brief 
int32_t drv_wdt_deinit(void)
{
    return 0;
}

// @brief 喂狗
void drv_wdt_feed(void)
{
    if (HAL_IWDG_Refresh(&m_hiwdg) != HAL_OK) {
        print_error("watch dog feed fail.");
    }
}

// @brief 初始化 iwdg 
int drv_iwdg_init(void)
{
    memset(&m_hiwdg, 0, sizeof(IWDG_HandleTypeDef));
    m_hiwdg.Instance = IWDG;
    m_hiwdg.Init.Prescaler = IWDG_PRESCALER_256;
    m_hiwdg.Init.Reload = 0x00000FFF;
    return 0;
}
DEVICE_INIT_FUNC_REGISTER(drv_iwdg_init);

#endif /**/

