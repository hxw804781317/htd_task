#define DEBUG_LVL           1       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG           "dev.occ"
#include "htd_dev_timer_occ.h"
#include "htd_device.h"
#include "htd_components.h"


//////////////////////////////////////////////////////////////////////
// @brief 释放内存句柄
static void occ_free_pluse_array_handle(void* p_ptr)
{
    if (p_ptr) {
        htd_free(p_ptr);
        print_debug("occ finish.");
    }
}

// @brief 停止通告句柄
static void htd_dev_occ_stop_notify_handle(void* handle)
{
    htd_occ_handle_t* p_handle = (htd_occ_handle_t*)handle;
    if (p_handle == NULL) {
        return;
    }

    uint8_t* p_ptr = (uint8_t*)p_handle->p_pulse_array;
    if (p_ptr) {
        p_handle->p_pulse_array = NULL;
        if (htd_task_load_function(occ_free_pluse_array_handle, p_ptr)) {
            ASSERT(0);
        }
    }
}

// @brief 打开 OCC(多脉冲生成器)
htd_occ_handle_t* htd_dev_occ_open(const char* p_name, const htd_occ_user_config_t *p_user_config)
{
    ASSERT(p_name);
    ASSERT(p_user_config);

    const htd_dev_t* p_dev = htd_dev_find(p_name, HTD_OCC_DEV_IDENTIFY);
    if (p_dev == NULL) {
        print_error("occ(%s) open failed.", p_name);
        return NULL;
    }
    
    // 创建新的设备
    htd_occ_handle_t* p_handle = (htd_occ_handle_t*)htd_malloc(sizeof(htd_occ_handle_t));
    ASSERT(p_handle);
    htd_memset(p_handle, 0, sizeof(htd_occ_handle_t));
    p_handle->p_dev = p_dev;
    p_handle->mode = p_user_config->mode;
    p_handle->pin_num = p_user_config->pin_num;
    p_handle->prescaler = p_user_config->prescaler;
    p_handle->occ_stop_notify_handle = &htd_dev_occ_stop_notify_handle;
    int32_t err_code = p_dev->ctrl_handle(p_handle, HTD_OCC_CMD_OPEN, NULL, 0);
    if (err_code != ERROR_CODE_SUCCESS) {
        htd_free(p_handle);
        print_error("occ open failed. %s, pin_num=%d", p_name, p_user_config->pin_num);
        return NULL;
    }
    return p_handle;
}

// @brief 关闭 OCC(多脉冲生成器)
int32_t htd_dev_occ_close(htd_occ_handle_t* p_handle)
{
    ASSERT(p_handle);
    int32_t err_code = p_handle->p_dev->ctrl_handle(p_handle, HTD_OCC_CMD_CLOSE, NULL, 0);
    // 引脚设置为高阻态(保持引脚处于低电平状态)
    if (p_handle->pin_num != PIN_NUM_INVALID) {
        htd_pin_input_analog_set(p_handle->pin_num);
    }
    if (p_handle->p_pulse_array) {
        htd_free(p_handle->p_pulse_array);
    }
    htd_free(p_handle);
    return err_code;
}

// @brief 获取频率
uint32_t htd_dev_occ_freq_hz_get(htd_occ_handle_t* p_handle)
{
    ASSERT(p_handle);
    return p_handle->freq_hz;
}

// @brief 设置脉冲数组, 配置之后需要调用 htd_dev_occ_start 启动，调用该函数时，如果设备正在运行，会自动停止
// @param pluse_data_size p_pluse_array 数据位宽，单位字节(1/2)
// @param p_pulse_array 脉冲数组, 单位为 tick
// @param pluse_array_count 脉冲数组大小
int32_t htd_dev_occ_pluse_set(htd_occ_handle_t* p_handle, uint8_t pluse_data_size, uint8_t* p_pluse_array, uint32_t pluse_array_count)
{
    ASSERT(p_handle);
    ASSERT(pluse_data_size == 1 || pluse_data_size == 2);
    ASSERT(p_pluse_array);
    ASSERT(pluse_array_count);

    p_handle->p_dev->ctrl_handle(p_handle, HTD_OCC_CMD_STOP, NULL, 0);
    if (p_handle->p_pulse_array) {
        htd_free(p_handle->p_pulse_array);
    }
    
    p_handle->p_pulse_array = (uint16_t*)htd_malloc(sizeof(uint16_t) * pluse_array_count);
    ASSERT(p_handle->p_pulse_array);
    p_handle->pulse_array_offset = 0;
    p_handle->pulse_array_size = pluse_array_count;
    if (pluse_data_size == 2) {
        htd_memcpy(p_handle->p_pulse_array, p_pluse_array, pluse_array_count * pluse_data_size);
    } else {
        while (pluse_array_count--) {
            p_handle->p_pulse_array[p_handle->pulse_array_offset++] = *p_pluse_array++;
        }
        p_handle->pulse_array_offset = 0;
    }
    return 0;
}

