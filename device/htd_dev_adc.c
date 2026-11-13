//#define DEBUG_LVL           4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG           "dev.adc"
#include "htd_device.h"
#include "htd_dev_adc.h"



//////////////////////////////////////////////////////////////////////
// @brief 打开ADC操作句柄
htd_adc_handle_t* htd_adc_open(const char* p_name, const htd_adc_user_config_t* p_user_config)
{
    const htd_dev_t* p_dev = htd_dev_find(p_name, HTD_ADC_DEV_IDENTIFY);
    if (p_dev == NULL) {
        return NULL;
    }
    ASSERT(p_dev->ctrl_handle);

    int32_t ret = 0;
    htd_adc_handle_t* p_handle = (htd_adc_handle_t*)htd_malloc(sizeof(htd_adc_handle_t));
    ASSERT(p_handle);
    htd_memset(p_handle, 0, sizeof(htd_adc_handle_t));
    p_handle->p_dev = p_dev;
    ret = p_dev->ctrl_handle(p_handle, HTD_ADC_CMD_OPEN, (void*)p_user_config, sizeof(htd_adc_user_config_t));
    if (ret != ERROR_CODE_SUCCESS) {
        print_error("adc open failed. p_name=%s", p_name);
        htd_free(p_handle);
        return NULL;
    }
    return p_handle;
}

// @brief 控制
int32_t htd_adc_ctrl(htd_adc_handle_t* p_handle, htd_adc_cmd_t cmd, void* p_param, uint32_t param_len)
{
    int32_t ret = ERROR_CODE_INVALID_STATE;
    if (p_handle && p_handle->p_dev) {
        ret = p_handle->p_dev->ctrl_handle(p_handle, cmd, p_param, param_len);
    }
    return ret;
}

// @brief 关闭
int32_t htd_adc_close(htd_adc_handle_t* p_handle)
{
    if (p_handle) {
        p_handle->p_dev->ctrl_handle(p_handle, HTD_ADC_CMD_CLOSE, NULL, 0);
        htd_free(p_handle);
    }
    return 0;
}

// @brief 使能/禁用ADC
int32_t htd_adc_enable_set(htd_adc_handle_t* p_handle, bool is_enable)
{
    ASSERT(p_handle);
    htd_adc_cmd_t cmd = is_enable ? HTD_ADC_CMD_ENABLE : HTD_ADC_CMD_DISABLE;
    return p_handle->p_dev->ctrl_handle(p_handle, cmd, NULL, 0);
}

// @brief 使能/禁用模拟看门狗功能
int32_t htd_adc_awd_enable_set(htd_adc_handle_t* p_handle, bool is_enable, htd_adc_awd_param_t* p_param)
{
    ASSERT(p_handle);
    htd_adc_cmd_t cmd = is_enable ? HTD_ADC_CMD_AWD_ENABLE : HTD_ADC_CMD_AWD_DISABLE;
    return p_handle->p_dev->ctrl_handle(p_handle, cmd, p_param, sizeof(htd_adc_awd_param_t));
}

// @brief 快速获取ADC值
// @note  调用该接口前需要先使能ADC
int32_t htd_adc_value_fast_get(htd_adc_handle_t* p_handle, uint16_t* p_adc_val, uint32_t count)
{
    ASSERT(p_handle);
    ASSERT(p_adc_val);
    ASSERT(count);
    return p_handle->p_dev->ctrl_handle(p_handle, HTD_ADC_CMD_FAST_VAL_GET, p_adc_val, sizeof(uint16_t) * count);
}

// @brief 获取ADC和对应的电压值
// @note  调用该接口前需要先使能ADC
int32_t htd_adc_voltage_get(htd_adc_handle_t* p_handle, htd_adc_voltage_t* p_val, uint32_t count)
{
    ASSERT(p_handle);
    ASSERT(p_val);
    ASSERT(count);
    return p_handle->p_dev->ctrl_handle(p_handle, HTD_ADC_CMD_VALTAGE_GET, p_val, sizeof(htd_adc_voltage_t) * count);
}



//////////////////////////////////////////////////////////////////////
#if (defined(HTD_CONSOLE_ENABLE) && (DEBUG_LVL > 0) && 1)
#include "htd_console_process.h"

static htd_adc_handle_t* m_adc_test_handle = NULL;

// @brief 调试命令
static int adc_base_cmd(int argc, char **argv)
{
    if (argc < 2) {
        print_error("Usage: %s open/close/read", argv[0]);
        return 0;
    }

    htd_adc_user_config_t m_adc_config = {
        .pin_num = PIN_NUM_GET(A, 5),
        .resolution = HTD_ADC_RESOLUTION_12BITS,
        .oversample = HTD_ADC_OVERSAMPLE_64X,
        .vref_onoff = true,
    };

    if (htd_memcmp(argv[1], "open", 5) == 0) {
        if (argc != 3) {
            print_error("Usage: %s open [dev]", argv[0]);
            return -1;
        }
        ASSERT(m_adc_test_handle == NULL);
        m_adc_test_handle = htd_adc_open(argv[2], &m_adc_config);
        if (m_adc_test_handle == NULL) {
            print_error("adc open failed.");
            return 0;
        }
        print_warning("adc open success.");
        
    } else if (htd_memcmp(argv[1], "close", 6) == 0) {
        if (m_adc_test_handle) {
            htd_adc_close(m_adc_test_handle);
            m_adc_test_handle = NULL;
        }
    } else if (htd_memcmp(argv[1], "read", 5) == 0) {
        if (NULL == m_adc_test_handle) {
            return 0;
        }

        htd_adc_enable_set(m_adc_test_handle, true);
        htd_adc_voltage_t adc_voltage;
        int32_t ret = htd_adc_voltage_get(m_adc_test_handle, &adc_voltage, 1);
        print_debug("ret=%d, adc_val=%u, voltage_mv=%u", ret, adc_voltage.adc_val, adc_voltage.voltage_mv);
        htd_adc_enable_set(m_adc_test_handle, false);
        
    } else {
        print_error("Usage: %s open/close", argv[0]);
    }
    return 0;
}
CONSOLE_CMD_EXPORT(adc_base_cmd, adc);
#endif /*(defined(HTD_CONSOLE_ENABLE) && (DEBUG_LVL > 0))*/

