#define DEBUG_LVL   4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG   "drv.adc"
#include "htd_drv_common.h"
#include "htd_device.h"



#if defined(BSP_USING_ADC) && 0
//////////////////////////////////////////////////////////////////////
enum {
    #ifdef BSP_USING_ADC1
        ADC1_INDEX,
    #endif
    #ifdef BSP_USING_ADC2
        ADC2_INDEX,
    #endif
    #ifdef BSP_USING_ADC3
        ADC3_INDEX,
    #endif
        ADC_ENABLE_COUNT,
};

static ADC_HandleTypeDef m_adc_objects[ADC_ENABLE_COUNT] = {
    #ifdef BSP_USING_ADC1
    {
       .Instance = ADC1,
       .Init.DataAlign = ADC_DATAALIGN_RIGHT,
       .Init.ScanConvMode = ADC_SCAN_DISABLE, 
       .Init.ContinuousConvMode = DISABLE,
       .Init.NbrOfConversion = 1,
       .Init.DiscontinuousConvMode = DISABLE,
       .Init.NbrOfDiscConversion = 1, 
       .Init.ExternalTrigConv = ADC_SOFTWARE_START,
    },
    #endif /*BSP_USING_ADC1*/
    #ifdef BSP_USING_ADC2
    { 
       .Instance = ADC2,
       .Init.DataAlign = ADC_DATAALIGN_RIGHT,
       .Init.ScanConvMode = ADC_SCAN_DISABLE,
       .Init.ContinuousConvMode = DISABLE,
       .Init.NbrOfConversion = 1,W
       .Init.DiscontinuousConvMode = DISABLE,
       .Init.NbrOfDiscConversion = 1,
       .Init.ExternalTrigConv = ADC_SOFTWARE_START,
    },
    #endif /*BSP_USING_ADC2*/
    #ifdef BSP_USING_ADC3
    {
       .Instance = ADC3,
       .Init.DataAlign = ADC_DATAALIGN_RIGHT,
       .Init.ScanConvMode = ADC_SCAN_DISABLE,
       .Init.ContinuousConvMode = DISABLE,
       .Init.NbrOfConversion = 1,
       .Init.DiscontinuousConvMode = DISABLE,
       .Init.NbrOfDiscConversion = 1,
       .Init.ExternalTrigConv = ADC_SOFTWARE_START,
    },
    #endif /*BSP_USING_ADC3*/
};

// @brief 获取通道
static int32_t stm32_adc_channel_get(adc_channel_t ch, uint32_t *stm32_channel)
{
    uint32_t channel_val = ADC_CHANNEL_0;
    const uint32_t ch_table[] = {
        ADC_CHANNEL_0,  ADC_CHANNEL_1,  ADC_CHANNEL_2,  ADC_CHANNEL_3, 
        ADC_CHANNEL_4,  ADC_CHANNEL_5,  ADC_CHANNEL_6,  ADC_CHANNEL_7,
        ADC_CHANNEL_8,  ADC_CHANNEL_9,  ADC_CHANNEL_10, ADC_CHANNEL_11,
        ADC_CHANNEL_12, ADC_CHANNEL_13, ADC_CHANNEL_14, ADC_CHANNEL_15,
    };
    if (ch < ARRAY_COUNT(ch_table)) {
        channel_val = ch_table[ch];
    } else {
        switch (ch) {
        #ifdef ADC_CHANNEL_16
            case ADC_CH_16: {
                channel_val = ADC_CHANNEL_16;
                break;
            }
        #endif /* ADC_CHANNEL_16 */
        case ADC_CH_17: {
            channel_val = ADC_CHANNEL_17;
            break;
        }
        #ifdef ADC_CHANNEL_18
            case ADC_CH_18: {
                channel_val = ADC_CHANNEL_18;
                break;
            }
        #endif /* ADC_CHANNEL_18 */
        #ifdef ADC_CHANNEL_19
            case ADC_CH_19: {
                channel_val = ADC_CHANNEL_19;
                break;
            }
        #endif /* ADC_CHANNEL_19 */
        #ifdef ADC_CHANNEL_VREFINT
            case ADC_CH_INTER_VREF: {
                channel_val = ADC_CHANNEL_VREFINT;
                break;
            }
        #endif /* ADC_CHANNEL_VREFINT */
        #ifdef ADC_CHANNEL_VBAT
            case ADC_CH_INTER_VBAT: {
                channel_val = ADC_CHANNEL_VBAT;
                break;
            }
        #endif /* ADC_CHANNEL_VBAT */
        #ifdef ADC_CHANNEL_TEMPSENSOR
            case ADC_CH_INTER_TEMPER: {
                channel_val = ADC_CHANNEL_TEMPSENSOR;
                break;
            }
        #endif 
            default: {
                return -1;
            }
        }
    }
    if (stm32_channel) {
        *stm32_channel = channel_val;
    }
    return 0;
}

