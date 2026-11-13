#define DEBUG_LVL           1       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG           "dev.timer"
#include "htd_dev_timer.h"
#include "htd_device.h"
#include "htd_components.h"


//////////////////////////////////////////////////////////////////////
// @brief 创建硬件定时器
htd_timer_handle_t* htd_dev_timer_create(const char* p_name, uint32_t timeout_us, htd_timer_mode_t mode, timer_timeout_notify_t timeout_func, void* arg)
{
    ASSERT(timeout_us);
    ASSERT(timeout_func);

    // 创建新的设备
    htd_timer_handle_t* p_handle = htd_malloc(sizeof(htd_timer_handle_t));
    ASSERT(p_handle);
    htd_memset(p_handle, 0, sizeof(htd_timer_handle_t));
    p_handle->notify_handle = timeout_func;
    p_handle->timeout_us = timeout_us;
    p_handle->mode = mode;
    p_handle->arg = arg;
    if (p_name) {
        htd_strncpy(p_handle->name, p_name, sizeof(p_handle->name));
    }

    int32_t err_code;
    const htd_dev_t* p_dev = NULL;
    while (1) {
        if (p_dev) {
            p_dev = htd_dev_find(p_dev->p_name, HTD_TIMER_DEV_IDENTIFY);
        } else {
            p_dev = htd_dev_find(NULL, HTD_TIMER_DEV_IDENTIFY);
        }
        if (p_dev == NULL) {
            break;
        }
        
        ASSERT(p_dev->ctrl_handle);
        p_handle->p_dev = p_dev;
        err_code = p_dev->ctrl_handle(p_handle, HTD_TIMER_CMD_CREATE, NULL, 0);
        if (err_code == ERROR_CODE_SUCCESS) {
            return p_handle;
        }
    }
    print_error("timer open failed. %s", p_name);
    htd_free(p_handle);
    return NULL;
}

// @brief 销毁定时器
int32_t htd_dev_timer_destroy(htd_timer_handle_t* p_handle)
{
    ASSERT(p_handle && p_handle->p_dev);
    int32_t err_code = p_handle->p_dev->ctrl_handle(p_handle, HTD_TIMER_CMD_DESTROY, NULL, 0);
    htd_free(p_handle);
    return err_code;
}

// @brief 启动硬件定时器
int32_t htd_dev_timer_start(htd_timer_handle_t* p_handle)
{
    ASSERT(p_handle && p_handle->p_dev);
    return p_handle->p_dev->ctrl_handle(p_handle, HTD_TIMER_CMD_START, NULL, 0);
}

// @brief 重启定时器
int32_t htd_dev_timer_restart(htd_timer_handle_t* p_handle, uint32_t timeout_us)
{
    ASSERT(p_handle && p_handle->p_dev);
    p_handle->timeout_us = timeout_us;
    return p_handle->p_dev->ctrl_handle(p_handle, HTD_TIMER_CMD_RESTART, &timeout_us, sizeof(uint32_t));
}

// @brief 停止硬件定时器
int32_t htd_dev_timer_stop(htd_timer_handle_t* p_handle)
{
    ASSERT(p_handle && p_handle->p_dev);
    return p_handle->p_dev->ctrl_handle(p_handle, HTD_TIMER_CMD_STOP, NULL, 0);
}

// @brief 设置定时器模式
int32_t htd_dev_timer_mode_set(htd_timer_handle_t* p_handle, htd_timer_mode_t mode)
{
    ASSERT(p_handle && p_handle->p_dev);
    p_handle->mode = mode;
    return p_handle->p_dev->ctrl_handle(p_handle, HTD_TIMER_CMD_MODE_SET, &mode, sizeof(htd_timer_mode_t));
}

// @brief 检查硬件定时器是否在运行
bool htd_dev_timer_is_active(htd_timer_handle_t* p_handle)
{
    ASSERT(p_handle && p_handle->p_dev);
    return (p_handle->state == HTD_TIMER_STATE_START);
}

// @brief 获取定时器参数
void* htd_dev_timer_param_get(htd_timer_handle_t* p_handle)
{
    return p_handle->arg;
}


//////////////////////////////////////////////////////////////////////
#if (defined(HTD_CONSOLE_ENABLE) && (DEBUG_LVL == 4) && (!defined(_WIN32)))

// @brief 回调句柄
void htd_dev_timer_timeout_handle(void* handle)
{
    uint32_t *p_loop = htd_dev_timer_param_get((htd_timer_handle_t*)handle);
    if (p_loop) {
        p_loop[0]++;
    }
    print_warning("timeout");
}

// @brief 测试句柄
static void htd_dev_timer_test(void)
{
#if 1
    uint32_t loop_cnt = 0;
    // UP: 65538, 65539;  Compare: 65439, 65440
    htd_timer_handle_t* p_handle = htd_dev_timer_create("test", 65440, HTD_TIMER_MODE_REPEAT, htd_dev_timer_timeout_handle, &loop_cnt);
    ASSERT(p_handle);
    // 启动
    htd_dev_timer_start(p_handle);
    // 切换循环模式
    htd_dev_timer_mode_set(p_handle, HTD_TIMER_MODE_ONESHOT);
    htd_delay_ms(1000);
    htd_dev_timer_stop(p_handle);
    // 销毁
    htd_dev_timer_destroy(p_handle);
    p_handle = NULL;
#endif
#if 0
    // 单次, 60000, 65537, 75535
    htd_timer_handle_t* p_handle = htd_dev_timer_create("test", 65535+10000, HTD_TIMER_MODE_ONESHOT, htd_dev_timer_timeout_handle, NULL);
    ASSERT(p_handle);
    // 启动
    htd_dev_timer_start(p_handle);
    htd_delay_ms(1000);
    htd_dev_timer_stop(p_handle);
    // 销毁
    htd_dev_timer_destroy(p_handle);
    p_handle = NULL;
#endif
}

// @brief 调试命令
static int htd_timer_cmd(int argc, char **argv)
{
    if (argc < 2) {
        print_error("Usage: %s test", argv[0]);
        return 0;
    }
    // 测试
    if (0 == htd_memcmp(argv[1], "test", 5)) {
        htd_dev_timer_test();
        return 0;
    }
    print_error("Usage: %s test", argv[0]);
    return 0;
}
CONSOLE_CMD_EXPORT(htd_timer_cmd, tim);
#endif /*(defined(HTD_CONSOLE_ENABLE) && (DEBUG_LVL > 0))*/


