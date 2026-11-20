#ifndef _HTD_BAROMETER_DEV_H_
#define _HTD_BAROMETER_DEV_H_

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include "htd_device.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


//////////////////////////////////////////////////////////////////////
typedef struct {
    const char* chip_name;
    // 初始化函数，并获取设备的id
    int32_t (*init)(void);
    // 去初始化函数，设备进入低功耗状态
    void (*uninit)(bool is_power_off);
    // 获取气压值
    int32_t (*read_handle)(uint32_t *pressure_pa);
} barometer_dev_t;


// @brief 函数注册
int32_t htd_barometer_dev_register(const barometer_dev_t* p_dev);
// @brief 初始化气压计设备
int32_t htd_barometer_dev_init(void);
// @brief 去初始化(设备进入低功耗状态)
void htd_barometer_dev_uninit(bool is_power_off);
// @brief 获取设备名字
const char* htd_barometer_dev_name_get(char* p_name);
// @brief 读取温度值和气压值
int32_t htd_barometer_dev_read(uint32_t *pressure_pa);




#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_BAROMETER_DEV_H_*/

