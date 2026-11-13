#ifndef _HTD_DEV_TIMER_H_
#define _HTD_DEV_TIMER_H_

#include "htd_device.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


//////////////////////////////////////////////////////////////////////
#define HTD_TIMER_STATE_NONE            0
#define HTD_TIMER_STATE_START           1
#define HTD_TIMER_STATE_STOP            2
typedef uint8_t htd_timer_state_t;

#define HTD_TIMER_MODE_ONESHOT          0
#define HTD_TIMER_MODE_REPEAT           1
typedef uint8_t htd_timer_mode_t;

#define HTD_TIMER_CMD_NONE              0
#define HTD_TIMER_CMD_MODE_SET          1 
#define HTD_TIMER_CMD_START             2
#define HTD_TIMER_CMD_RESTART           3
#define HTD_TIMER_CMD_STOP              4
#define HTD_TIMER_CMD_CREATE            5
#define HTD_TIMER_CMD_DESTROY           6
typedef uint32_t htd_timer_cmd_t;

typedef struct htd_timer_handle_s htd_timer_handle_t;
typedef void (*timer_timeout_notify_t)(void* handle);
struct htd_timer_handle_s {
    const htd_dev_t* p_dev;
    timer_timeout_notify_t notify_handle;
    uint32_t timeout_us;
    uint32_t elapsed_us;
    uint16_t arr_def;
    uint16_t start_us;
    uint16_t compare_def;
    htd_timer_state_t state;
    htd_timer_mode_t mode;
    char name[16];
    void* arg;
};
#define HTD_TIMER_DEV_IDENTIFY          (HTD_DEV_MAGIC_MASK | 0x8823)

// @brief 创建硬件定时器
htd_timer_handle_t* htd_dev_timer_create(const char* p_name, uint32_t timeout_us, htd_timer_mode_t mode, timer_timeout_notify_t timeout_func, void* arg);
// @brief 销毁定时器
int32_t htd_dev_timer_destroy(htd_timer_handle_t* p_handle);


// @brief 启动硬件定时器
int32_t htd_dev_timer_start(htd_timer_handle_t* p_handle);
// @brief 重启定时器
int32_t htd_dev_timer_restart(htd_timer_handle_t* p_handle, uint32_t timeout_us);
// @brief 停止硬件定时器
int32_t htd_dev_timer_stop(htd_timer_handle_t* p_handle);

// @brief 设置定时器模式
int32_t htd_dev_timer_mode_set(htd_timer_handle_t* p_handle, htd_timer_mode_t mode);
// @brief 检查硬件定时器是否在运行
bool htd_dev_timer_is_active(htd_timer_handle_t* p_handle);
// @brief 获取定时器参数
void* htd_dev_timer_param_get(htd_timer_handle_t* p_handle);



#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_DEV_TIMER_H_*/

