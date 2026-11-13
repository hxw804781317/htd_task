#ifndef _HTD_DEV_RNG_H_
#define _HTD_DEV_RNG_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


// @brief 获取随机种子(WEAK 函数，驱动实现)
// @return 返回0说明没有获取到对应的随机值
uint32_t htd_dev_rng_data_get(void);


// @brief 获取随机值
uint32_t htd_rng_val_get(void);
// @brief 获取随机数据
uint32_t htd_rng_data_get(uint8_t* p_data, uint32_t data_len);
// @brief 初始化
int32_t htd_rng_init(void);


#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_DEV_RNG_H_*/

