// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _DRV_NORFLASH_SFF_PARSER_H_        // (serials flash fit)
#define _DRV_NORFLASH_SFF_PARSER_H_

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


//////////////////////////////////////////////////////////////////////
#ifndef ARRAY_COUNT
    #define ARRAY_COUNT(X)              (sizeof(X) / sizeof(X[0]))
#endif
#ifndef __BIT
    #define __BIT(x)                    (1uL<<(x)) 
#endif

// spi 交互句柄；p_rbuff 不为空且 rbuff_len 不为0，返回读取到的数据长度, 否则返回写入p_wbuff的数据长度
typedef int32_t (*sff_spi_transfer_t)(uint8_t* p_wbuff, uint32_t wbuff_len, uint8_t* p_rbuff, uint32_t rbuff_len);

// norflash 厂商id
#define SFF_MF_ID_CYPRESS               0x01            // 赛普拉斯
#define SFF_MF_ID_FUJITSU               0x04            // 富士通
#define SFF_MF_ID_EON                   0x1C            // 硅成
#define SFF_MF_ID_ATMEL                 0x1F            // 爱特梅尔
#define SFF_MF_ID_NUMONYX               0x20            // 恒忆
#define SFF_MF_ID_MICRON                0x2C            // 美光
#define SFF_MF_ID_AMIC                  0x37            // 立锜
#define SFF_MF_ID_SANYO                 0x62            // 三洋(已经并入松下)
#define SFF_MF_ID_PM25L                 0x7F
#define SFF_MF_ID_PUYA                  0x85            // 普冉
#define SFF_MF_ID_INTEL                 0x89            // 英特尔
#define SFF_MF_ID_ESMT                  0x8C            // 晶豪
#define SFF_MF_ID_FUDAN                 0xA1            // 复旦微
#define SFF_MF_ID_HYUNDAI               0xAD            // 现代(海力士)
#define SFF_MF_ID_SST                   0xBF
#define SFF_MF_ID_MICRONIX              0xC2
#define SFF_MF_ID_GIGA                  0xC8            // 兆易创新
#define SFF_MF_ID_ISSI                  0xD5            // 北京君正
#define SFF_MF_ID_WINBOND               0xEF            // 华邦
#define SFF_MF_ID_FIDELIX               0xF8
typedef uint8_t sff_mf_id_t;

// 厂商信息表
typedef struct {
    const char* p_name;
    sff_mf_id_t id;
}sff_manufacturer_t;
const static sff_manufacturer_t m_sff_menufacturer_info_table[] = {
    {"Cypress",    SFF_MF_ID_CYPRESS},
    {"Fujitsu",    SFF_MF_ID_FUJITSU},
    {"EON",        SFF_MF_ID_EON},
    {"Atmel",      SFF_MF_ID_ATMEL},
    {"Numonyx",    SFF_MF_ID_NUMONYX},
    {"Micron",     SFF_MF_ID_MICRON},
    {"AMIC",       SFF_MF_ID_AMIC},
    {"Sanyo",      SFF_MF_ID_SANYO},
    {"Intel",      SFF_MF_ID_INTEL},
    {"ESMT",       SFF_MF_ID_ESMT},
    {"Fudan",      SFF_MF_ID_FUDAN},
    {"Hyundai",    SFF_MF_ID_HYUNDAI},
    {"SST",        SFF_MF_ID_SST},
    {"Micronix",   SFF_MF_ID_MICRONIX},
    {"GigaDevice", SFF_MF_ID_GIGA},
    {"ISSI",       SFF_MF_ID_ISSI},
    {"Winbond",    SFF_MF_ID_WINBOND},
    {"Fidelix",    SFF_MF_ID_FIDELIX},
};
// @brief 获取厂家信息
static const char* sff_manufacturer_name_get(sff_mf_id_t id)
{
    for (uint32_t i=0; i<ARRAY_COUNT(m_sff_menufacturer_info_table); ++i) {
        if (id == m_sff_menufacturer_info_table[i].id) {
            return m_sff_menufacturer_info_table[i].p_name;
        }
    }
    return NULL;
}


