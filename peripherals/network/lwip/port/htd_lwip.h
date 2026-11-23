#ifndef _HTD_LWIP_H_
#define _HTD_LWIP_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


// @brief 初始化
int32_t htd_lwip_init(void);

// @brief 去初始化
int32_t htd_lwip_uninit(void);


#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_LWIP_H_*/

