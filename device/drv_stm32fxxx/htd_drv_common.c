#define DEBUG_LVL   4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG   "drv.common"
#include "htd_drv_common.h"
#include "htd_device.h"
#include "htd_heap_ff.h"


//////////////////////////////////////////////////////////////////////
// @brief 延迟us
void htd_delay_us(uint32_t us)
{
#if defined(SOC_SERIES_STM32F0)
    if (0 == us) {
        return;
    }
    extern uint32_t SystemCoreClock;
    uint32_t cycles = us * (SystemCoreClock / 1000000);

    // 定义机器码数组
    __ALIGN(16) 
    static const uint16_t delay_machine_code[] = {
        0x3800 + 4,                 // SUBS r0, #3  (0x1F03)
        0xD8FD,                     // BHI .-2
        0x4770                      // BX LR
    };
    typedef void (* delay_func_t)(uint32_t);
    // 转换函数指针 (Thumb 模式需 | 1)
    const delay_func_t delay_cycles = (delay_func_t)((((uint32_t)delay_machine_code) | 1));
    // 执行延时
    delay_cycles(cycles);
#else
    if (us == 0) {
        return ;
    }
    extern uint32_t SystemCoreClock;
    uint32_t time_cycles = us * (SystemCoreClock / 1000000);

    // 保存并启动 DWT
    uint32_t core_debug = CoreDebug->DEMCR;
    CoreDebug->DEMCR = core_debug | CoreDebug_DEMCR_TRCENA_Msk;
    // Save the current state of the CTRL register in the DWT block. Make sure
    // that the cycle counter is enabled.
    uint32_t dwt_ctrl = DWT->CTRL;
    DWT->CTRL = dwt_ctrl | DWT_CTRL_CYCCNTENA_Msk;
    // Store start value of the cycle counter.
    uint32_t cyccnt_initial = DWT->CYCCNT;
    // 等待延迟的时间
    while ((DWT->CYCCNT - cyccnt_initial) < time_cycles){}
    // 恢复原始状态
    DWT->CTRL = dwt_ctrl;
    CoreDebug->DEMCR = core_debug;
#endif
}

// @brief 延迟毫秒
void htd_delay_ms(uint32_t ms)
{
    while (ms > 5000) {
        htd_delay_us(5000*1000);
        ms -= 5000;
    }
    htd_delay_us(ms * 1000);
}




//////////////////////////////////////////////////////////////////////
#if defined(SOC_SERIES_STM32F7) 
static htd_heap_dev_t* m_drv_heap = NULL;

// @brief 申请内存
void* htd_drv_malloc(uint32_t size)
{
    void* p_ptr = NULL;
    if (m_drv_heap) {
        p_ptr = m_drv_heap->alloc_handle(m_drv_heap, size, htd_drv_malloc);
        if (p_ptr) {
            return p_ptr;
        }
    }
    return NULL;
}

// @brief 释放内存
void htd_drv_free(void* p_ptr)
{
    if (m_drv_heap) {
        m_drv_heap->free_handle(p_ptr);
    }
}

// @brief 初始化堆
int32_t htd_drv_heap_init(void)
{
    m_drv_heap = htd_heap_ff_init((void*)RAMDTCM_BASE, RAMDTCM_SIZE);
    ASSERT(m_drv_heap);
    return 0;
}
DEVICE_INIT_FUNC_REGISTER(htd_drv_heap_init);

#else
// @brief 申请内存
void* htd_drv_malloc(uint32_t size)
{
    return htd_malloc(size);
}
// @brief 释放内存
void htd_drv_free(void* p_ptr)
{
    htd_free(p_ptr);
}
#endif



//////////////////////////////////////////////////////////////////////
// @brief 获取时钟频率(单位HZ)
uint32_t htd_rcc_freq_hz_get(void)
{
    uint32_t clock_source = RCC->CFGR & RCC_CFGR_SWS;
    if (clock_source == RCC_CFGR_SWS_HSI) {
        return HSI_VALUE;
    }
    if (clock_source == RCC_CFGR_SWS_HSE) {
        return HSE_VALUE;
    }
#ifdef RCC_CFGR_SWS_PLL
    if (clock_source == RCC_CFGR_SWS_PLL) {
        return HAL_RCC_GetSysClockFreq(); // This function returns the PLL clock frequency
    }
#endif
#ifdef RCC_CFGR_SWS_PLL1
    if (clock_source == RCC_CFGR_SWS_PLL1) {
        return HAL_RCC_GetSysClockFreq(); // This function returns the PLL clock frequency
    }
#endif

    ASSERT(0);
    return 0;
}

// @brief 获取芯片id
uint32_t htd_mcu_id_get(uint8_t* p_id, uint32_t id_len)
{
    uint32_t val;
    uint32_t copy_len;
    uint8_t* p_start = p_id;
    if (id_len) {
        val = HAL_GetUIDw2();
        copy_len = sizeof(uint32_t);
        if (id_len < copy_len) {
            copy_len = id_len;
        }
        memcpy(p_id, &val, copy_len);
        p_id += copy_len;
        id_len -= copy_len;
    }
    if (id_len) {
        val = HAL_GetUIDw1();
        copy_len = sizeof(uint32_t);
        if (id_len < copy_len) {
            copy_len = id_len;
        }
        memcpy(p_id, &val, copy_len);
        p_id += copy_len;
        id_len -= copy_len;
    }
    if (id_len) {
        val = HAL_GetUIDw0();
        copy_len = sizeof(uint32_t);
        if (id_len < copy_len) {
            copy_len = id_len;
        }
        memcpy(p_id, &val, copy_len);
        p_id += copy_len;
        id_len -= copy_len;
    }
    return p_id - p_start;
}

// @brief 获取内存堆信息，返回总共多少个内存堆
// @param idx 第几块内存，partition_count 获取多少块内存信息
uint32_t htd_heap_partition_get(htd_ram_partition_t* p_info, uint32_t partition_count, uint32_t idx)
{
    htd_ram_partition_t* p_start = p_info;

#ifdef D1_AXISRAM_BASE
    // 第一块 AXI
    if (idx == 0) {
        if (partition_count) {
            p_info->addr = HTD_HEAP_BEGIN;
            p_info->size = D1_AXISRAM_BASE + D1_AXISRAM_SIZE - HTD_HEAP_BEGIN;
            partition_count--;
            p_info++;
        }
    } else {
        idx--;
    }
#endif
#ifdef D2_AHBSRAM_BASE
    // 第二块；SRAM1+SRAM2+SRAM3
    if (idx == 0) {
        if (partition_count) {
            p_info->addr = D2_AHBSRAM_BASE;
            p_info->size = D2_AXISRAM_SIZE;  
            partition_count--;
            p_info++;
        }
    } else {
        idx--;
    }
#endif
#ifdef SRAM_BASE
    // 通用内存，可以被DMA访问
    if (idx == 0) {
        if (partition_count) {
            p_info->addr = HTD_HEAP_BEGIN;
            p_info->size = SRAM_BASE + SRAM_SIZE- HTD_HEAP_BEGIN;
            partition_count--;
            p_info++;
        }
    } else {
        idx--;
    }
#endif
#ifdef RAMDTCM_BASE
    // 专用数据 DMA，访问速度最快，有些特殊的 DMA 无法访问
#endif
#ifdef RAMITCM_BASE
    // ITCM最适合存放那些一旦错过截止时间就会导致系统失效的代码, 比如中断的响应时间必须精确可控
#endif
    return p_info - p_start;
}

// @brief 显示设备信息
void htd_dev_info_show(void)
{
    
}