//////////////////////////////////////////////////////////////////////
typedef struct {
    uint8_t mf_id;                  // 厂商id
    uint8_t type_id;                // 类型id
    uint8_t capacity_id;            // 查看容量大小的命令id
    uint32_t capacity;              // flash 容量，单位字节; 超过16M(0x1000000)使用4字节地址
    uint32_t erase_unit;            // 擦除大小，单位字节
    uint8_t erase_cmd;              // 擦除命令
    uint8_t write_unit;             // 一般情况下都是4字节
    uint16_t page_size;             // 页大小
} sff_flash_info_t;
const static sff_flash_info_t m_sff_chip_info[] = { 
    {SFF_MF_ID_ATMEL,   0x26, 0x00, 2L*1024L*1024L, 512,       0x81, 4, 256},   // "AT45DB161E"
    {SFF_MF_ID_WINBOND, 0x40, 0x13, 512L*1024L,     4096,      0x20, 4, 256},   // "W25Q40BV"
    {SFF_MF_ID_WINBOND, 0x40, 0x15, 2L*1024L*1024L, 4096,      0x20, 4, 256},   // "W25Q16BV"
    {SFF_MF_ID_WINBOND, 0x40, 0x16, 4L*1024L*1024L, 4096,      0x20, 4, 256},   // "W25Q32BV"
    {SFF_MF_ID_WINBOND, 0x40, 0x17, 8L*1024L*1024L, 4096,      0x20, 4, 256},   // "W25Q64CV"
    {SFF_MF_ID_WINBOND, 0x60, 0x17, 8L*1024L*1024L, 4096,      0x20, 4, 256},   // "W25Q64DW"
    {SFF_MF_ID_WINBOND, 0x40, 0x18, 16L*1024L*1024L,4096,      0x20, 4, 256},   // "W25Q128BV"
    {SFF_MF_ID_WINBOND, 0x40, 0x19, 32L*1024L*1024L,4096,      0x20, 4, 256},   // "W25Q256FV"
    {SFF_MF_ID_SST,     0x25, 0x8E, 1L*1024L*1024L, 4096,      0x20, 4, 256},   // "SST25VF080B"
    {SFF_MF_ID_SST,     0x25, 0x41, 2L*1024L*1024L, 4096,      0x20, 4, 256},   // "SST25VF016B"
    {SFF_MF_ID_MICRON,  0x20, 0x16, 4L*1024L*1024L, 64L*1024L, 0xD8, 4, 256},   // "M25P32"
    {SFF_MF_ID_MICRON,  0x20, 0x14, 1L*1024L*1024L, 64L*1024L, 0xD8, 4, 256},   // "M25P80"
    {SFF_MF_ID_MICRON,  0x20, 0x13, 512L*1024L,     64L*1024L, 0xD8, 4, 256},   // "M25P40"
    {SFF_MF_ID_EON,     0x30, 0x16, 4L*1024L*1024L, 4096,      0x20, 4, 256},   // "EN25Q32B"
    {SFF_MF_ID_GIGA,    0x40, 0x17, 8L*1024L*1024L, 4096,      0x20, 4, 256},   // "GD25Q64B"
    {SFF_MF_ID_GIGA,    0x40, 0x15, 2L*1024L*1024L, 4096,      0x20, 4, 256},   // "GD25Q16B"
    {SFF_MF_ID_CYPRESS, 0x40, 0x15, 2L*1024L*1024L, 4096,      0x20, 4, 256},   // "S25FL216K"
    {SFF_MF_ID_CYPRESS, 0x02, 0x15, 4L*1024L*1024L, 4096,      0x20, 4, 256},   // "S25FL032P"
    {SFF_MF_ID_AMIC,    0x30, 0x14, 1L*1024L*1024L, 4096,      0x20, 4, 256},   // "A25L080"
    {SFF_MF_ID_ESMT,    0x20, 0x13, 512L*1024L,     4096,      0x20, 2, 256},   // "F25L004"
    {SFF_MF_ID_SST,     0x25, 0x41, 2L*1024L*1024L, 4096,      0x20, 2, 256},   // "PCT25VF016B"
};
// @brief 获取 flash 信息
static const sff_flash_info_t* sff_flash_info_get(uint8_t mf_id, uint8_t type_id, uint8_t capacity_id)
{
    for (uint32_t i=0; i<ARRAY_COUNT(m_sff_chip_info); ++i) {
        if (m_sff_chip_info[i].mf_id == mf_id && 
            m_sff_chip_info[i].type_id == type_id && 
            m_sff_chip_info[i].capacity_id == capacity_id) {
            return &m_sff_chip_info[i];
        }
    }
    return NULL;
}


//////////////////////////////////////////////////////////////////////
// @brief 获取 sff 解析的信息; 通过获取到的 p_info 进行norflash 的写/擦除操作
int32_t sff_parser_info_get(sff_spi_transfer_t transfer_handle, sff_flash_info_t* p_info);



#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_DRV_NORFLASH_SFF_PARSER_H_*/

