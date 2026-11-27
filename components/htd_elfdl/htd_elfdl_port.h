// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_ELFDL_PORT_H_
#define _HTD_ELFDL_PORT_H_

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>
#include "htd_task.h"

#include "htd_elfdl_defined.h"
#include "htd_elfdl_section.h"
#include "htd_elfdl_list.h"

#include "htd_vfs_process.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/



//////////////////////////////////////////////////////////////////////
#ifndef ASSERT
    #define ASSERT(a)                   ASSERT(a)
#endif
#ifndef htd_snprintf
    #define htd_snprintf(buf, buff_size, fmt, ...)  snprintf(buf, buff_size, fmt, ##__VA_ARGS__)
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
#ifndef htd_strncpy
    #define htd_strncpy(dst, src, n)    strncpy(dst, src, n)
#endif
#ifndef htd_malloc
    #define htd_malloc(n)               malloc(n)
#endif 
#ifndef htd_free
    #define htd_free(p)                 free(p)
#endif 

// @brief 对接外部打印接口
extern int32_t htd_print(const char *fmt, ...);

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
#define ERR_ELFDL_SUCCESS                     (0)
#define ERR_ELFDL_FAILED                      (-0x3201)
#define ERR_ELFDL_INVALID_FILE                (-0x3202)
#define ERR_ELFDL_INVALID_FORMAT              (-0x3203)
#define ERR_ELFDL_NOT_FOUND                   (-0x3204)





//////////////////////////////////////////////////////////////////////
#include "htd_device.h"

// @brief CrC32
__STATIC_INLINE uint32_t htd_elfdl_crc32_get(const uint8_t* p_data, uint32_t data_len)
{
    htd_crc32_ctx_t ctx;
    ctx.crc32_type = HTD_CRC32_GEN;
    ctx.result = 0xffffffff;
    htd_crc32_init(&ctx);
    htd_crc32_update(&ctx, (const uint8_t*)p_data, data_len);
    return htd_crc32_final(&ctx);
}






#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_ELFDL_PORT_H_*/

