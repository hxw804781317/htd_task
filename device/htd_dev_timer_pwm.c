#define DEBUG_LVL           1       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG           "dev.pwm"
#include "htd_dev_timer_pwm.h"
#include "htd_device.h"
#include "htd_components.h"


//////////////////////////////////////////////////////////////////////
// @brief 打开 PWM
htd_pwm_handle_t* htd_dev_pwm_open(const char* p_name, const htd_pwm_user_config_t *p_user_config)
{
    ASSERT(p_name);
    ASSERT(p_user_config);

    const htd_dev_t* p_dev = htd_dev_find(p_name, HTD_PWM_DEV_IDENTIFY);
    if (p_dev == NULL) {
        print_error("pwm(%s) open failed.", p_name);
        return NULL;
    }
    
    // 创建新的设备
    htd_pwm_handle_t* p_handle = (htd_pwm_handle_t*)htd_malloc(sizeof(htd_pwm_handle_t));
    ASSERT(p_handle);
    htd_memset(p_handle, 0, sizeof(htd_pwm_handle_t));
    p_handle->p_dev = p_dev;
    p_handle->mode = p_user_config->mode;
    p_handle->pwm_channel = 0xff;
    htd_memcpy(p_handle->pin_num, p_user_config->pin_num, sizeof(p_handle->pin_num));
    p_handle->prescaler = p_user_config->prescaler;
    int32_t err_code = p_dev->ctrl_handle(p_handle, HTD_PWM_CMD_OPEN, (void*)p_user_config, sizeof(htd_pwm_user_config_t));
    if (err_code != ERROR_CODE_SUCCESS) {
        htd_free(p_handle);
        print_error("pwm open failed. %s, pin_num=%d", p_name, p_user_config->pin_num);
        return NULL;
    }
    return p_handle;
}

// @brief 关闭 PWM
int32_t htd_dev_pwm_close(htd_pwm_handle_t* p_handle)
{
    ASSERT(p_handle);
    int32_t err_code = p_handle->p_dev->ctrl_handle(p_handle, HTD_PWM_CMD_CLOSE, NULL, 0);
    // 引脚设置为高阻态(保持引脚处于低电平状态)
    for (uint32_t i=0; i<ARRAY_COUNT(p_handle->pin_num); ++i) {
        if (p_handle->pin_num[i] != PIN_NUM_INVALID) {
            htd_pin_input_analog_set(p_handle->pin_num[i]);
        }
    }
    htd_free(p_handle);
    return err_code;
}

// @brief 获取频率
uint32_t htd_dev_pwm_freq_hz_get(htd_pwm_handle_t* p_handle)
{
    ASSERT(p_handle);
    return p_handle->freq_hz;
}

// @brief 设置周期
int32_t htd_dev_pwm_period_set(htd_pwm_handle_t* p_handle, uint16_t period)
{
    ASSERT(p_handle);
    return p_handle->p_dev->ctrl_handle(p_handle, HTD_PWM_CMD_PERIOD_SET, &period, sizeof(uint16_t));
}

// @brief 设置占空比和周期，单位为tick，不能超过一个周期(设置占空比前必须先设置周期)
// @param ch_idx通道序号，0xff 代表所有通道都启动, 当 open 配置了自适配引脚 pin_auto 这个参数无效
// @note  配置之后需要调用 htd_dev_pwm_start 启动，调用该函数时，如果设备正在运行，会自动停止
int32_t htd_dev_pwm_duty_set(htd_pwm_handle_t* p_handle, uint8_t ch_idx, uint16_t duty)
{
    ASSERT(p_handle);
    if (p_handle->pwm_channel < ARRAY_COUNT(p_handle->pin_num)) {
        ch_idx = p_handle->pwm_channel;
    }
    htd_pwm_duty_param_t param;
    param.ch_idx = ch_idx;
    param.duty = duty;
    return p_handle->p_dev->ctrl_handle(p_handle, HTD_PWM_CMD_DUTY_SET, &param, sizeof(htd_pwm_duty_param_t));
}

