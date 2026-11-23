// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _VFS_LFS_CONFIG_H_
#define _VFS_LFS_CONFIG_H_

#include "htd_vfs_port.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

// 定义后打开所有 trace
//#define LFS_YES_TRACE
// 定义后关掉所有 debug
//#define LFS_NO_DEBUG
// 定义后关掉所有 warn
//#define LFS_NO_WARN
// 定义后关掉所有 error
//#define LFS_NO_ERROR
// 定义后关掉所有 assert
//#define LFS_NO_ASSERT

// 支持多线程，通常都是打开
#define LFS_THREADSAFE

// 文件名字最大大小
#define LFS_NAME_MAX   VFS_PATH_MAX


#ifndef PRIx32
#define PRIx32          "08x"
#endif
#ifndef PRIx16
#define PRIx16          "04x"
#endif
#ifndef PRIu32
#define PRIu32          "u"
#endif
#ifndef PRId32
#define PRId32          "d"
#endif
#ifndef PRIu16
#define PRIu16          "d"
#endif
#ifndef PRIu8
#define PRIu8           "d"
#endif

// Logging functions
#ifdef LFS_YES_TRACE
    #define LFS_TRACE(fmt, ...)         do {htd_print(fmt"\r\n", ##__VA_ARGS__);}while(0)
#else
    #define LFS_TRACE(fmt, ...)
#endif

#if !defined(LFS_NO_DEBUG)
    #define LFS_DEBUG(fmt, ...)         do {htd_print(fmt"\r\n", ##__VA_ARGS__);}while(0)
#else
    #define LFS_DEBUG(fmt, ...)
#endif

#if !defined(LFS_NO_WARN)
    #define LFS_WARN(fmt, ...)          do {htd_print("\033[0;33m"fmt"\r\n\033[0;0m", ##__VA_ARGS__);}while(0)
#else
    #define LFS_WARN(fmt, ...)
#endif

#if !defined(LFS_NO_ERROR)
    #define LFS_ERROR(fmt, ...)         do {htd_print("\033[0;31m[L%d]"fmt"\r\n\033[0;0m", __LINE__, ##__VA_ARGS__);}while(0)
#else
    #define LFS_ERROR(fmt, ...)
#endif

// Runtime assertions
#ifndef LFS_NO_ASSERT
    #define LFS_ASSERT(test)            VFS_ASSERT(test)
#else
    #define LFS_ASSERT(test)
#endif


// Builtin functions, these may be replaced by more efficient
// toolchain-specific implementations. LFS_NO_INTRINSICS falls back to a more
// expensive basic C implementation for debugging purposes

// Min/max functions for unsigned 32-bit numbers
static inline uint32_t lfs_max(uint32_t a, uint32_t b)
{
    return (a > b) ? a : b;
}

static inline uint32_t lfs_min(uint32_t a, uint32_t b)
{
    return (a < b) ? a : b;
}

// Align to nearest multiple of a size
static inline uint32_t lfs_aligndown(uint32_t a, uint32_t alignment)
{
    return a - (a % alignment);
}

static inline uint32_t lfs_alignup(uint32_t a, uint32_t alignment)
{
    return lfs_aligndown(a + alignment - 1, alignment);
}

// Find the smallest power of 2 greater than or equal to a
static inline uint32_t lfs_npw2(uint32_t a)
{
#if !defined(LFS_NO_INTRINSICS) && (defined(__GNUC__) || defined(__CC_ARM))
    return 32 - __builtin_clz(a - 1);
#elif defined(_WIN32)
    return 32 - _lzcnt_u32(a - 1);
#else
    uint32_t r = 0;
    uint32_t s;
    a -= 1;
    s = (a > 0xffff) << 4;
    a >>= s;
    r |= s;
    s = (a > 0xff) << 3;
    a >>= s;
    r |= s;
    s = (a > 0xf) << 2;
    a >>= s;
    r |= s;
    s = (a > 0x3) << 1;
    a >>= s;
    r |= s;
    return (r | (a >> 1)) + 1;
#endif
}

// Count the number of trailing binary zeros in a
// lfs_ctz(0) may be undefined
static inline uint32_t lfs_ctz(uint32_t a)
{
#if !defined(LFS_NO_INTRINSICS) && defined(__GNUC__)
    return __builtin_ctz(a);
#elif defined(_WIN32)
    return _tzcnt_u32(a);
#else
    return lfs_npw2((a & -a) + 1) - 1;
#endif
}

