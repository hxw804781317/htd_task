#ifndef _ARCH_CC_H_
#define _ARCH_CC_H_

#include "rtconfig.h"
#include "htd_console_log.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/



#ifndef BYTE_ORDER
    #ifdef ARCH_CPU_BIG_ENDIAN
        #define BYTE_ORDER BIG_ENDIAN
    #else
        #define BYTE_ORDER LITTLE_ENDIAN
    #endif /* ARCH_CPU_BIG_ENDIAN */
#endif /* BYTE_ORDER */


#if defined(__CC_ARM)   /* ARMCC compiler */
    #define PACK_STRUCT_FIELD(x) x
    #define PACK_STRUCT_STRUCT __attribute__ ((__packed__))
    #define PACK_STRUCT_BEGIN
    #define PACK_STRUCT_END
#elif defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050) /*Arm Compiler 6*/
    #define PACK_STRUCT_FIELD(x) x
    #define PACK_STRUCT_STRUCT __attribute__((packed))
    #define PACK_STRUCT_BEGIN
    #define PACK_STRUCT_END
#elif defined(__IAR_SYSTEMS_ICC__)   /* IAR Compiler */
    #define PACK_STRUCT_BEGIN
    #define PACK_STRUCT_STRUCT
    #define PACK_STRUCT_END
    #define PACK_STRUCT_FIELD(x) x
    #define PACK_STRUCT_USE_INCLUDES
#elif defined(__GNUC__)     /* GNU GCC Compiler */
    #define PACK_STRUCT_FIELD(x) x
    #define PACK_STRUCT_STRUCT __attribute__((packed))
    #define PACK_STRUCT_BEGIN
    #define PACK_STRUCT_END
#elif defined(_MSC_VER)
    #define PACK_STRUCT_FIELD(x) x
    #define PACK_STRUCT_STRUCT
    #define PACK_STRUCT_BEGIN
    #define PACK_STRUCT_END
    #define PACK_STRUCT_USE_INCLUDES
#endif

#if 0
#define X8_F  "2x"
#define U16_F "4d"
#define S16_F "4d"
#define X16_F "4x"
#define U32_F "8ld"
#define S32_F "8ld"
#define X32_F "8lx"
#endif

#define SZT_F "lu" 
#define X8_F  "02x"
#define U16_F "u"
#define S16_F "d"
#define X16_F "x"
#define U32_F "u"
#define S32_F "d"
#define X32_F "x"



void sys_arch_assert(const char* file, int line);
#define LWIP_PLATFORM_DIAG(x)   do {htd_console_print x;} while(0)
#define LWIP_PLATFORM_ASSERT(x) do {htd_console_print(x); sys_arch_assert(__FILE__, __LINE__);}while(0)


#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /* _ARCH_CC_H_ */

