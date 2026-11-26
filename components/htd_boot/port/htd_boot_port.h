// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#ifndef _HTD_BOOT_PORT_H_
#define _HTD_BOOT_PORT_H_

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#include "htd_task.h"
#include "htd_boot_key.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/



//////////////////////////////////////////////////////////////////////
// 是否支持 fastlz 压缩方式
//#define HTD_BOOT_SUPPORT_COMPRESS_FASTLZ    1
// 是否支持 quicklz 压缩方式
//#define HTD_BOOT_SUPPORT_COMPRESS_QUICKLZ   1
// 是否支持增量升级
//#define HTD_BOOT_SUPPORT_UPDATE_DIFF        1


// 是否支持串口通信
//#define HTD_BOOT_SUPPORT_SERIAL             1
// 是否支持 YModem 通信协议
//#define HTD_BOOT_SUPPORT_SERIAL_YMODEM      1


#ifdef HTD_BOOT_SUPPORT_SERIAL
    // 串口设备名字
    #ifndef HTD_BOOT_SERIAL_NAME
        #define HTD_BOOT_SERIAL_NAME        "uart1"
    #endif
    // 串口引脚 TX
    #ifndef HTD_BOOT_SERIAL_TX_PINNUM
        #define HTD_BOOT_SERIAL_TX_PINNUM   0x09
    #endif
    // 串口引脚 RX
    #ifndef HTD_BOOT_SERIAL_RX_PINNUM
        #define HTD_BOOT_SERIAL_RX_PINNUM   0x0a
    #endif
    // 串口波特率
    #ifndef HTD_BOOT_SERIAL_BAUDRATE
        #define HTD_BOOT_SERIAL_BAUDRATE    115200
    #endif
#endif

// bootloader 程序地址和大小
#ifndef HTD_BOOTLOADER_ADDR
    #define HTD_BOOTLOADER_ADDR             0x8000000
#endif
#ifndef HTD_BOOTLOADER_SIZE
    #define HTD_BOOTLOADER_SIZE             0x8000
#endif
// 配置信息地址和大小(至少两个擦除单位), 
// 如果该地址配置为0xffffffff, 则为 HTD_BOOTLOADER_ADDR 地址最后两个擦除单位
#ifndef HTD_BOOT_CONFIG_ADDR
    #define HTD_BOOT_CONFIG_ADDR            0x8008000
#endif
#ifndef HTD_BOOT_CONFIG_SIZE
    #define HTD_BOOT_CONFIG_SIZE            0x8000
#endif
// 主程序区地址和大小
#ifndef HTD_BOOT_BANKA_ADDR
    #define HTD_BOOT_BANKA_ADDR             0x8010000
#endif
#ifndef HTD_BOOT_BANKA_SIZE
    #define HTD_BOOT_BANKA_SIZE             0x20000
#endif
// 备份区地址和大小
#ifndef HTD_BOOT_BANKB_ADDR
    #define HTD_BOOT_BANKB_ADDR             0x8030000
#endif
#ifndef HTD_BOOT_BANKB_SIZE
    #define HTD_BOOT_BANKB_SIZE             0x20000
#endif

// 每次拷贝数据的单位，必须为bankA和bankB擦除大小的公约数
#ifndef HTD_BOOT_OVERWRITE_UNIT
    #define HTD_BOOT_OVERWRITE_UNIT         256
#endif
// 配置信息大小, 单位为两个擦除页
#ifndef HTD_BOOTLOADER_CONFIG_UNIT
    #define HTD_BOOTLOADER_CONFIG_UNIT      2
#endif



//////////////////////////////////////////////////////////////////////
// 对接对应的接口
#ifndef ASSERT
    #define ASSERT(x)
#endif
#ifndef htd_memset
    #define htd_memset(ptr, v, n)           memset(ptr, v, n)
#endif
#ifndef htd_memcpy
    #define htd_memcpy(dst, src, n)         memcpy(dst, src, n)
#endif
#ifndef htd_memcmp
    #define htd_memcmp(v1, v2, n)           memcmp(v1, v2, n)
#endif
#ifndef htd_malloc
    #define htd_malloc(s)                   malloc(s)
#endif
#ifndef htd_free
    #define htd_free(p)                     free(p)
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
#ifndef BOOT_MIN
    #define BOOT_MIN(x,y)                   (((x)<(y))?(x):(y))
#endif
#ifndef BOOT_MAX
    #define BOOT_MAX(x,y)                   (((x)>(y))?(x):(y))
#endif
#ifndef IS_POWER_OF_TWO
    #define IS_POWER_OF_TWO(A)              (((A) != 0) && ((((A) - 1) & (A)) == 0))
#endif
#ifndef ALIGN_UP
    #define ALIGN_UP(n, align)              (((n) + (align) - 1) & ~((align) - 1))
#endif
#ifndef IS_ERASE_ADDR
    #define IS_ERASE_ADDR(addr, erase_unit) (0 == ((addr) & ((erase_unit) - 1)))