// Count the number of binary ones in a
static inline uint32_t lfs_popc(uint32_t a)
{
#if !defined(LFS_NO_INTRINSICS) && (defined(__GNUC__) || defined(__CC_ARM))
    return __builtin_popcount(a);
#else
    a = a - ((a >> 1) & 0x55555555);
    a = (a & 0x33333333) + ((a >> 2) & 0x33333333);
    return (((a + (a >> 4)) & 0xf0f0f0f) * 0x1010101) >> 24;
#endif
}

// Find the sequence comparison of a and b, this is the distance
// between a and b ignoring overflow
static inline int lfs_scmp(uint32_t a, uint32_t b)
{
    return (int)(unsigned)(a - b);
}

// Convert between 32-bit little-endian and native order
static inline uint32_t lfs_fromle32(uint32_t a)
{
#if !defined(LFS_NO_INTRINSICS) && ( \
    (defined(  BYTE_ORDER  ) && defined(  ORDER_LITTLE_ENDIAN  ) &&   BYTE_ORDER   ==   ORDER_LITTLE_ENDIAN  ) || \
    (defined(__BYTE_ORDER  ) && defined(__ORDER_LITTLE_ENDIAN  ) && __BYTE_ORDER   == __ORDER_LITTLE_ENDIAN  ) || \
    (defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__))
    return a;
#elif !defined(LFS_NO_INTRINSICS) && ( \
    (defined(  BYTE_ORDER  ) && defined(  ORDER_BIG_ENDIAN  ) &&   BYTE_ORDER   ==   ORDER_BIG_ENDIAN  ) || \
    (defined(__BYTE_ORDER  ) && defined(__ORDER_BIG_ENDIAN  ) && __BYTE_ORDER   == __ORDER_BIG_ENDIAN  ) || \
    (defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__))
    return __builtin_bswap32(a);
#elif defined(_WIN32)
    return a;
#else
    return (((uint8_t *)&a)[0] <<  0) |
           (((uint8_t *)&a)[1] <<  8) |
           (((uint8_t *)&a)[2] << 16) |
           (((uint8_t *)&a)[3] << 24);
#endif
}

static inline uint32_t lfs_tole32(uint32_t a)
{
    return lfs_fromle32(a);
}

// Convert between 32-bit big-endian and native order
static inline uint32_t lfs_frombe32(uint32_t a)
{
#if !defined(LFS_NO_INTRINSICS) && ( \
    (defined(  BYTE_ORDER  ) && defined(  ORDER_LITTLE_ENDIAN  ) &&   BYTE_ORDER   ==   ORDER_LITTLE_ENDIAN  ) || \
    (defined(__BYTE_ORDER  ) && defined(__ORDER_LITTLE_ENDIAN  ) && __BYTE_ORDER   == __ORDER_LITTLE_ENDIAN  ) || \
    (defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__))
    return __builtin_bswap32(a);
#elif !defined(LFS_NO_INTRINSICS) && ( \
    (defined(  BYTE_ORDER  ) && defined(  ORDER_BIG_ENDIAN  ) &&   BYTE_ORDER   ==   ORDER_BIG_ENDIAN  ) || \
    (defined(__BYTE_ORDER  ) && defined(__ORDER_BIG_ENDIAN  ) && __BYTE_ORDER   == __ORDER_BIG_ENDIAN  ) || \
    (defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__))
    return a;
#elif defined(_WIN32)
    return a;
#else
    return (((uint8_t *)&a)[0] << 24) |
           (((uint8_t *)&a)[1] << 16) |
           (((uint8_t *)&a)[2] <<  8) |
           (((uint8_t *)&a)[3] <<  0);
#endif
}

static inline uint32_t lfs_tobe32(uint32_t a)
{
    return lfs_frombe32(a);
}

// Calculate CRC-32 with polynomial = 0x04c11db7
uint32_t lfs_crc(uint32_t crc, const void *buffer, size_t size);

static inline void *lfs_malloc(size_t size)
{
#ifndef LFS_NO_MALLOC
    return htd_malloc(size);
#else
    (void)size;
    return NULL;
#endif
}

static inline void lfs_free(void *p)
{
#ifndef LFS_NO_MALLOC
    htd_free(p);
#else
    (void)p;
#endif
}


#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_VFS_LFS_CONFIG_H_*/

