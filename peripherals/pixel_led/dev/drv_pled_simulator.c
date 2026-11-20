#define DEBUG_LVL               4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG               "pled.simulator"
#include "htd_common.h"
#include "htd_dev_pixel_led.h"
#include "htd_device.h"


#if (defined(HTD_PIXEL_LED_USING_SIMULATE) && (HTD_PIXEL_LED_USING_SIMULATE == 1))
//////////////////////////////////////////////////////////////////////
// @brief 模拟打开
void drv_pled_simulator_open(void)
{

}

// @brief 模拟关闭
void drv_pled_simulator_close(bool is_power_off)
{

}

// @brief 设置颜色
void drv_pled_simulator_color_set(pled_color_t* p_color, uint32_t led_count)
{

}

// @brief 擦除函数
static int32_t drv_pled_simulator_ctrl(htd_pled_cmd_t cmd, void* p_param, uint32_t param_len)
{
    if (cmd == PLED_CMD_OPEN) {
        drv_pled_simulator_open();
    } else if (cmd == PLED_CMD_CLOSE) {
        drv_pled_simulator_close(true);
    } else if (cmd == PLED_CMD_COLOR_SET) {
        drv_pled_simulator_color_set((pled_color_t*)p_param, param_len);
    } else {
        return (-ERROR_CODE_INVALID_PARAM);
    }
    return 0;
}

// @brief 注册到上层应用中
static int32_t drv_pled_simulator_register(void)
{
    const static htd_pled_dev_t m_pled_dev = {
        .p_name = "simulator",
        .ctrl_handle = drv_pled_simulator_ctrl,
    };
    return htd_dev_pled_register(&m_pled_dev);
}
DEVICE_INIT_FUNC_REGISTER(drv_pled_simulator_register);
#endif /*defined(HTD_VFS_MODULE_ENABLE) && defined(_WIN32)*/