// @brief 启动 OCC(多脉冲生成器)
int32_t htd_dev_occ_start(htd_occ_handle_t* p_handle)
{
    ASSERT(p_handle);
    ASSERT(p_handle->p_pulse_array);
    ASSERT(p_handle->pulse_array_size > 0);
    return p_handle->p_dev->ctrl_handle(p_handle, HTD_OCC_CMD_START, NULL, 0);
}

// @brief 停止 OCC(多脉冲生成器)
int32_t htd_dev_occ_stop(htd_occ_handle_t* p_handle)
{
    ASSERT(p_handle);
    return p_handle->p_dev->ctrl_handle(p_handle, HTD_OCC_CMD_STOP, NULL, 0);
}

// @brief 判断 OCC(多脉冲生成器) 是否在运行
int32_t htd_dev_occ_is_active(htd_occ_handle_t* p_handle)
{
    ASSERT(p_handle);
    return (p_handle->state == HTD_TIMER_STATE_START);
}




//////////////////////////////////////////////////////////////////////
#if (defined(HTD_CONSOLE_ENABLE) && defined(BSP_USING_OCC) && (DEBUG_LVL == 4))
static htd_occ_handle_t* m_occ_handle = NULL;

// @brief 启动
static void htd_tim_occ_start_handle(void)
{
    if (m_occ_handle == NULL) {
        return;
    }

    uint32_t freq_hz = htd_dev_occ_freq_hz_get(m_occ_handle);
    uint32_t pluse_array_count = 8;
    uint16_t* p_pluse_array = htd_malloc(sizeof(uint16_t) * pluse_array_count);
    ASSERT(p_pluse_array);

    freq_hz /= 1000 * 1000;        // 转 1us
    for (uint32_t i=0; i<pluse_array_count; ++i) {
        p_pluse_array[i] = (3 + i) * freq_hz;
        print_debug("freq: %d, %u(us)", p_pluse_array[i], p_pluse_array[i]/freq_hz);
    }
    htd_dev_occ_pluse_set(m_occ_handle, 2, (uint8_t*)p_pluse_array, pluse_array_count);
    htd_free(p_pluse_array), p_pluse_array = NULL;

    // 启动
    htd_dev_occ_start(m_occ_handle);
}

// @brief 调试命令
static int occ_cmd(int argc, char **argv)
{
    if (argc < 2) {
        print_error("Usage: %s open/close", argv[0]);
        return 0;
    }

    // 打开
    if (0 == memcmp(argv[1], "open", 5)) {
        if (argc != 3) {
            print_error("Usage: %s open [occ1/occ2...]", argv[0]);
            return 0;
        }
        htd_occ_user_config_t occ_config;
        occ_config.pin_num = PIN_NUM_GET(B, 0);
        occ_config.mode = HTD_OCC_MODE_HL;
        occ_config.prescaler = 0;
        m_occ_handle = htd_dev_occ_open(argv[2], &occ_config);
        return 0;
    } 
    // 关闭
    if (0 == memcmp(argv[1], "close", 6)) {
        if (m_occ_handle) {
            htd_dev_occ_close(m_occ_handle);
            m_occ_handle = NULL;
        }
        return 0;
    } 
    // 启动
    if (0 == memcmp(argv[1], "start", 6)) {
        htd_tim_occ_start_handle();
        return 0;
    }
    // 停止
    if (0 == memcmp(argv[1], "stop", 5)) {
        if (m_occ_handle) {
            htd_dev_occ_stop(m_occ_handle);
        }
        return 0;
    }

    print_error("Usage: %s open/close/list", argv[0]);
    return 0;
}
CONSOLE_CMD_EXPORT(occ_cmd, occ);
#endif /*(defined(HTD_CONSOLE_ENABLE) && (DEBUG_LVL > 0))*/