// @brief 设置占空比数组, 配置之后需要调用 htd_dev_pwm_start 启动，调用该函数时，如果设备正在运行，会自动停止
// @param ch_idx 通道号，当 open 配置了自适配引脚 pin_auto 这个参数无效
// @param p_duty_array 占空比数组，单位为 tick, 
// @param duty_array_count 占空比数组长度
// @param loop_count 循环次数，以数组内容执行N次
int32_t htd_dev_pwm_duty16_array_set(htd_pwm_handle_t* p_handle, uint8_t ch_idx, uint16_t* p_duty_array, uint32_t duty_array_count, uint32_t loop_count)
{
    ASSERT(p_handle);
    ASSERT(p_duty_array);
    ASSERT(duty_array_count);
    ASSERT(loop_count);

    if (p_handle->pwm_channel < ARRAY_COUNT(p_handle->pin_num)) {
        ch_idx = p_handle->pwm_channel;
    }
    p_handle->p_dev->ctrl_handle(p_handle, HTD_PWM_CMD_STOP, &ch_idx, sizeof(uint8_t));
    htd_pwm_duty_array_param_t param;
    param.ch_idx = ch_idx;
    param.duty_bytes = 2;
    param.p_duty_array = (uint8_t*)p_duty_array;
    param.duty_array_size = duty_array_count;
    param.loop_count = loop_count;
    return p_handle->p_dev->ctrl_handle(p_handle, HTD_PWM_CMD_DUTY_ARRAY_SET, &param, sizeof(htd_pwm_duty_array_param_t));
}

// @brief 设置占空比数组, 配置之后需要调用 htd_dev_pwm_start 启动，调用该函数时，如果设备正在运行，会自动停止
// @param ch_idx 通道号，当 open 配置了自适配引脚 pin_auto 这个参数无效
// @param p_duty_array 占空比数组，单位为 tick, 
// @param duty_array_count 占空比数组长度
// @param loop_count 循环次数，以数组内容执行N次
int32_t htd_dev_pwm_duty8_array_set(htd_pwm_handle_t* p_handle, uint8_t ch_idx, uint8_t* p_duty_array, uint32_t duty_array_count, uint32_t loop_count)
{
    ASSERT(p_handle);
    ASSERT(p_duty_array);
    ASSERT(duty_array_count);
    ASSERT(loop_count);

    if (p_handle->pwm_channel < ARRAY_COUNT(p_handle->pin_num)) {
        ch_idx = p_handle->pwm_channel;
    }
    p_handle->p_dev->ctrl_handle(p_handle, HTD_PWM_CMD_STOP, &ch_idx, sizeof(uint8_t));
    htd_pwm_duty_array_param_t param;
    param.ch_idx = ch_idx;
    param.duty_bytes = 1;
    param.p_duty_array = p_duty_array;
    param.duty_array_size = duty_array_count; 
    param.loop_count = loop_count;
    return p_handle->p_dev->ctrl_handle(p_handle, HTD_PWM_CMD_DUTY_ARRAY_SET, &param, sizeof(htd_pwm_duty_array_param_t));
}

// @brief 启动某个引脚的 PWM
// @param ch_idx_bit 通道序号bit位，0xff 代表所有通道都启动, 当 open 配置了自适配引脚 pin_auto 这个参数无效
int32_t htd_dev_pwm_start(htd_pwm_handle_t* p_handle, uint8_t ch_idx_bit)
{
    uint32_t ch_bits;
    ASSERT(p_handle);
    if (p_handle->pwm_channel < ARRAY_COUNT(p_handle->pin_num)) {
        ch_bits = __BIT(p_handle->pwm_channel);
    } else {
        ch_bits = (ch_idx_bit == 0xff) ? 0xffffffff : ch_idx_bit;
    }
    return p_handle->p_dev->ctrl_handle(p_handle, HTD_PWM_CMD_START, &ch_bits, sizeof(uint32_t));
}

// @brief 停止某个引脚的 PWM
// @param ch_idx通道序号，0xff 代表所有通道都启动, 当 open 配置了自适配引脚 pin_auto 这个参数无效
int32_t htd_dev_pwm_stop(htd_pwm_handle_t* p_handle, uint8_t ch_idx_bit)
{
    uint32_t ch_bits;
    ASSERT(p_handle);
    if (p_handle->pwm_channel < ARRAY_COUNT(p_handle->pin_num)) {
        ch_bits = __BIT(p_handle->pwm_channel);
    } else {
        ch_bits = (ch_idx_bit == 0xff) ? 0xffffffff : ch_idx_bit;
    }
    return p_handle->p_dev->ctrl_handle(p_handle, HTD_PWM_CMD_STOP, &ch_bits, sizeof(uint32_t));
}