// @brief 获取ADC值
static uint32_t drv_adc_value_get(ADC_HandleTypeDef* p_adc, uint32_t *p_val)
{
    ASSERT(p_val != NULL);
    HAL_ADC_PollForConversion(p_adc, 100);
    *p_val = (uint32_t)HAL_ADC_GetValue(p_adc);
    return 0;
}

// @brief 打开(类似使能, 对应的close类似disable)
static int32_t drv_adc_open(adc_handle_t* p_handle)
{
    ADC_ChannelConfTypeDef chan_config;
    ADC_HandleTypeDef* p_adc = (ADC_HandleTypeDef*)p_handle->p_dev->p_object;
    if (stm32_adc_channel_get(p_handle->channel, &chan_config.Channel) != 0) {
        print_error("ADC channel illegal: %d", p_handle->channel);
        return -1;
    }
    chan_config.Rank = 1;
    chan_config.SamplingTime = ADC_SAMPLETIME_55CYCLES_5;
    if (HAL_ADC_ConfigChannel(p_adc, &chan_config) != HAL_OK) {
        print_error("Failed to configure ADC channel %d", p_handle->channel);
        return -1;
    }
    if (HAL_OK != HAL_ADC_Start(p_adc)) {
        return -1;
    }
    return 0;
}

// @brief 关闭
static int32_t drv_adc_close(adc_handle_t* p_handle)
{
    ADC_HandleTypeDef* p_adc = (ADC_HandleTypeDef*)p_handle->p_dev->p_object;
    HAL_ADC_Stop(p_adc);
}

// @brief ADC 控制
static int32_t drv_adc_ctrl(adc_handle_t* p_handle, htd_adc_cmd_t cmd, void* p_param, uint32_t param_len)
{
    ASSERT(p_handle);
    switch (cmd) {
        case HTD_ADC_CMD_CH_VAL_GET: {
            ADC_HandleTypeDef* p_adc = (ADC_HandleTypeDef*)p_handle->p_dev->p_object;
            return drv_adc_value_get(p_adc, (uint32_t*)p_param);
        }
        case HTD_ADC_CMD_SAMPLE_BITS_GET: {
            return 12;
        }
        case HTD_ADC_CMD_VREF_VOLTAGE_GET: {
            return 3300;
        }
        default: {
            break;
        }
    }
    return 0;
}

/**
* @brief ADC MSP Initialization
* This function configures the hardware resources used in this example
* @param hadc: ADC handle pointer
* @retval None
*/
void HAL_ADC_MspInit(ADC_HandleTypeDef* hadc)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if (hadc->Instance == ADC1) {
        // 使用 ADC1 时钟
        __HAL_RCC_ADC1_CLK_ENABLE();
        // PA1->ADC1_IN1; PA4->ADC1_IN4; PA5->ADC1_IN5; PA6->ADC1_IN6; PA7->ADC1_IN7
        __HAL_RCC_GPIOA_CLK_ENABLE();
        GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
}

/**
* @brief ADC MSP De-Initialization
* This function freeze the hardware resources used in this example
* @param hadc: ADC handle pointer
* @retval None
*/
void HAL_ADC_MspDeInit(ADC_HandleTypeDef* hadc)
{
    if (hadc->Instance == ADC1) {
        // 禁用 ADC1 时钟
        __HAL_RCC_ADC1_CLK_DISABLE();
        // PA1->ADC1_IN1; PA4->ADC1_IN4; PA5->ADC1_IN5; PA6->ADC1_IN6; PA7->ADC1_IN7
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_1|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7);
    }
}

// @brief 初始化ADC
int drv_adc_init(void)
{
    for (uint32_t i=0; i<ARRAY_COUNT(m_adc_objects); ++i) {
        if (HAL_ADC_Init(&m_adc_objects[i]) != HAL_OK) {     // 会调用 HAL_ADC_MspInit
            print_error("ADC[%d] init failed", i);
            return -1;
        }
    }
    return 0;
}
DEVICE_INIT_FUNC_REGISTER(drv_adc_init);


#ifdef BSP_USING_ADC1
    HTD_ADC_DEV_REGISTER(adc1, drv_adc_open, drv_adc_close, drv_adc_ctrl, &m_adc_objects[ADC1_INDEX]);
#endif
#ifdef BSP_USING_ADC2
    HTD_ADC_DEV_REGISTER(adc2, drv_adc_open, drv_adc_close, drv_adc_ctrl, &m_adc_objects[ADC2_INDEX]);
#endif
#ifdef BSP_USING_ADC3
    HTD_ADC_DEV_REGISTER(adc3, drv_adc_open, drv_adc_close, drv_adc_ctrl, &m_adc_objects[ADC3_INDEX]);
#endif
#endif /**/


