// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_SECTION_H_
#define _HTD_SECTION_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CONCAT_2
    #define HTD_CONCAT_2_(p1, p2)                       p1##p2
    #define CONCAT_2(p1, p2)                            HTD_CONCAT_2_(p1, p2)
#endif
#ifndef STRINGIFY
    #define HTD_STRINGIFY_(val)                         #val
    #define STRINGIFY(val)                              HTD_STRINGIFY_(val)
#endif 

// 使用 IAR 语言扩展
#if defined(__ICCARM__)
    #pragma language=extended
#endif


// 平台相关：section/region 属性与符号
#if defined(__CC_ARM) || defined(__CLANG_ARM) || defined (__ARMCC_VERSION)
    // ARM Compiler 5/6 
    #define HTD_SECTION(section_name)                       __attribute__((used, section(STRINGIFY(section_name))))
    #define HTD_USED                                        __attribute__((used))
    #define HTD_SECTION_START_ADDR(section_name)            &CONCAT_2(section_name, $$Base)
    #define HTD_LOAD_REGION_START_ADDR(region_name)         &CONCAT_2(CONCAT_2(Load$$LR$$, region_name), $$Base)
    #define HTD_EXEC_REGION_LOAD_START_ADDR(region_name)    &CONCAT_2(CONCAT_2(Load$$, region_name), $$Base)
    #define HTD_EXEC_REGION_START_ADDR(region_name)         &CONCAT_2(CONCAT_2(Image$$, region_name), $$Base)

    #define HTD_SECTION_END_ADDR(section_name)              &CONCAT_2(section_name, $$Limit)
    #define HTD_LOAD_REGION_END_ADDR(region_name)           &CONCAT_2(CONCAT_2(Load$$LR$$, region_name), $$Limit)
    #define HTD_EXEC_REGION_LOAD_END_ADDR(region_name)      &CONCAT_2(CONCAT_2(Load$$, region_name), $$Limit)
    #define HTD_EXEC_REGION_END_ADDR(region_name)           &CONCAT_2(CONCAT_2(Image$$, region_name), $$Limit)

#elif defined(__GNUC__)
    // 纯 GCC(含 ARM GCC), 注意 __ARMCC_VERSION 优先匹配上面的 ARM 分支
    #define HTD_SECTION(section_name)                       __attribute__((used, section(STRINGIFY(section_name))))
    #define HTD_USED                                        __attribute__((used))
    #define HTD_SECTION_START_ADDR_(name)                   &CONCAT_2(CONCAT_2(__, name), _start__)
    #define HTD_SECTION_START_ADDR(name)                    HTD_SECTION_START_ADDR_(name)
    #define HTD_EXEC_REGION_LOAD_START_ADDR(region_name)    HTD_SECTION_START_ADDR(CONCAT_2(region_name, _load))

    #define HTD_SECTION_END_ADDR(section_name)              &CONCAT_2(CONCAT_2(__, section_name), _end__)
    #define HTD_EXEC_REGION_END_ADDR(region_name)           &CONCAT_2(CONCAT_2(__, region_name), _end__)
    #define HTD_EXEC_REGION_LOAD_END_ADDR(region_name)      HTD_SECTION_END_ADDR(CONCAT_2(region_name, _load))

#elif defined(__ICCARM__)
    // IAR EWARM：统一用 __section_begin/__section_end 访问 section
    #define HTD_SECTION(x)                                  @ STRINGIFY(x)
    #define HTD_USED                                        __root
    #define HTD_SECTION_START_ADDR(section_name)            __section_begin(STRINGIFY(section_name))
    #define HTD_SECTION_END_ADDR(section_name)              __section_end(STRINGIFY(section_name))
    // IAR 下 exec region 与 load region 通过不同 section 名区分，这里约定 region 的 init/load 副本放在 <region>_init section 中。
    #define HTD_EXEC_REGION_START_ADDR(region_name)         __section_begin(STRINGIFY(region_name))
    #define HTD_EXEC_REGION_END_ADDR(region_name)           __section_end(STRINGIFY(region_name))
    #define HTD_EXEC_REGION_LOAD_START_ADDR(region_name)    __section_begin(STRINGIFY(CONCAT_2(region_name, _init)))
    #define HTD_EXEC_REGION_LOAD_END_ADDR(region_name)      __section_end(STRINGIFY(CONCAT_2(region_name, _init)))
    // IAR 下 load region 与 exec region 相同(由 ICF 配置决定)
    #define HTD_LOAD_REGION_START_ADDR(region_name)         HTD_EXEC_REGION_START_ADDR(region_name)
    #define HTD_LOAD_REGION_END_ADDR(region_name)           HTD_EXEC_REGION_END_ADDR(region_name)
    
