#ifndef _HTD_DEVICE_H_
#define _HTD_DEVICE_H_

#include "htd_task.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/



//////////////////////////////////////////////////////////////////////
// 设备
typedef struct htd_dev_s {
    uint32_t magic;
    const char* p_name;
    void* p_object;
    int32_t (*ctrl_handle)(void* p_handle, uint32_t cmd, void* p_param, uint32_t param_len);
}htd_dev_t;

#define HTD_DEV_MAGIC_MASK              0x95ad0000
#define HTD_DEV_SECTION_NAME            htd_dev
#define HTD_DEV_REGISTER(name, identify, ctrl_func, object)   \
    HTD_SECTION_ITEM_REGISTER(HTD_DEV_SECTION_NAME, static const htd_dev_t CONCAT_2(m_dev_, name)) = {\
        .magic = identify,              \
        .p_name = STR(name),            \
        .p_object = object,             \
        .ctrl_handle = ctrl_func,       \
    };

// @brief 查找对应的设备
htd_dev_t* htd_dev_find(const char* p_name, uint32_t identify_number);
// @brief 展示所有设备
void htd_dev_list(void);



//////////////////////////////////////////////////////////////////////
// 定义命令回调函数
typedef int (*dev_init_func_t)(void);
typedef struct {
    uint32_t magic;
    const char* name;
    dev_init_func_t init_func;
} dev_init_t;
#define DEV_INIT_MAGIC         0x58908761
#define DEVICE_INIT_SECTION_NAME    htd_init_device
#define DEVICE_INIT_FUNC_REGISTER(func)                                     \
    HTD_SECTION_ITEM_REGISTER(DEVICE_INIT_SECTION_NAME, static const dev_init_t CONCAT_2(htd_dev_, func)) = {\
        .magic = DEV_INIT_MAGIC,                                            \
        .name = STR(func),                                                  \
        .init_func = (dev_init_func_t)func                                  \
    };
#ifndef INIT_DEVICE_EXPORT
    #define INIT_DEVICE_EXPORT(func)            DEVICE_INIT_FUNC_REGISTER(func)
#endif

// @brief 初始化
int32_t htd_dev_init(void);



//////////////////////////////////////////////////////////////////////
// 定义事件广播类型
#define HTD_DEV_EVT_NONE                0
#define HTD_DEV_EVT_SDCARD_PLUG         1           // @uint8_t
typedef uint16_t htd_dev_evt_t;


#define HTD_EVT_MAGIC_VAL               0x5a8d
typedef struct {
    uint16_t magic;
    htd_dev_evt_t evt;
    void (*func)(void* p_param, uint32_t param_len);
}htd_evt_broadcast_t;
#define HTD_EVT_BROADCAST_SECTION       htd_broadcast_section
#define HTD_EVT_BROADCAST_HANDLE_REGISTER(_evt, _notify_handle)        \
    HTD_SECTION_ITEM_REGISTER(HTD_EVT_BROADCAST_SECTION, static const htd_evt_broadcast_t CONCAT_2(htd_broadcast_, notify_handle)) = {\
        .magic = HTD_EVT_MAGIC_VAL,         \
        .evt = _evt,                        \
        .func = _notify_handle,             \
    }

// @brief 事件广播
void htd_dev_evt_broadcast(htd_dev_evt_t evt, void* p_param, uint32_t param_len);




#ifdef __cplusplus
}
#endif /*__cplusplus*/


//////////////////////////////////////////////////////////////////////
#include "htd_sfw_aes128.h"
#include "htd_sfw_sha256.h"
#include "htd_sfw_sha1.h"
#include "htd_sfw_md5.h"
#include "htd_sfw_crc.h"

#include "htd_dev_aes.h"
#include "htd_dev_crc.h"
#include "htd_dev_delay.h"
#include "htd_dev_gpio.h"
#include "htd_dev_info.h"
#include "htd_dev_i2cm.h"
#include "htd_dev_adc.h"
#include "htd_dev_inflash.h"
#include "htd_dev_pm.h"
#include "htd_dev_rng.h"
#include "htd_dev_spim.h"
#include "htd_dev_timer.h"
#include "htd_dev_serial.h"
#include "htd_dev_wdt.h"
#include "htd_dev_rtc.h"
#include "htd_dev_timer.h"
#include "htd_dev_timer_occ.h"
#include "htd_dev_timer_pwm.h"
#include "htd_dev_hash.h"
#include "htd_dev_sdio.h"


#endif /*_HTD_DEVICE_H_*/

