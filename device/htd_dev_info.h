// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_DEV_INFO_H_
#define _HTD_DEV_INFO_H_

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


//////////////////////////////////////////////////////////////////
// RAM 内存起始位置
#if defined(__CC_ARM) || defined(__ARMCC_VERSION)   // KEIL
    extern uint32_t Image$$RW_IRAM1$$Base;          // 配置的内存起始地址
    #define HTD_RAM_START       ((uintptr_t)&Image$$RW_IRAM1$$Base)
#elif defined(__ICCARM__)                           // IAR
    extern uint32_t __ICFEDIT_region_RAM_start__;
    extern uint32_t __ICFEDIT_region_RAM_end__;
    #define HTD_RAM_START       ((uintptr_t)&__ICFEDIT_region_RAM_start__)
    #define HTD_RAM_END         ((uintptr_t)&__ICFEDIT_region_RAM_end__)
#elif defined(__SES_ARM)
    extern uint32_t __app_ram_start__;
    #define HTD_RAM_START       ((uintptr_t)&__app_ram_start__)
#elif defined(__GNUC__)
    extern uint32_t __data_start__;
    #define HTD_RAM_START       ((uintptr_t)&__data_start__)
#endif
//lint -restore



//////////////////////////////////////////////////////////////////
// 栈
#if defined(__LINT__)
    #define HTD_STACK_BASE      0x1F000 // Arbitrary value.
    #define HTD_STACK_END       0x20000 // Arbitrary value.
#elif defined(__CC_ARM) || defined(__ARMCC_VERSION)
    extern char STACK$$Base;
    extern char STACK$$Length;
    #define HTD_STACK_BASE      ((uintptr_t)&STACK$$Base)
    #define HTD_STACK_END       (HTD_STACK_BASE + (uintptr_t)&STACK$$Length)
#elif defined(__ICCARM__)
    extern char CSTACK$$Base;
    extern char CSTACK$$Length;
    #define HTD_STACK_BASE      ((uintptr_t)&CSTACK$$Base)
    #define HTD_STACK_END       (HTD_STACK_BASE + (uintptr_t)&CSTACK$$Length)
#elif defined(__GNUC__)
    extern uint32_t __StackTop;
    extern uint32_t __StackLimit;
    #define HTD_STACK_BASE      ((uintptr_t)&__StackTop) 
    #define HTD_STACK_END       (HTD_STACK_BASE + ((uintptr_t)&__StackLimit))
#endif


//////////////////////////////////////////////////////////////////
// 堆
#if defined(__ARMCC_VERSION)
    extern int Image$$RW_IRAM1$$ZI$$Limit;
    #define HTD_HEAP_BEGIN      ((uintptr_t)&Image$$RW_IRAM1$$ZI$$Limit)

    extern int Image$$RW_IRAM2$$ZI$$Limit;
    #define HTD_HEAP2_BEGIN      ((uintptr_t)&Image$$RW_IRAM2$$ZI$$Limit)
#elif __ICCARM__
    #pragma section="CSTACK"
    #define HTD_HEAP_BEGIN      (__segment_end("CSTACK"))
#else
    extern int __bss_end__;
    #define HTD_HEAP_BEGIN      ((uintptr_t)&__bss_end__)
#endif



//////////////////////////////////////////////////////////////////
/* These macros are valid only when absolute placement is used for the application
 * image. The macros are not compile time symbols. They cannot be used as a
 * constant expression, for example, inside a static assert or linker script
 * at-placement. */
#if defined(__LINT__)
    #define HTD_CODE_START (0)      // Arbitrary value.
    #define HTD_CODE_END   (0x1000) // Arbitrary value.
    #define HTD_CODE_SIZE  (0x1000) // Arbitrary value.
#elif defined(__CC_ARM) || defined(__ARMCC_VERSION)
    extern char Load$$LR$$LR_IROM1$$Base;
    extern char Load$$LR$$LR_IROM1$$Length;
    extern char Load$$LR$$LR_IROM1$$Limit;
    #define HTD_CODE_START ((uintptr_t)&Load$$LR$$LR_IROM1$$Base)
    #define HTD_CODE_END   ((uintptr_t)&Load$$LR$$LR_IROM1$$Limit)
    #define HTD_CODE_SIZE  ((uintptr_t)&Load$$LR$$LR_IROM1$$Length)
#elif defined(__ICCARM__)
    extern void * __vector_table;
    extern char RO_END$$Base;
    #define HTD_CODE_START ((uintptr_t)&__vector_table)
    #define HTD_CODE_END   ((uintptr_t)&RO_END$$Base)
    #define HTD_CODE_SIZE  (HTD_CODE_END - HTD_CODE_START)
#elif defined(__SES_ARM)
    extern uint32_t * _vectors;
    extern uint32_t __FLASH_segment_used_end__;
    #define HTD_CODE_START (((uintptr_t))&_vectors)
    #define HTD_CODE_END   ((uintptr_t)&__FLASH_segment_used_end__)
    #define HTD_CODE_SIZE  (HTD_CODE_END - HTD_CODE_START)
#elif defined(__GNUC__)
    extern uint32_t __isr_vector;
    extern uint32_t __etext;
    #define HTD_CODE_START ((uintptr_t)&__isr_vector)
    #define HTD_CODE_END   ((uintptr_t)&__etext)
    #define HTD_CODE_SIZE  (HTD_CODE_END - HTD_CODE_START)
#endif




//////////////////////////////////////////////////////////////////
typedef struct {
    uint32_t addr;
    uint32_t size;
} htd_ram_partition_t;


// @brief 获取时钟频率(单位HZ)
uint32_t htd_rcc_freq_hz_get(void);
// @brief 获取芯片id
uint32_t htd_mcu_id_get(uint8_t* p_id, uint32_t id_len);
// @brief 获取 stack 起始地址
uintptr_t htd_stack_start_addr_get(void);
// @brief 获取 stack 大小
size_t htd_stack_size_get(void);
// @brief 获取内存堆信息，返回总共多少个内存堆
// @param idx 第几块内存，partition_count 获取多少块内存信息
uint32_t htd_heap_partition_get(htd_ram_partition_t* p_info, uint32_t partition_count, uint32_t idx);
// @brief 获取 code 起始地址
uintptr_t htd_code_start_addr_get(void);
// @brief 获取 code 大小, 单位byte
size_t htd_code_size_get(void);

// @brief 显示内存信息
void htd_dev_ram_info_show(void);
// @brief 显示设备信息
void htd_dev_info_show(void);


#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_DEV_INFO_H_*/