#elif defined(_MSC_VER)
    #define HTD_SECTION(x)                                  __pragma(section(STRINGIFY(x), read)) \
                                                            __declspec(allocate(STRINGIFY(x)))
    #define HTD_USED                       
    #define HTD_SECTION_START_ADDR(section_name)            &CONCAT_2(section_name, _start)
    #define HTD_LOAD_REGION_START_ADDR(region_name)         (0)

    #define HTD_SECTION_END_ADDR(section_name)              &CONCAT_2(section_name, _end)
    #define HTD_LOAD_REGION_END_ADDR(region_name)           (0)
#else
    #error not supported tool chain
#endif


/**@brief   Macro for retrieving the length of a given section, in bytes.
 *
 * @param[in]   section_name    Name of the section.
 * @hideinitializer
 */
#define HTD_SECTION_LENGTH(section_name)  ((size_t)HTD_SECTION_END_ADDR(section_name) - (size_t)HTD_SECTION_START_ADDR(section_name))
#define HTD_EXEC_REGION_SIZE(region_name) ((size_t)(HTD_EXEC_REGION_END_ADDR(region_name)) - (size_t)(HTD_EXEC_REGION_START_ADDR(region_name)))


#if defined(__CC_ARM) || defined(__CLANG_ARM) || defined (__ARMCC_VERSION)
    #define HTD_SECTION_DEF(section_name, data_type)                                \
        extern data_type * CONCAT_2(section_name, $$Base);                          \
        extern void      * CONCAT_2(section_name, $$Limit)
    #define HTD_LOAD_REGION_DEF(region_name)                                        \
        extern uint8_t * CONCAT_2(CONCAT_2(Load$$LR$$, region_name), $$Base);       \
        extern void    * CONCAT_2(CONCAT_2(Load$$LR$$, region_name), $$Limit)
    #define HTD_EXEC_REGION_LOAD_SYM_DEF(region_name)                               \
        extern uint8_t * CONCAT_2(CONCAT_2(Load$$, region_name), $$Base);           \
        extern void    * CONCAT_2(CONCAT_2(Load$$, region_name), $$Limit)
    #define HTD_EXEC_REGION_DEF(region_name)                                        \
        extern uint8_t * CONCAT_2(CONCAT_2(Image$$, region_name), $$Base);          \
        extern void    * CONCAT_2(CONCAT_2(Image$$, region_name), $$Limit)
    
#elif defined(__GNUC__)
    #define HTD_SECTION_DEF(section_name, data_type)                                \
        extern data_type * CONCAT_2(CONCAT_2(__, section_name), _start__);          \
        extern void      * CONCAT_2(CONCAT_2(__, section_name), _end__)
    #define HTD_EXEC_REGION_DEF(region_name)                                        \
        extern uint8_t * CONCAT_2(CONCAT_2(__, region_name), _start__);             \
        extern void    * CONCAT_2(CONCAT_2(__, region_name), _end__)
    #define HTD_LOAD_REGION_DEF(region_name)                                        \
        HTD_EXEC_REGION_DEF(CONCAT_2(region_name, _load))
    #define HTD_EXEC_REGION_LOAD_SYM_DEF(region_name)     HTD_LOAD_REGION_DEF(region_name)

#elif defined(__ICCARM__)
    #define HTD_SECTION_DEF(section_name, data_type)                                \
        _Pragma(STRINGIFY(section = STRINGIFY(section_name)))
    #define HTD_EXEC_REGION_DEF(region_name)                                        \
        extern uint8_t * CONCAT_2(region_name, $$Base);                             \
        extern void    * CONCAT_2(region_name, $$Limit)
    #define HTD_LOAD_REGION_DEF(region_name)                                        \
        HTD_EXEC_REGION_DEF(region_name)
    #define HTD_EXEC_REGION_LOAD_SYM_DEF(region_name)     HTD_LOAD_REGION_DEF(region_name)

