// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL       4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG       "boot.port"
#include "htd_boot_port.h"
#include "htd_device.h"
#include "uECC.h"


//////////////////////////////////////////////////////////////////////
// @brief 生成随机值
static int boot_rng_generate(uint8_t *dest, unsigned size)
{
    return htd_rng_data_get(dest, size);
}

// @brief 校验值
uint8_t boot_checksum_get(const uint8_t* p_buff, uint16_t buff_len)
{
    uint16_t result;
    htd_crc8_ctx_t crc8_ctx = m_crc8_ctx_def;
    htd_crc8_init(&crc8_ctx);
    result = htd_crc8_update(&crc8_ctx, p_buff, buff_len);
    htd_crc8_final(&crc8_ctx);
    return result;
}

// @brief 计算 CRC 校验和
uint16_t htd_ymodem_crc16(const uint8_t* p_data, uint32_t data_len)
{
    htd_crc16_ctx_t crc16_ctx = m_crc16_ctx_xmodem_def;
    htd_crc16_init(&crc16_ctx);
    htd_crc16_update(&crc16_ctx, p_data, data_len);
    return htd_crc16_final(&crc16_ctx);
}

// @brief 初始化 sha256;
void* boot_sha256_create(void)
{
    htd_sha256_ctx_t* p_ctx = MALLOC(sizeof(htd_sha256_ctx_t));
    ASSERT(p_ctx);
    htd_sha256_init(p_ctx);
    return p_ctx;
}
// @brief 更新 sha256
void boot_sha256_update(void *p_ctx, const uint8_t* p_data, uint32_t data_len)
{
    htd_sha256_update((htd_sha256_ctx_t*)p_ctx, p_data, data_len);
}
// @brief 获取 sha256 结果，同时销毁 p_ctx
void boot_sha256_final(void *p_ctx, uint8_t output[32])
{
    htd_sha256_final((htd_sha256_ctx_t*)p_ctx, output);
    FREE(p_ctx);
}

// @brief 通过私钥 p_skey 对 p_data 数值进行 ECDSA 签名
bool boot_secp256r1_sign(const uint8_t* p_skey, uint8_t const * p_data, uint32_t data_len, uint8_t* p_out_signa)
{
    uECC_Curve curve = uECC_secp256r1();
    uECC_set_rng(boot_rng_generate);
    return (uECC_sign(p_skey, p_data, data_len, p_out_signa, curve) == 1) ? true : false;
}

// @brief 通过公钥 p_pkey 校验 ECDSA 签名
bool boot_secp256r1_verify(uint8_t const *p_pkey, uint8_t const * p_data, uint32_t data_len, uint8_t const *p_signa)
{
    uECC_Curve curve = uECC_secp256r1();
    uECC_set_rng(boot_rng_generate);
    return (uECC_verify(p_pkey, p_data, data_len, p_signa, curve) == 1) ? true : false;
}

// @brief 获取系统时间
uint32_t boot_start_ms_get(void)
{
    return htd_time_ms_get();
}


//////////////////////////////////////////////////////////////////////
#if defined(_WIN32)
// @brief 擦除单位
uint32_t boot_norflash_erase_unit_get(uint32_t addr)
{
    return htd_dev_norflash_erase_unit_get();
}
#endif

// @brief 打开
const boot_flash_handle_t* boot_flash_handle_get(uint32_t addr)
{
    // 内部 flash
    if (htd_inflash_bank_is(addr, addr+1)) {
        const static boot_flash_handle_t m_inflash_handle = {
            .read_handle = htd_inflash_read,
            .write_handle = htd_inflash_write,
            .erase_handle = htd_inflash_erase,
            .erase_unit_get_handle = htd_inflash_erase_unit_get,
        };
        return &m_inflash_handle;
    }

#if defined(_WIN32)
    // norflash
    uint32_t start = htd_dev_norflash_phy_saddr_get();
    uint32_t end = htd_dev_norflash_phy_eaddr_get();
    if (start <= addr && addr < end) {
        const static boot_flash_handle_t m_norflash_handle = {
            .read_handle = htd_dev_norflash_read,
            .write_handle = htd_dev_norflash_write,
            .erase_handle = htd_dev_norflash_erase,
            .erase_unit_get_handle = boot_norflash_erase_unit_get,
        };
        return &m_norflash_handle;
    }
#endif

    boot_error("addr=0x%08x", addr);
    ASSERT(0);
    return NULL;
}

// @brief 从 flash 中读取数据, 返回读取到的数据长度
int32_t boot_flash_read(const boot_flash_handle_t* p_flash, uint32_t addr, uint8_t* p_data, uint32_t data_len)
{
    ASSERT(p_flash);
    ASSERT(p_data && data_len > 0);
    return p_flash->read_handle(addr, p_data, data_len);
}

// @brief 写数据到flash中，返回写入的数据长度
int32_t boot_flash_write(const boot_flash_handle_t* p_flash, uint32_t addr, const uint8_t* p_data, uint32_t data_len)
{
    ASSERT(p_flash);
    ASSERT(p_data && data_len > 0);
    return p_flash->write_handle(addr, p_data, data_len);
}

// @brief 擦除一页数据, 成功返回0，失败返回对应的错误码
int32_t boot_flash_erase(const boot_flash_handle_t* p_flash, uint32_t addr, uint32_t size)
{
    ASSERT(p_flash);
    uint32_t erase_unit = p_flash->erase_unit_get_handle(addr);
    ASSERT(erase_unit);
    if (IS_ERASE_ADDR(addr, erase_unit) == false) {
        return -ERROR_CODE_INVALID_ADDR;
    }
    size = ALIGN_UP(size, erase_unit);
    return p_flash->erase_handle(addr, size);
}

// @brief 获取擦除单位
uint32_t boot_flash_erase_unit_get(const boot_flash_handle_t* p_flash, uint32_t addr)
{
    ASSERT(p_flash);
    return p_flash->erase_unit_get_handle(addr);
}


//////////////////////////////////////////////////////////////////////
#if defined(HTD_BOOT_SUPPORT_SERIAL)
// @brief 打开串口
boot_serial_t boot_serial_open(void)
{
    // 波特率探测，探测不到返回NULL
    boot_warning("open serial. name=%s(%d)", HTD_BOOT_SERIAL_NAME, HTD_BOOT_SERIAL_BAUDRATE);
    static const htd_serial_param_t param = HTD_SERIAL_CONFIG_DEF(HTD_BOOT_SERIAL_TX_PINNUM, HTD_BOOT_SERIAL_RX_PINNUM, HTD_BOOT_SERIAL_BAUDRATE, NULL);
    return htd_serial_open(HTD_BOOT_SERIAL_NAME, &param);
}

// @brief 读取串口数据
int32_t boot_serial_read(boot_serial_t p_handle, uint8_t* p_data, uint32_t data_len, uint32_t timeout_ms)
{
    return htd_serial_read((htd_serial_handle_t*)p_handle, p_data, data_len, timeout_ms);
}

// @brief 发送串口数据
int32_t boot_serial_write(boot_serial_t p_handle, const uint8_t* p_data, uint32_t data_len)
{
    return htd_serial_write((htd_serial_handle_t*)p_handle, p_data, data_len);
}

// @brief 关闭串口
void boot_serial_close(boot_serial_t p_handle)
{
    htd_serial_close(p_handle);
}
#endif