#endif
#ifndef ERASE_ADDR
    #define ERASE_ADDR(addr, erase_unit)    ((addr) & (~((erase_unit) - 1)))
#endif 

#ifndef HTD_BOOT_SECP256R1_PRIVATE_KEY
    #define HTD_BOOT_SECP256R1_PRIVATE_KEY  {0x1E, 0xEB, 0x22, 0xF8, 0x94, 0x37, 0x36, 0x91, 0xB0, 0x52, 0xFB, 0x09, 0xDB, 0x28, 0x5C, 0x08, \
                                             0x1D, 0x8F, 0x1B, 0x02, 0x9F, 0x23, 0x75, 0x61, 0xE7, 0xB3, 0x6B, 0x58, 0x2B, 0xED, 0xCE, 0x01,}
    #define HTD_BOOT_SECP256R1_PUBLIC_KEY   {0x1B, 0x00, 0x9F, 0xD8, 0xD6, 0x36, 0x63, 0x83, 0x19, 0xC5, 0xE5, 0x03, 0xA3, 0xE8, 0xFD, 0x68, \
                                             0xBE, 0xE9, 0x18, 0x59, 0x43, 0xDB, 0xBC, 0x07, 0x3F, 0x36, 0xF9, 0x80, 0xC8, 0xCA, 0x20, 0x28, \
                                             0xE7, 0xE1, 0x6B, 0x8D, 0x6D, 0x15, 0x0B, 0xA4, 0xBA, 0x85, 0x63, 0x31, 0x0E, 0xCF, 0xE2, 0x49, \
                                             0x7B, 0x91, 0xB9, 0xC7, 0xA5, 0x4F, 0xBA, 0x68, 0x94, 0xD5, 0xAD, 0x83, 0x2F, 0x9B, 0xA6, 0xDB,}
#endif /*HTD_BOOT_SECP256R1_PRIVATE_KEY*/




//////////////////////////////////////////////////////////////////////
// @brief 校验值
uint8_t boot_checksum_get(const uint8_t* p_buff, uint16_t buff_len);
// @brief 计算 CRC 校验和
uint16_t htd_ymodem_crc16(const uint8_t* p_data, uint32_t data_len);


// @brief 初始化 sha256;
void* boot_sha256_create(void);
// @brief 更新 sha256
void boot_sha256_update(void *p_ctx, const uint8_t* p_data, uint32_t data_len);
// @brief 获取 sha256 结果，同时销毁 p_ctx
void boot_sha256_final(void *p_ctx, uint8_t output[32]);


// @brief 通过私钥 p_skey 对 p_data 数值进行 ECDSA 签名
bool boot_secp256r1_sign(const uint8_t* p_skey, uint8_t const * p_data, uint32_t data_len, uint8_t* p_out_signa);
// @brief 通过公钥 p_pkey 校验 ECDSA 签名
bool boot_secp256r1_verify(uint8_t const *p_pkey, uint8_t const * p_data, uint32_t data_len, uint8_t const *p_signa);


// @brief 获取系统时间
uint32_t boot_start_ms_get(void);



//////////////////////////////////////////////////////////////////////
typedef struct {
    int32_t (*read_handle)(uint32_t addr, uint8_t* p_data, uint32_t data_len);
    int32_t (*write_handle)(uint32_t addr, const uint8_t* p_data, uint32_t data_len);
    int32_t (*erase_handle)(uint32_t addr, size_t size);
    uint32_t (*erase_unit_get_handle)(uint32_t addr);
}boot_flash_handle_t;
// @brief 获取flash操作句柄
const boot_flash_handle_t* boot_flash_handle_get(uint32_t addr);


// @brief 从 flash 中读取数据, 返回读取到的数据长度
int32_t boot_flash_read(const boot_flash_handle_t* p_flash, uint32_t addr, uint8_t* p_data, uint32_t data_len);
// @brief 写数据到flash中，返回写入的数据长度
int32_t boot_flash_write(const boot_flash_handle_t* p_flash, uint32_t addr, const uint8_t* p_data, uint32_t data_len);
// @brief 擦除一页数据, 成功返回0，失败返回对应的错误码
int32_t boot_flash_erase(const boot_flash_handle_t* p_flash, uint32_t addr, uint32_t size);
// @brief 获取擦除单位
uint32_t boot_flash_erase_unit_get(const boot_flash_handle_t* p_flash, uint32_t addr);






//////////////////////////////////////////////////////////////////////
typedef void* boot_serial_t;

// @brief 打开串口
boot_serial_t boot_serial_open(void);
// @brief 读取串口数据
int32_t boot_serial_read(boot_serial_t p_handle, uint8_t* p_data, uint32_t data_len, uint32_t timeout_ms);
// @brief 发送串口数据
int32_t boot_serial_write(boot_serial_t p_handle, const uint8_t* p_data, uint32_t data_len);
// @brief 关闭串口
void boot_serial_close(boot_serial_t p_handle);




#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_HTD_BOOT_PORT_H_*/

