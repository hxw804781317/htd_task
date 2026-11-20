#ifndef _HTD_BAROMETER_PROCESS_H_
#define _HTD_BAROMETER_PROCESS_H_

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
 
#ifdef __cplusplus
 extern "C" {
#endif /*__cplusplus*/


// 默认的标准大气压 101.325kPa = 101325Pa
#define BAR_AIR_PRESSUR_DEFAULT_VALUE           (101325)
// 标准大气压最大值和最小值
#define BAR_AIR_PRESSURE_MAX_VALUE              (110000)
#define BAR_AIR_PRESSURE_MIN_VALUE              (30000)
#define BAR_AIR_PRESSURE_IS_VALID(v)            (((v) >= BAR_AIR_PRESSURE_MIN_VALUE) && ((v) <= BAR_AIR_PRESSURE_MIN_VALUE))

// @brief 读 flash 的出厂校准值(weak函数，需要外部实现)
int32_t htd_barometer_factory_flash_read(void* p_data, uint32_t data_len);
// @brief 写入出厂校准值(weak函数，需要外部实现)
int32_t htd_barometer_factory_flash_write(const void* p_data, uint32_t data_len);


// @brief 开始计量
int32_t htd_barometer_start(void);
// @brief 停止
int32_t htd_barometer_stop(void);
// @brief 读取当前气压值，单位pa
uint32_t htd_barometer_pressure_get(void);
// @brief 读取当前海拔高度
int32_t htd_barometer_altitude_get(void);
// @brief 出厂时，设置当前气压值
int32_t htd_barometer_factory_calibrate_set(uint32_t pressure_pa, uint32_t sea_level_pa);
// @brief 设置当前海平面气压值
int32_t htd_barometer_sea_leval_pressure_set(int32_t sea_level_pa);


#ifdef __cplusplus
}
#endif
#endif /*_HTD_BAROMETER_PROCESS_H_*/

