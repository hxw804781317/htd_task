// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_COMPILER_H__
#define _HTD_COMPILER_H__

#if (defined ( __CC_ARM ) || defined(__CLANG_ARM))
    #ifndef __ASM
        #define __ASM               __asm
    #endif
    #ifndef __INLINE
        #define __INLINE            __inline
    #endif
    #ifndef __WEAK
        #define __WEAK              __weak
    #endif
    #ifndef __ALIGN
        #define __ALIGN(n)          __align(n)
    #endif
    #ifndef __PACKED
        #define __PACKED            __packed
    #endif
    #ifndef __GET_SP
        #define __GET_SP()          __current_sp()
    #endif
    #ifndef __STATIC_INLINE
        #define __STATIC_INLINE     static __inline
    #endif
    #ifndef __UNUSED
        #define __UNUSED            __attribute__((unused))
    #endif
    #ifndef __USED
        #define __USED              __attribute__((used))
    #endif
    
#elif defined (__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
    #ifndef __ASM
        #define __ASM               __asm
    #endif
    #ifndef __INLINE
        #define __INLINE            __inline
    #endif
    #ifndef __WEAK
        #define __WEAK              __attribute__((weak))
    #endif
    #ifndef __ALIGN
        #define __ALIGN(n)          __attribute__((aligned(n)))
    #endif
    #ifndef __PACKED
        #define __PACKED            __attribute__((packed, aligned(1)))
    #endif
    #ifndef __GET_SP
        #define __GET_SP()          __current_sp()
    #endif
    #ifndef __STATIC_INLINE
        #define __STATIC_INLINE     static __inline
    #endif
    #ifndef __UNUSED
        #define __UNUSED            __attribute__((unused))
    #endif
    #ifndef __USED
        #define __USED              __attribute__((used))
    #endif
    
#elif ((defined __ICCARM__) || (defined __IAR_SYSTEMS_ICC__))
    #ifndef __ASM
        #define __ASM               __asm
    #endif
    #ifndef __INLINE
        #define __INLINE            inline
    #endif
    #ifndef __WEAK
        #define __WEAK              __weak
    #endif
    #ifndef __ALIGN
        #define STRING_PRAGMA(x) _Pragma(#x)
        #define __ALIGN(n) STRING_PRAGMA(data_alignment = n)
    #endif
    #ifndef __PACKED
        #define __PACKED            __packed
    #endif
    #ifndef __STATIC_INLINE
        #define __STATIC_INLINE     static inline
    #endif
    #ifndef __GET_SP()
        #define __GET_SP()          __get_SP()
    #endif
    #ifndef __UNUSED
        #define __UNUSED
    #endif
    #ifndef __USED
        #define __USED              __root
    #endif

#elif defined   ( __GNUC__ )
    #ifndef __ASM
        #define __ASM               __asm
    #endif
    #ifndef __INLINE
        #define __INLINE            inline
    #endif
    #ifndef __WEAK
        #define __WEAK              __attribute__((weak))
    #endif
    #ifndef __ALIGN
        #define __ALIGN(n)          __attribute__((aligned(n)))
    #endif
    #ifndef __PACKED
        #define __PACKED           __attribute__((packed)) 
    #endif
    #ifndef __STATIC_INLINE
        #define __STATIC_INLINE     static inline
    #endif
    #ifndef __GET_SP
        #define __GET_SP()         gcc_current_sp()
        static inline unsigned int gcc_current_sp(void)
        {
            register unsigned sp __ASM("sp");
            return sp;
        }
    #endif
    #ifndef __UNUSED
        #define __UNUSED            __attribute__((unused))
    #endif
    #ifndef __USED
        #define __USED              __attribute__((used))
    #endif

#elif defined (__TASKING__)
    #ifndef __ASM
        #define __ASM               __asm
    #endif
    #ifndef __INLINE
        #define __INLINE            inline
    #endif
    #ifndef __WEAK
        #define __WEAK              __attribute__((weak))
    #endif
    #ifndef __ALIGN
        #define __ALIGN(n)          __align(n)
    #endif
    #ifndef __PACKED
        #define __PACKED
    #endif
    #ifndef __STATIC_INLINE
        #define __STATIC_INLINE     static inline
    #endif
    #ifndef __GET_SP
        #define __GET_SP()          __get_MSP()
    #endif
    
#elif (defined _MSC_VER)            // 微软windows
    #ifndef __WEAK
        #define __WEAK
    #endif
    #ifndef __ALIGN
        #define __ALIGN(n)          __declspec(align(n))
    #endif
    #ifndef __STATIC_INLINE
        #define __STATIC_INLINE     static __inline
    #endif
    #ifndef __UNUSED
        #define __UNUSED
    #endif
    #ifndef __USED
        #define __USED
    #endif
#endif


///////////////////////////////////////////////////////////////
typedef union {
    char ch;
    unsigned long val;
}endian_check_t;
const static endian_check_t endian_check = {.val = 0x000000a5};
// 检查大小端，大端返回true，小端返回false
#define CHECK_CPU_ENDIAN        (endian_check.ch)





///////////////////////////////////////////////////////////////
#ifdef __CC_ARM
    #define STATIC_ASSERT_SIMPLE(EXPR)      extern char (*_do_assert(void)) [sizeof(char[1 - 2*!(EXPR)])]
    #define STATIC_ASSERT_MSG(EXPR, MSG)    extern char (*_do_assert(void)) [sizeof(char[1 - 2*!(EXPR)])]
#elif defined(__ICCARM__)
    #define STATIC_ASSERT_SIMPLE(EXPR)      static_assert(EXPR, "unspecified message")
    #define STATIC_ASSERT_MSG(EXPR, MSG)    static_assert(EXPR, MSG)
#elif defined(__GNUC__)
    #define STATIC_ASSERT_SIMPLE(EXPR)      _Static_assert(EXPR, "unspecified message")
    #define STATIC_ASSERT_MSG(EXPR, MSG)    _Static_assert(EXPR, MSG)
#elif defined(__LINT__)
    #define STATIC_ASSERT_SIMPLE(EXPR)      extern char (*_ignore(void))
    #define STATIC_ASSERT_MSG(EXPR, MSG)    extern char (*_ignore(void))
#elif defined(_MSC_VER)
    #define STATIC_ASSERT_SIMPLE(EXPR)     
    #define STATIC_ASSERT_MSG(EXPR, MSG)    
#else
    #error "Please select a compiler."
#endif
// 静态 static 
#define _SELECT_ASSERT_FUNC(x, EXPR, MSG, ASSERT_MACRO, ...) ASSERT_MACRO
#define STATIC_ASSERT(...)                                                    \
    _SELECT_ASSERT_FUNC(x, ##__VA_ARGS__,                                     \
                        STATIC_ASSERT_MSG(__VA_ARGS__),                       \
                        STATIC_ASSERT_SIMPLE(__VA_ARGS__))



#endif /*_HTD_COMPILER_H__*/