// @brief 判断 PWM 是否在运行
// @param ch_idx通道序号，0xff 代表所有通道都启动, 当 open 配置了自适配引脚 pin_auto 这个参数无效
int32_t htd_dev_pwm_is_active(htd_pwm_handle_t* p_handle, uint8_t ch_idx)
{
    ASSERT(p_handle);
    if (p_handle->pwm_channel < ARRAY_COUNT(p_handle->pin_num)) {
        ch_idx = p_handle->pwm_channel;
    }
    return p_handle->p_dev->ctrl_handle(p_handle, HTD_PWM_CMD_ACTIVE_CHECK, &ch_idx, sizeof(uint8_t));
}




//////////////////////////////////////////////////////////////////////
#if (defined(HTD_CONSOLE_ENABLE) && defined(BSP_USING_PWM) && (DEBUG_LVL == 4) && 1)
static htd_pwm_handle_t* m_pwm_handle = NULL;

// @brief 启动
static void htd_tim_pwm_start_handle(void)
{
    if (m_pwm_handle == NULL) {
        return;
    }

    uint32_t freq_hz = htd_dev_pwm_freq_hz_get(m_pwm_handle);
    uint32_t pluse_array_size = 8;
    uint8_t* p_pluse_array = htd_malloc(sizeof(uint16_t) * pluse_array_size);
    ASSERT(p_pluse_array);

    freq_hz /= 1000 * 1000;        // 转 1us
    for (uint32_t i=0; i<pluse_array_size; ++i) {
        if (i & 0x01) {
            p_pluse_array[i] = 4 * freq_hz / 10;
        } else {
            p_pluse_array[i] = 8 * freq_hz / 10;
        }
        print_debug("freq=%d, %d(us)", p_pluse_array[i], p_pluse_array[i]/freq_hz);
    }

    // 设置周期
    htd_dev_pwm_period_set(m_pwm_handle, (12 * freq_hz + (freq_hz/2)) / 10);
    //htd_dev_pwm_duty_set(m_pwm_handle, 2, freq_hz * 40);
    htd_dev_pwm_duty8_array_set(m_pwm_handle, 2, p_pluse_array, pluse_array_size, 1);
    htd_free(p_pluse_array), p_pluse_array = NULL;

    // 启动
    htd_dev_pwm_start(m_pwm_handle, __BIT(2));
}

// @brief 调试命令
static int pwm_cmd(int argc, char **argv)
{
    if (argc < 2) {
        print_error("Usage: %s open/close", argv[0]);
        return 0;
    }

    // 打开
    if (0 == memcmp(argv[1], "open", 5)) {
        if (argc != 3) {
            print_error("Usage: %s open [tim_pwm1/tim_pwm2...]", argv[0]);
            return 0;
        }
        htd_pwm_user_config_t pwm_config;
        pwm_config.pin_num[0] = PIN_NUM_INVALID;   //{PIN_NUM_GET(B, 4), PIN_NUM_GET(B, 5), PIN_NUM_GET(B, 0), PIN_NUM_GET(B, 1)}
        pwm_config.pin_num[1] = PIN_NUM_INVALID;
        pwm_config.pin_num[2] = PIN_NUM_GET(B, 0);
        pwm_config.pin_num[3] = PIN_NUM_INVALID;
        pwm_config.mode = HTD_PWM_MODE_HL;
        pwm_config.prescaler = 0;
        pwm_config.pin_auto = PIN_NUM_INVALID;
        m_pwm_handle = htd_dev_pwm_open(argv[2], &pwm_config);
        return 0;
    } 
    // 关闭
    if (0 == memcmp(argv[1], "close", 6)) {
        if (m_pwm_handle) {
            htd_dev_pwm_close(m_pwm_handle);
            m_pwm_handle = NULL;
        }
        return 0;
    } 
    // 启动
    if (0 == memcmp(argv[1], "start", 6)) {
        htd_tim_pwm_start_handle();
        return 0;
    }
    // 停止
    if (0 == memcmp(argv[1], "stop", 5)) {
        if (m_pwm_handle) {
            htd_dev_pwm_stop(m_pwm_handle, 0xff);
        }
        return 0;
    }

    print_error("Usage: %s open/close/list", argv[0]);
    return 0;
}
CONSOLE_CMD_EXPORT(pwm_cmd, pwm);
#endif /*(defined(HTD_CONSOLE_ENABLE) && (DEBUG_LVL > 0))*/

