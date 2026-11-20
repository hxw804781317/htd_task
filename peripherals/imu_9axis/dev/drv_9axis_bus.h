#ifndef _DRV_9AXIS_BUS_H_
#define _DRV_9AXIS_BUS_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


// @brief 上电控制
void drv_9axis_power_ctrl(bool enable);
// @brief 总线初始化
int32_t drv_9axis_bus_init(void);
// @brief 总线去初始化
int32_t drv_9axis_bus_deinit(void);


// @brief 总线读数据
int32_t drv_9axis_bus_data_read(uint8_t reg, uint8_t* p_data, uint16_t data_size);
// @brief 写寄存器地址
int32_t drv_9axis_register_write(uint8_t reg, uint8_t val);
// @brief 读寄存器
uint8_t drv_9axis_register_read(uint8_t reg);


#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_DRV_9AXIS_BUS_H_*/