#elif defined(_MSC_VER)
    #define HTD_SECTION_DEF(section_name, data_type)                                \
        HTD_SECTION(STRINGIFY(CONCAT_2(section_name,$0)))                           \
        data_type CONCAT_2(section_name, _start);                                   \
        HTD_SECTION(STRINGIFY(CONCAT_2(section_name,$2)))                           \
        data_type CONCAT_2(section_name, _end)
#endif


#if defined(__CC_ARM) || defined(__CLANG_ARM) || defined (__ARMCC_VERSION)
    #define HTD_SECTION_ITEM_REGISTER(section_name, section_var) section_var __attribute__ ((section(STRINGIFY(section_name)))) __attribute__((used))
#elif defined(__GNUC__)
    #define HTD_SECTION_ITEM_REGISTER(section_name, section_var) section_var __attribute__ ((section("." STRINGIFY(section_name)))) __attribute__((used))
#elif defined(__ICCARM__)
    #define HTD_SECTION_ITEM_REGISTER(section_name, section_var) __root section_var @ STRINGIFY(section_name)
#elif defined(_MSC_VER)
    #define HTD_SECTION_ITEM_REGISTER(section_name, section_var)  HTD_SECTION(STRINGIFY(CONCAT_2(section_name,$1)))  section_var
#endif


/**@brief   Macro for retrieving a variable from a section.
 *
 * @warning     The stored symbol can only be resolved using this macro if the
 *              type of the data is word aligned. The operation of acquiring
 *              the stored symbol relies on the size of the stored type. No
 *              padding can exist in the named section in between individual
 *              stored items or this macro will fail.
 *
 * @param[in]   section_name    Name of the section.
 * @param[in]   data_type       Data type of the variable.
 * @param[in]   i               Index of the variable in section.
 * @hideinitializer
 */
#define HTD_SECTION_ITEM_GET(section_name, data_type, i)    ((data_type*)HTD_SECTION_START_ADDR(section_name) + (i))


/**@brief   Macro for getting the number of variables in a section.
 *
 * @param[in]   section_name    Name of the section.
 * @param[in]   data_type       Data type of the variables in the section.
 * @hideinitializer
 */
#define HTD_SECTION_ITEM_COUNT(section_name, data_type)     (HTD_SECTION_LENGTH(section_name) / sizeof(data_type))

#if defined(__CC_ARM)
    #define HTD_SECTION_ZIDATA_BEGIN(section_name)          _Pragma(STRINGIFY(arm section zidata=STRINGIFY(section_name)))
    #define HTD_SECTION_ZIDATA_END                          _Pragma(STRINGIFY(arm section zidata))
#elif defined(__CLANG_ARM) || defined (__ARMCC_VERSION)
    #define HTD_SECTION_ZIDATA_BEGIN(section_name)          _Pragma(STRINGIFY(clang section bss=STRINGIFY(section_name)))
    #define HTD_SECTION_ZIDATA_END                          _Pragma(STRINGIFY(clang section bss=""))
#elif defined(__GNUC__)
    #define HTD_SECTION_ZIDATA_BEGIN(section_name)          
    #define HTD_SECTION_ZIDATA_END                          
#else
    #define HTD_SECTION_ZIDATA_BEGIN(section_name)
    #define HTD_SECTION_ZIDATA_END
#endif


// @brief 块起始地址
static inline const uint32_t* htd_section_first_item_get(const uint32_t* p_start, const uint32_t* p_end)
{
#ifdef _MSC_VER
    while (p_start < p_end) {
        if (*p_start == 0) {
            p_start++;
        } else {
            return p_start;
        }
    }
    return NULL;
#else
    return p_start;
#endif
}
// @brief 下一个条目
static inline const uint32_t* htd_section_next_item_get(const uint32_t *p_item, const uint32_t* p_end, uint32_t item_size)
{
#ifdef _MSC_VER
    p_item += (item_size / sizeof(uint32_t*));
    do {
        if (p_item >= p_end) {
            return NULL;
        }
        if (*p_item) {
            break;
        }
        p_item++;
    } while(1);
    return p_item;
#else
    p_item += (item_size / sizeof(uint32_t*));
    if (p_item >= p_end) {
        return NULL;
    }
    return p_item;
#endif
}


/** @} */
#ifdef __cplusplus
}
#endif
#endif // _HTD_SECTION_H_

