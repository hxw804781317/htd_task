// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_FLASH_PORT_H_
#define _HTD_FLASH_PORT_H_

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>

#include "htd_task.h"
#include "htd_device.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


//////////////////////////////////////////////////////////////////////
// 功能 flash-kv 使能
#ifndef HTD_FLASH_KV_ENABLE
    //#define HTD_FLASH_KV_ENABLE         1
#endif
// 功能 flash-box 使能
#ifndef HTD_FLASH_BOX_ENABLE
    //#define HTD_FLASH_BOX_ENABLE        1
#endif
// 功能 flash-log 使能
#ifndef HTD_FLASH_LOG_ENABLE
    //#define HTD_FLASH_LOG_ENABLE        1
#endif
// 功能 flash-var 使能
#ifndef HTD_FLASH_VAR_ENABLE
    //#define HTD_FLASH_VAR_ENABLE        1
#endif


//////////////////////////////////////////////////////////////////////
#ifndef ASSERT
    #define ASSERT(a)                   ASSERT(a)
#endif
#ifndef __STATIC_INLINE
    #define __STATIC_INLINE             static inline
#endif
#ifndef htd_snprintf
    #define htd_snprintf(buf, buff_size, fmt, ...)  snprintf(buf, buff_size, fmt, ##__VA_ARGS__)
#endif
#ifndef htd_vsnprintf
    #define htd_vsnprintf(buf, buff_size, fmt, ...) vsnprintf(buf, buff_size, fmt, ##__VA_ARGS__)
#endif
#ifndef htd_memset
    #define htd_memset(ptr, v, n)       memset(ptr, v, n)
#endif
#ifndef htd_memcpy
    #define htd_memcpy(dst, src, n)     memcpy(dst, src, n)
#endif
#ifndef htd_memmove
    #define htd_memmove(dst, src, n)    memmove(dst, src, n)
#endif
#ifndef htd_memcmp
    #define htd_memcmp(v1, v2, n)       memcmp(v1, v2, n)
#endif
#ifndef htd_strlen
    #define htd_strlen(str)             strlen(str)
#endif
#ifndef htd_strcmp
    #define htd_strcmp(v1, v2, n)       strcmp(v1, v2, n)
#endif
#ifndef htd_malloc
    #define htd_malloc(n)               malloc(n)
#endif 
#ifndef htd_free
    #define htd_free(p)                 free(p)
#endif 

#ifndef print_debug
    #define print_debug(...)            
#endif
#ifndef print_warning
    #define print_warning(...)          
#endif
#ifndef print_error
    #define print_error(...)            
#endif






//////////////////////////////////////////////////////////////////////
// 错误码
#define ERR_FLASH_SUCCESS                         (0)
#define ERR_FLASH_FAILED                          (-0x8001)
#define ERR_FLASH_NO_MEM                          (-0x8002)
#define ERR_FLASH_INVALID_ARG                     (-0x8003)
#define ERR_FLASH_NOT_FOUND                       (-0x8004)
#define ERR_FLASH_INVALID_DATA                    (-0x8005)
#define ERR_FLASH_BUSY                            (-0x8006)
#define ERR_FLASH_NOT_SUPPORTED                   (-0x8007)
#define ERR_FLASH_CHECKSUM_FAILED                 (-0x8008)




//////////////////////////////////////////////////////////////////////
// @brief crc8 校验值
__STATIC_INLINE uint8_t htd_flash_crc8(const uint8_t *p_data, uint16_t data_len, uint8_t crc)
{
    return htd_sfw_crc8(p_data, data_len, crc);
}




#ifdef __cplusplus 
}
#endif /*__cplusplus*/
#endif /*_HTD_FLASH_PORT_H_*/

