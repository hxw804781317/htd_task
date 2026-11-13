// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_DEV_DELAY_H_
#define _HTD_DEV_DELAY_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

// @brief 延迟ns
void htd_delay_ns(uint32_t ns);
// @brief 延迟us
void htd_delay_us(uint32_t us);
// @brief 延迟毫秒
void htd_delay_ms(uint32_t ms);

// @brief 调试接口
void htd_delay_test(void);



#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_DEV_DELAY_H_*/

