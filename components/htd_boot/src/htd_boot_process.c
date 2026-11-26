// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL       4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG       "boot"
#include "htd_boot_process.h"
#include "htd_boot_overwrite_swap.h"
#include "htd_boot_decompress.h"


//////////////////////////////////////////////////////////////////////
static const uint8_t m_boot_public_key[] = HTD_BOOT_SECP256R1_PUBLIC_KEY;
static htd_boot_env_t m_boot_env_info = {
    .bankA_addr = HTD_BOOT_BANKA_ADDR,
    .bankA_size = HTD_BOOT_BANKA_SIZE,
    .bankB_addr = HTD_BOOT_BANKB_ADDR,
    .bankB_size = HTD_BOOT_BANKB_SIZE,
};

// @brief 获取环境信息
htd_boot_env_t* htd_boot_env_info_get(void)
{
    return &m_boot_env_info;
}


//////////////////////////////////////////////////////////////////////
// @brief 销毁 boot 对象
static void boot_object_destroy(htd_boot_object_t* p_object)
{
    if (p_object) {
        htd_free(p_object);
    }
}

// @brief 检测擦除单位是否一致
bool boot_assert_erase_unit_is_consistent(const boot_flash_handle_t* p_flash, uint32_t addr, uint32_t size)
{
    uint32_t erase_unit = boot_flash_erase_unit_get(p_flash, addr);
    do {
        addr += erase_unit;
        if (size <= erase_unit) {
            break;
        }
        size -= erase_unit;
        if (erase_unit != boot_flash_erase_unit_get(p_flash, addr)) {
            return false;
        }
    } while (1);
    return true;
}

// @brief 计算交换单位
uint32_t boot_swap_unit_get(void)
{
    htd_boot_env_t* p_env = htd_boot_env_info_get();
    const boot_flash_handle_t* p_flash = boot_flash_handle_get(p_env->bankB_addr);
    uint32_t erase_unit2 = boot_flash_erase_unit_get(p_flash, p_env->bankB_addr);

    uint32_t offset = 0;
    uint32_t erase_unit;
    uint32_t erase_unit_max = 0;
    uint32_t erase_unit_min = 0xffffffff;
    p_flash = boot_flash_handle_get(p_env->bankA_addr);
    while (offset < p_env->bankA_size) {
        erase_unit = boot_flash_erase_unit_get(p_flash, p_env->bankA_addr + offset);
        if (erase_unit > erase_unit_max) {
            erase_unit_max = erase_unit;
        }
        if (erase_unit < erase_unit_min) {
            erase_unit_min = erase_unit;
        }
        offset += erase_unit;
        if ((erase_unit % erase_unit2) != 0) {
            // 主分区每个擦除单位都必须能被备份区的擦除单位整除
            print_error("erase_unit=0x%08x, erase_unit2=0x%08x", erase_unit, erase_unit2);
            ASSERT(0);
        } 
    }
    return erase_unit_max;
}

// @brief 初始化boot对象
static htd_boot_object_t* boot_object_create(void)
{
    htd_boot_object_t *p_object = htd_malloc(sizeof(htd_boot_object_t));
    ASSERT(p_object);
    htd_memset(p_object, 0, sizeof(htd_boot_object_t));

    // 初始化配置信息
    if (!boot_config_init()) {
        print_error("boot config init failed.");
        htd_free(p_object);
        ASSERT(0);
        return NULL;
    }

    htd_boot_env_t* p_env = htd_boot_env_info_get();
    p_object->fwB_download_addr = 0xffffffff;
    p_object->p_flash_fwA = boot_flash_handle_get(p_env->bankA_addr);
    p_object->p_flash_fwB = boot_flash_handle_get(p_env->bankB_addr);
    p_object->p_env = p_env;

#if defined(HTD_BOOT_OVERWRITE_SWAP)
    // 备份区的擦除地址必须一致
    ASSERT(boot_assert_erase_unit_is_consistent(p_object->p_flash_fwB, p_env->bankB_addr, p_env->bankB_size));
    p_object->fwB_offset = boot_swap_unit_get();
#endif

    // 提取信息
    boot_header_t image_hdr;
    boot_flash_read(p_object->p_flash_fwA, p_env->bankA_addr, (uint8_t*)&image_hdr, sizeof(boot_header_t));
    if (image_hdr.magic == HTD_BOOT_MAGIC) {
        p_object->version = (image_hdr.ver_major<<24)|(image_hdr.ver_minor<<16)|(image_hdr.ver_revision<<8)|image_hdr.ver_reserve;
        p_object->fwA_size = image_hdr.fw_size;
        p_object->image_hdr_len = image_hdr.fw_hdr_size;
    }
    print_debug("slotA addr=0x%08x, slotB addr=0x%08x", p_env->bankA_addr, p_env->bankB_addr);
    print_debug("fw_size=0x%08x", image_hdr.fw_size);
    return p_object;
}

// @brief 固件验证
static bool boot_image_fw_verify(htd_boot_object_t* p_object, boot_header_t* p_hdr, const boot_flash_handle_t* p_flash, uint32_t image_addr)
{
    // 计算哈希值
    uint32_t payload_addr = image_addr + (HTD_BOOT_VERIFY_CODE_OFFSET + sizeof(p_hdr->verify_code));
    uint32_t payload_len = p_hdr->fw_size - (HTD_BOOT_VERIFY_CODE_OFFSET + sizeof(p_hdr->verify_code));
    print_debug("image_addr=0x%08x, payload=0x%08x, payload_len=0x%08x, fw_size=%u", image_addr, payload_addr, payload_len, p_hdr->fw_size);

    uint32_t read_len;
    uint32_t data_len = sizeof(p_object->swap_data);
    void *p_sha256_ctx = boot_sha256_create();
    while (payload_len) {
        if (payload_len < data_len) {
            data_len = payload_len;
        }
        read_len = boot_flash_read(p_flash, payload_addr, p_object->swap_data, data_len);
        ASSERT(read_len == data_len);
        boot_sha256_update(p_sha256_ctx, p_object->swap_data, read_len);
        payload_len -= data_len;
        payload_addr += data_len;
    }
    boot_sha256_final(p_sha256_ctx, p_object->swap_data);
    
    // ecdsa 签名验证
    return boot_secp256r1_verify(m_boot_public_key, p_object->swap_data, 32, p_hdr->verify_code);
}

// @brief 查找下载地址
static uint32_t boot_slotB_find_download_addr(htd_boot_object_t *p_object)
{
    // 全量升级放在 slotB_addr 后面偏移一个单位
    boot_header_t image_hdr;
    htd_boot_env_t* p_env = p_object->p_env;
    if (boot_config_update_type_get() == BOOT_UPDATE_TYPE_FULL) {
        boot_flash_read(p_object->p_flash_fwB, p_env->bankB_addr + p_object->fwB_offset, (uint8_t*)&image_hdr, sizeof(boot_header_t));
        if (image_hdr.magic == HTD_BOOT_MAGIC) {
            return p_env->bankB_addr + p_object->fwB_offset;
        }
        return 0xffffffff;
    }
    
    // 其他的升级方式放在最后面
    uint32_t erase_unit;
    uint32_t offset = p_env->bankB_size;
    while (offset) {
        erase_unit = boot_flash_erase_unit_get(p_object->p_flash_fwB, p_env->bankB_addr+offset-1);
        offset -= erase_unit;
        boot_flash_read(p_object->p_flash_fwB, p_env->bankB_addr + offset, (uint8_t*)&image_hdr, sizeof(boot_header_t));
        if (image_hdr.magic == HTD_BOOT_MAGIC) {
            return p_env->bankB_addr + offset;
        }
    }
    return 0xffffffff;
}

// @brief 校验 A 区固件是否正确
static bool boot_slotA_image_check(htd_boot_object_t *p_object)
{
    boot_header_t image_hdr;
    htd_boot_env_t* p_env = p_object->p_env;
    boot_flash_read(p_object->p_flash_fwA, p_env->bankA_addr, (uint8_t*)&image_hdr, sizeof(boot_header_t));
    if (image_hdr.magic != HTD_BOOT_MAGIC || image_hdr.fw_size > p_env->bankA_size) {
        return false;
    }
    
    // 固件验证
    if (!boot_image_fw_verify(p_object, &image_hdr, p_object->p_flash_fwA, p_env->bankA_addr)) {
        print_error("imageA verify failed. bankA_addr=0x%08x", p_env->bankA_addr);
        return false;
    }
    
    p_object->version = (image_hdr.ver_major<<24)|(image_hdr.ver_minor<<16)|(image_hdr.ver_revision<<8)|image_hdr.ver_reserve;
    p_object->fwA_size = image_hdr.fw_size;
    p_object->image_hdr_len = image_hdr.fw_hdr_size;
    return true;
}

// @brief 校验 B 区固件是否正确
static bool boot_slotB_image_check(htd_boot_object_t *p_object)
{
    htd_boot_env_t* p_env = p_object->p_env;
    if (p_object->fwB_download_addr < p_env->bankB_addr || 
        p_object->fwB_download_addr >= p_env->bankB_addr + p_env->bankB_size) {
        return false;
    }
    
    boot_header_t image_hdr;
    boot_flash_read(p_object->p_flash_fwB, p_object->fwB_download_addr, (uint8_t*)&image_hdr, sizeof(boot_header_t));
    if (image_hdr.magic != HTD_BOOT_MAGIC || image_hdr.fw_size > p_env->bankB_size) {
        return false;
    }
    print_debug("fw download addr: 0x%08x", p_object->fwB_download_addr);
    
    // 固件验证
    if (!boot_image_fw_verify(p_object, &image_hdr, p_object->p_flash_fwB, p_object->fwB_download_addr)) {
        print_error("image verify failed. fwb_download_addr=0x%08x", p_object->fwB_download_addr);
        return false;
    }
    p_object->fwB_size = image_hdr.fw_size;
    
    // 版本号, 只有大于才升级
    uint32_t ver = (image_hdr.ver_major<<24)|(image_hdr.ver_minor<<16)|(image_hdr.ver_revision<<8)|image_hdr.ver_reserve;
    if (ver <= p_object->version) {
        print_error("version check failed. ver=0x%08x->0x%08x", p_object->version, ver);
        return false;
    }
    print_warning("update version: 0x%08x->0x%08x. update_type=%d", p_object->version, ver, image_hdr.update_type);
    
    // 设置升级方式
    boot_config_update_type_set(image_hdr.update_type);
    return true;
}

// @brief 交换状态下重置固件信息
static bool boot_image_info_reset_in_swap(htd_boot_object_t *p_object)
{
    // 交换状态下，A 区头部信息已经存储在 B 区
    boot_header_t image_hdr;
    htd_boot_env_t* p_env = p_object->p_env;
    boot_flash_read(p_object->p_flash_fwB, p_env->bankB_addr, (uint8_t*)&image_hdr, sizeof(boot_header_t));
    if (image_hdr.magic != HTD_BOOT_MAGIC) {
        return false;
    }
    p_object->version = (image_hdr.ver_major<<24)|(image_hdr.ver_minor<<16)|(image_hdr.ver_revision<<8)|image_hdr.ver_reserve;
    p_object->fwA_size = image_hdr.fw_size;
    p_object->image_hdr_len = image_hdr.fw_hdr_size;
    // B 区头部信息已经存储在 A 区，或者在原有位置
    boot_flash_read(p_object->p_flash_fwA, p_env->bankA_addr, (uint8_t*)&image_hdr, sizeof(boot_header_t));
    if (image_hdr.magic != HTD_BOOT_MAGIC) {
        boot_flash_read(p_object->p_flash_fwB, p_env->bankB_addr+p_object->fwB_offset, (uint8_t*)&image_hdr, sizeof(boot_header_t));
        if (image_hdr.magic != HTD_BOOT_MAGIC) {
            return false;
        }
    }
    p_object->fwB_size = image_hdr.fw_size;
    return true;
}

// @brief 回滚状态下重置固件信息
bool boot_image_info_reset_in_rollback(htd_boot_object_t *p_object)
{
    // 回滚状态下，A/B 区头部信息存储在 B 区
    boot_header_t image_hdr;
    htd_boot_env_t* p_env = p_object->p_env;
    boot_flash_read(p_object->p_flash_fwB, p_env->bankB_addr, (uint8_t*)&image_hdr, sizeof(boot_header_t));
    if (image_hdr.magic != HTD_BOOT_MAGIC) {
        return false;
    }
    // 固件验证
    if (!boot_image_fw_verify(p_object, &image_hdr, p_object->p_flash_fwB, p_env->bankB_addr)) {
        print_error("image verify failed. bankB_addr=0x%08x", p_env->bankB_addr);
        return false;
    }
    
    p_object->fwA_size = image_hdr.fw_size;
    p_object->fwB_size = image_hdr.fw_size;
    p_object->image_hdr_len = image_hdr.fw_hdr_size;
    p_object->version = (image_hdr.ver_major<<24)|(image_hdr.ver_minor<<16)|(image_hdr.ver_revision<<8)|image_hdr.ver_reserve;
    return true;
}

// @brief 固件下载完成处理方式
static void boot_image_download_finish_opt(htd_boot_object_t *p_object)
{
    p_object->fwB_download_addr = boot_slotB_find_download_addr(p_object);
    if (!boot_slotB_image_check(p_object)) {
        print_error("ImageB check failed.");
        return;
    }
    print_warning("ImageB check succuss. start swap. fwB_size=%u", p_object->fwB_size);
    
    // 全量升级
    boot_update_type_t update_type = boot_config_update_type_get();
    if (update_type == BOOT_UPDATE_TYPE_FULL) {
        boot_overwrite(p_object);
        return;
    } 

    // 压缩升级
#if defined(HTD_BOOT_SUPPORT_COMPRESS_QUICKLZ) || defined(HTD_BOOT_SUPPORT_COMPRESS_FASTLZ)
    if (update_type == BOOT_UPDATE_TYPE_COMPRESS_QUICKLZ ||
        update_type == BOOT_UPDATE_TYPE_COMPRESS_FASTLZ) {
        // 解压
        boot_config_offset_state_set(0, BOOT_STATE_DECOMPRESS); 
        boot_decompress(p_object);
        // 交换
        boot_config_update_type_set(BOOT_UPDATE_TYPE_FULL);
        boot_config_offset_state_set(0, BOOT_STATE_DOWNLOAD_FINISH); 
        if (boot_slotB_image_check(p_object)) {
            boot_overwrite(p_object);
        } else {
            print_error("ImageB check failed.");
        }
        return;
    } 
#endif 

    boot_config_offset_state_set(0, BOOT_STATE_IMAGE_OK);
    return;
}

// @brief boot 主流程
bool htd_boot_process(uint32_t* program_addr)
{
    htd_boot_object_t *p_object = boot_object_create();
    ASSERT(p_object);

    // 判断是否要升级
    bool ret = false;
    bool is_verify = true;
    boot_state_t boot_state = boot_config_state_get();
    print_warning("boot_state=%d", boot_state);
    switch (boot_state) {
        case BOOT_STATE_DOWNLOAD_FINISH: {
            ret = true;
            boot_image_download_finish_opt(p_object);
            break;
        }
        case BOOT_STATE_DECOMPRESS: {
            // 继续解压
            if (boot_decompress(p_object)) {
                // 处理交换覆盖
                boot_config_update_type_set(BOOT_UPDATE_TYPE_FULL);
                boot_config_offset_state_set(0, BOOT_STATE_DOWNLOAD_FINISH); 
                boot_image_download_finish_opt(p_object);
                ret = true;
            }
            break;
        }
        case BOOT_STATE_SWAP:
        case BOOT_STATE_SWAP_A2B: {
            // 提取 A/B 两区固件信息
            if (boot_image_info_reset_in_swap(p_object)) {
                // 继续交换
                boot_overwrite(p_object);
                ret = true;
            }
            break;
        }
    #if defined(HTD_BOOT_OVERWRITE_SWAP)
        case BOOT_STATE_ROLLBACK: {
            // 提取 A/B 两区固件信息
            if (boot_image_info_reset_in_rollback(p_object)) {
                // 继续回滚
                boot_rollback(p_object);
                ret = true;
            }
            break;
        }
        case BOOT_STATE_IMAGE_ERROR: {
            // 应用层没有重置为 OK，说明新的固件有问题
            if (boot_config_offset_get() > 5) {
                // 提取 A/B 两区固件信息
                if (boot_image_info_reset_in_rollback(p_object)) {
                    // 回滚
                    boot_rollback(p_object);
                    ret = true;
                }
            } else {
                ret = true;
            }
            break;
        }
    #endif
        default : {
            is_verify = false;
            ret = true;
            break;
        }
    }
    
    if (ret) {
        if (boot_slotA_image_check(p_object)) {
            print_warning("imageA check success. version=0x%08x", p_object->version);
            if (program_addr) {
                print_debug("bankA_addr=0x%08x, image_hdr_len=%d", p_object->p_env->bankA_addr, p_object->image_hdr_len);
                *program_addr = p_object->p_env->bankA_addr + p_object->image_hdr_len;
            }
            // 写入固件错误标记，程序运行起来后，写入 OK 标记
            if (is_verify) {
            #if defined(HTD_BOOT_OVERWRITE_SWAP)
                boot_state = (boot_config_state_get() == BOOT_STATE_ROLLBACK_FINISH) ? BOOT_STATE_IMAGE_OK : BOOT_STATE_IMAGE_ERROR;
            #else
                boot_state = BOOT_STATE_IMAGE_OK;
            #endif
                boot_config_offset_state_set(boot_config_offset_get() + 1, boot_state);
            }
        } else {
            print_warning("imageA check failed. fwA_size=%u", p_object->fwA_size);
            ret = false;
        }
    } else {
        print_error("boot_state=%d", boot_state);
    }
    
    boot_object_destroy(p_object);
    return ret;
}

// @brief 获取全量升级下载地址
uint32_t htd_boot_full_download_addr_get(void)
{
    htd_boot_env_t* p_env = htd_boot_env_info_get();
#if defined(HTD_BOOT_OVERWRITE_SWAP)
    uint32_t offset = boot_swap_unit_get();
    return p_env->bankB_addr + offset;
#else
    return p_env->bankB_addr;
#endif
}

// @brief 获取压缩升级下载地址
uint32_t htd_boot_lz_download_addr_get(uint32_t filesize)
{
    htd_boot_env_t* p_env = htd_boot_env_info_get();
    // 其他的升级方式放在最后面
    uint32_t erase_unit;
    uint32_t offset = p_env->bankB_size;
    const boot_flash_handle_t* p_flash = boot_flash_handle_get(p_env->bankB_addr);
    while (offset) {
        erase_unit = boot_flash_erase_unit_get(p_flash, p_env->bankB_addr+offset-1);
        offset -= erase_unit;
        if (filesize <= erase_unit) {
            return p_env->bankB_addr + offset;
        }
        filesize -= erase_unit;
    }
    return p_env->bankB_addr;
}

//////////////////////////////////////////////////////////////////////
#if defined(HTD_BOOT_SUPPORT_SERIAL) && defined(HTD_BOOT_SUPPORT_SERIAL_YMODEM)
#include "htd_device.h"
static ymodem_ctx_t* m_ymodem_ctx = NULL;
static boot_serial_t m_ymodem_serial_handle = NULL;
const static boot_flash_handle_t* m_ymodem_flash = NULL;
static uint32_t m_ymodem_start_ms;
static uint32_t m_download_addr;


// @brief 发送数据
static int32_t ymodem_serial_read(uint8_t* p_data, int32_t data_len, uint32_t timeout_ms)
{
    ASSERT(m_ymodem_serial_handle);
    return boot_serial_read(m_ymodem_serial_handle, p_data, data_len, timeout_ms);
}

// @brief 接收数据
static int32_t ymodem_serial_write(const uint8_t* p_data, int32_t data_len)
{
    ASSERT(m_ymodem_serial_handle);
    return boot_serial_write(m_ymodem_serial_handle, p_data, data_len);
}

// @brief 事件通告
static int32_t ymodem_evt_notify(ymodem_evt_t evt, const uint8_t* p_param, uint32_t param_len)
{
    if (evt == YMODEM_EVT_START) {
        uint32_t filesize = 0;
        const char* p_filename = htd_ymodem_fileinfo_get(p_param, param_len, &filesize);
        print_debug("filename=%s, filesize=%u", p_filename, filesize);
        const char* p_postfix = strchr(p_filename, '.');
        htd_boot_env_t* p_env = htd_boot_env_info_get();
    #if defined(HTD_BOOT_IN_BOOTLOADER)
        if (strcmp(p_postfix, ".bin") == 0) {
            if (filesize > p_env->bankA_size) {
                return -1;
            }
            print_warning("bankA_addr=0x%08x, bankA_size=0x%08x", p_env->bankA_addr, p_env->bankA_size);
            // 写到A区
            m_download_addr = p_env->bankA_addr;
            m_ymodem_flash = boot_flash_handle_get(m_download_addr);
            m_ymodem_ctx->update_type = BOOT_UPDATE_TYPE_NONE;
            return 0;
        } 
    #endif 
        // 普通升级
        if (strcmp(p_postfix, ".ota") == 0) {
            if (filesize > p_env->bankB_size) {
                return -1;
            }
            // 写到B区
            m_download_addr = htd_boot_full_download_addr_get();
            print_warning("ota. m_download_addr=0x%08x", m_download_addr);
            m_ymodem_flash = boot_flash_handle_get(m_download_addr);
            m_ymodem_ctx->update_type = BOOT_UPDATE_TYPE_FULL;
            return 0;
        }
        // 压缩升级，flz
        if (strcmp(p_postfix, ".flz") == 0) {
            if (filesize > p_env->bankB_size) {
                return -1;
            }
            m_download_addr = htd_boot_lz_download_addr_get(filesize);
            print_warning("ota flz. m_download_addr=0x%08x", m_download_addr);
            m_ymodem_flash = boot_flash_handle_get(m_download_addr);
            m_ymodem_ctx->update_type = BOOT_UPDATE_TYPE_COMPRESS_FASTLZ;
            return 0;
        }
        // 压缩升级，qlz
        if (strcmp(p_postfix, ".qlz") == 0) {
            if (filesize > p_env->bankB_size) {
                return -1;
            }
            m_download_addr = htd_boot_lz_download_addr_get(filesize);
            print_warning("ota qlz. m_download_addr=0x%08x", m_download_addr);
            m_ymodem_flash = boot_flash_handle_get(m_download_addr);
            m_ymodem_ctx->update_type = BOOT_UPDATE_TYPE_COMPRESS_QUICKLZ;
            return 0;
        }
        return -1;
    } 

    if (evt == YMODEM_EVT_RECV_DATA) {
        ASSERT(m_ymodem_flash);
        boot_flash_erase(m_ymodem_flash, m_download_addr, param_len);
        boot_flash_write(m_ymodem_flash, m_download_addr, p_param, param_len);
        m_download_addr += param_len;
        
    } else if (evt == YMODEM_EVT_FINISH) {
        m_ymodem_flash = NULL;
    }
    return 0;
}

// @brief 下载固件流程, while 循环不断调用，返回1则代表下载完成, 返回2代表超时了, 3代表下载的是升级文件
uint32_t htd_boot_download_process(void)
{
    if (m_ymodem_ctx == NULL) {
        m_ymodem_serial_handle = boot_serial_open();
        ASSERT(m_ymodem_serial_handle);
        m_ymodem_ctx = htd_ymodem_create(ymodem_evt_notify, ymodem_serial_read, ymodem_serial_write);
        ASSERT(m_ymodem_ctx);
        m_ymodem_start_ms = boot_start_ms_get();
    }

    uint32_t ret = 1;
    if (htd_ymodem_process(m_ymodem_ctx, 100)) {
        if (htd_ymodem_is_linked(m_ymodem_ctx)) {
            m_ymodem_start_ms = boot_start_ms_get();
            return 0;
        } 
        if (((boot_start_ms_get() - m_ymodem_start_ms) & 0xffffffff) < 60*1000) {
            return 0;
        }
        ret = 2;
    } else {
        if (m_ymodem_ctx->update_type != BOOT_UPDATE_TYPE_NONE) {
            boot_state_update_in_application(m_ymodem_ctx->update_type, BOOT_STATE_DOWNLOAD_FINISH);
        }
    }
    
    htd_ymodem_destroy(m_ymodem_ctx);
    m_ymodem_ctx = NULL;
    boot_serial_close(m_ymodem_serial_handle);
    m_ymodem_serial_handle = NULL;
    return ret;
}
#endif /*defined(HTD_BOOT_SUPPORT_SERIAL) && defined(HTD_BOOT_SUPPORT_SERIAL_YMODEM)*/




//////////////////////////////////////////////////////////////////////
#if defined(HTD_CONSOLE_ENABLE) && defined(DEBUG_LVL) && (defined(_WIN32))
#include "htd_components.h"
#include "quicklz.h"
#include "fastlz.h"

// @brief 验证签名
static bool boot_image_ecdsa_sign(const uint8_t* p_data, uint32_t data_len, uint8_t* sign_code)
{
    const static uint8_t m_boot_private_key[] = HTD_BOOT_SECP256R1_PRIVATE_KEY;
    return boot_secp256r1_sign(m_boot_private_key, p_data, data_len, sign_code);
}

// @brief quicklz 压缩
static uint8_t* boot_compress_quicklz(const uint8_t* p_data, uint32_t data_len, uint32_t* out_len)
{
    uint32_t compress_len;
    uint8_t* p_out = (uint8_t*)htd_malloc(data_len);
    ASSERT(p_out);
    htd_memset(p_out, 0, data_len);

    qlz_state_compress *state_compress = (qlz_state_compress*)htd_malloc(sizeof(qlz_state_compress));
    ASSERT(state_compress);
    htd_memset(state_compress, 0, sizeof(qlz_state_compress));

    print_debug("[qlz]compress start : ");
    uint32_t data_size = data_len;
    char* p_ptr = (char*)p_out;
    uint32_t block_size = BOOT_DECOMPRESS_BUFFER_SIZE;
    while (data_size) {
        if (data_size < block_size) {
            block_size = data_size;
        } 
        // 压缩数据
        compress_len = qlz_compress(p_data, (char*)(p_ptr+sizeof(uint32_t)), block_size, state_compress);
        p_data += block_size;
        data_size -= block_size;
        // 保存数据长度
        htd_memcpy(p_ptr, &compress_len, sizeof(uint32_t));
        p_ptr += compress_len + sizeof(uint32_t);
    }
    compress_len = p_ptr - p_out;
    print_debug("[qlz]compressed %d bytes into %d bytes, compression ratio is %d%!", data_len, compress_len, (compress_len * 100) / data_len);
    htd_free(state_compress);

    if (out_len) {
        *out_len = compress_len;
    }
    return p_out;
}

// @brief fastlz 压缩
static uint8_t* boot_compress_fastlz(const uint8_t* p_data, uint32_t data_len, uint32_t* out_len)
{
    uint32_t compress_len;
    uint8_t* p_out = (uint8_t*)htd_malloc(data_len);
    ASSERT(p_out);
    htd_memset(p_out, 0, data_len);

    print_debug("[flz]compress start: ");
    uint32_t data_size = data_len;
    char* p_ptr = (char*)p_out;
    uint32_t block_size = BOOT_DECOMPRESS_BUFFER_SIZE;
    while (data_size) {
        if (data_size < block_size) {
            block_size = data_size;
        } 
        // 压缩数据
        compress_len = fastlz_compress_level(1, p_data, block_size, p_ptr+sizeof(uint32_t));
        p_data += block_size;
        data_size -= block_size;
        // 保存数据长度
        htd_memcpy(p_ptr, &compress_len, sizeof(uint32_t));
        p_ptr += compress_len + sizeof(uint32_t);
    }
    compress_len = p_ptr - p_out;
    print_debug("[qlz]compressed %d bytes into %d bytes, compression ratio is %d%!", data_len, compress_len, (compress_len * 100) / data_len);
    
    if (out_len) {
        *out_len = compress_len;
    }
    return p_out;
}

// @brief 生成完整压缩升级固件
void htd_boot_fw_compress_make(boot_update_type_t update_type)
{
    uint32_t temp;
    boot_firmware_header_t *fw_head;
    // HTD_BOOT_SLOT_A_ADDR 这里写版本A，数据内容全为A
    uint32_t fw_size = 32*1024 + 3;
    htd_boot_env_t* p_env = htd_boot_env_info_get();
    const boot_flash_handle_t* p_flash = boot_flash_handle_get(p_env->bankA_addr);
    uint8_t* p_fw_data = htd_malloc(fw_size + sizeof(boot_firmware_header_t));
    ASSERT(p_fw_data);
    fw_head = (boot_firmware_header_t*)p_fw_data;
    p_fw_data += sizeof(boot_firmware_header_t);
    htd_memset(p_fw_data, 'A', fw_size);
    // 写入头部数据
    htd_memset(fw_head, 0, sizeof(boot_firmware_header_t));
    fw_head->boot_hdr.magic = HTD_BOOT_MAGIC;
    fw_head->boot_hdr.fw_hdr_size = sizeof(boot_firmware_header_t);
    fw_head->boot_hdr.update_type = BOOT_UPDATE_TYPE_FULL;
    fw_head->boot_hdr.fw_size = fw_size + sizeof(boot_firmware_header_t);
    fw_head->boot_hdr.ver_major = 1;
    // 计算固件 sha256
    uint8_t sha256_data[32];
    void* p_ctx = boot_sha256_create();
    temp = HTD_BOOT_VERIFY_CODE_OFFSET + sizeof(fw_head->boot_hdr.verify_code);
    boot_sha256_update(p_ctx, ((uint8_t*)fw_head) + temp, sizeof(boot_firmware_header_t) - temp);
    boot_sha256_update(p_ctx, p_fw_data, fw_size);
    boot_sha256_final(p_ctx, sha256_data);
    // 填写签名验证码
    boot_image_ecdsa_sign(sha256_data, sizeof(sha256_data), fw_head->boot_hdr.verify_code);
    // 写入
    boot_flash_erase(p_flash, p_env->bankA_addr, fw_size+4096);
    boot_flash_write(p_flash, p_env->bankA_addr, (uint8_t*)fw_head, sizeof(boot_firmware_header_t)+fw_size);
    htd_free(fw_head);
    print_debug("ImageA addr: 0x%08x, fw_size=0x%08x", p_env->bankA_addr, fw_size);
    
    // p_env->slotB_addr 这里写版本B，数据内容全为B
    fw_size = 34*1024 + 120;
    p_flash = boot_flash_handle_get(p_env->bankB_addr);
    p_fw_data = htd_malloc(fw_size + sizeof(boot_firmware_header_t));
    ASSERT(p_fw_data);
    fw_head = (boot_firmware_header_t*)p_fw_data;
    p_fw_data += sizeof(boot_firmware_header_t);
    htd_memset(p_fw_data, 'B', fw_size);
    // 写入头部数据
    htd_memset(fw_head, 0, sizeof(boot_firmware_header_t));
    fw_head->boot_hdr.magic = HTD_BOOT_MAGIC;
    fw_head->boot_hdr.fw_hdr_size = sizeof(boot_firmware_header_t);
    fw_head->boot_hdr.update_type = BOOT_UPDATE_TYPE_FULL;
    fw_head->boot_hdr.fw_size = fw_size + sizeof(boot_firmware_header_t);
    fw_head->boot_hdr.ver_major = 2;
    // 计算固件 sha256
    p_ctx = boot_sha256_create();
    temp = HTD_BOOT_VERIFY_CODE_OFFSET + sizeof(fw_head->boot_hdr.verify_code);
    boot_sha256_update(p_ctx, ((uint8_t*)fw_head) + temp, sizeof(boot_firmware_header_t) - temp);
    boot_sha256_update(p_ctx, p_fw_data, fw_size);
    boot_sha256_final(p_ctx, sha256_data);
    // 填写签名验证码
    boot_image_ecdsa_sign(sha256_data, sizeof(sha256_data), fw_head->boot_hdr.verify_code);
    
    // 压缩
    uint32_t compress_data_len = 0;
    uint8_t* p_compress_data;
    if (update_type == BOOT_UPDATE_TYPE_COMPRESS_FASTLZ) {
        p_compress_data = boot_compress_fastlz((uint8_t*)fw_head, sizeof(boot_firmware_header_t)+fw_size, &compress_data_len);
    } else {
        update_type = BOOT_UPDATE_TYPE_COMPRESS_QUICKLZ;
        p_compress_data = boot_compress_quicklz((uint8_t*)fw_head, sizeof(boot_firmware_header_t)+fw_size, &compress_data_len);
    }
    // 填充头部值
    boot_header_t* p_hdr = &fw_head->boot_hdr;
    p_hdr->fw_hdr_size = sizeof(boot_header_t);
    p_hdr->update_type = update_type;
    p_hdr->fw_size = compress_data_len + sizeof(boot_header_t);
    // 计算固件 sha256
    p_ctx = boot_sha256_create();
    temp = HTD_BOOT_VERIFY_CODE_OFFSET + sizeof(fw_head->boot_hdr.verify_code);
    boot_sha256_update(p_ctx, ((uint8_t*)p_hdr) + temp, sizeof(boot_header_t) - temp);
    boot_sha256_update(p_ctx, p_compress_data, compress_data_len);
    boot_sha256_final(p_ctx, sha256_data);
    // 填写签名验证码
    boot_image_ecdsa_sign(sha256_data, sizeof(sha256_data), p_hdr->verify_code);
    
    // 写入数据
    temp = htd_boot_lz_download_addr_get(compress_data_len) - p_env->bankB_addr;
    boot_flash_erase(p_flash, p_env->bankB_addr, p_env->bankB_size);
    boot_flash_write(p_flash, p_env->bankB_addr+temp, (uint8_t*)p_hdr, sizeof(boot_header_t));
    boot_flash_write(p_flash, p_env->bankB_addr+temp+sizeof(boot_header_t), p_compress_data, compress_data_len);
    htd_free(fw_head);
    htd_free(p_compress_data);
    print_debug("ImageB addr: 0x%08x. fw_size=0x%08x", p_env->bankB_addr+temp, p_hdr->fw_size);

    // 写配置信息
    boot_state_update_in_application(update_type, BOOT_STATE_DOWNLOAD_FINISH);
}

// @brief 生成完整升级固件
void htd_boot_fw_full_make(void)
{
    uint32_t temp;
    boot_firmware_header_t *fw_head;
    // HTD_BOOT_SLOT_A_ADDR 这里写版本A，数据内容全为A
    uint32_t fw_size = 32*1024 + 3;
    htd_boot_env_t* p_env = htd_boot_env_info_get();
    const boot_flash_handle_t* p_flash = boot_flash_handle_get(p_env->bankA_addr);
    uint8_t* p_fw_data = htd_malloc(fw_size + sizeof(boot_firmware_header_t));
    ASSERT(p_fw_data);
    fw_head = (boot_firmware_header_t*)p_fw_data;
    p_fw_data += sizeof(boot_firmware_header_t);
    htd_memset(p_fw_data, 'A', fw_size);
    // 写入头部数据
    htd_memset(fw_head, 0, sizeof(boot_firmware_header_t));
    fw_head->boot_hdr.magic = HTD_BOOT_MAGIC;
    fw_head->boot_hdr.fw_hdr_size = sizeof(boot_firmware_header_t);
    fw_head->boot_hdr.update_type = BOOT_UPDATE_TYPE_FULL;
    fw_head->boot_hdr.fw_size = fw_size + sizeof(boot_firmware_header_t);
    fw_head->boot_hdr.ver_major = 1;
    // 计算固件 sha256
    uint8_t sha256_data[32];
    void* p_ctx = boot_sha256_create();
    temp = HTD_BOOT_VERIFY_CODE_OFFSET + sizeof(fw_head->boot_hdr.verify_code);
    boot_sha256_update(p_ctx, ((uint8_t*)fw_head) + temp, sizeof(boot_firmware_header_t) - temp);
    boot_sha256_update(p_ctx, p_fw_data, fw_size);
    boot_sha256_final(p_ctx, sha256_data);
    // 填写签名验证码
    boot_image_ecdsa_sign(sha256_data, sizeof(sha256_data), fw_head->boot_hdr.verify_code);
    // 写入
    boot_flash_erase(p_flash, p_env->bankA_addr, fw_size+4096);
    boot_flash_write(p_flash, p_env->bankA_addr, (uint8_t*)fw_head, sizeof(boot_firmware_header_t)+fw_size);
    htd_free(fw_head);
    print_debug("ImageA addr: 0x%08x, fw_size=0x%08x", p_env->bankA_addr, fw_size);
    
    // p_env->slotB_addr 这里写版本B，数据内容全为B
    fw_size = 34*1024 + 120;
    p_flash = boot_flash_handle_get(p_env->bankB_addr);
    p_fw_data = htd_malloc(fw_size + sizeof(boot_firmware_header_t));
    ASSERT(p_fw_data);
    fw_head = (boot_firmware_header_t*)p_fw_data;
    p_fw_data += sizeof(boot_firmware_header_t);
    htd_memset(p_fw_data, 'B', fw_size);
    // 写入头部数据
    htd_memset(fw_head, 0, sizeof(boot_firmware_header_t));
    fw_head->boot_hdr.magic = HTD_BOOT_MAGIC;
    fw_head->boot_hdr.fw_hdr_size = sizeof(boot_firmware_header_t);
    fw_head->boot_hdr.update_type = BOOT_UPDATE_TYPE_FULL;
    fw_head->boot_hdr.fw_size = fw_size + sizeof(boot_firmware_header_t);
    fw_head->boot_hdr.ver_major = 2;
    // 计算固件 sha256
    p_ctx = boot_sha256_create();
    temp = HTD_BOOT_VERIFY_CODE_OFFSET + sizeof(fw_head->boot_hdr.verify_code);
    boot_sha256_update(p_ctx, ((uint8_t*)fw_head) + temp, sizeof(boot_firmware_header_t) - temp);
    boot_sha256_update(p_ctx, p_fw_data, fw_size);
    boot_sha256_final(p_ctx, sha256_data);
    print_hex("sha256_data", sha256_data, 32);
    // 填写签名验证码
    boot_image_ecdsa_sign(sha256_data, sizeof(sha256_data), fw_head->boot_hdr.verify_code);
    // 写入数据
    temp = htd_boot_full_download_addr_get() - p_env->bankB_addr;
    boot_flash_erase(p_flash, p_env->bankB_addr, fw_size+temp+4096);
    boot_flash_write(p_flash, p_env->bankB_addr+temp, (uint8_t*)fw_head, sizeof(boot_firmware_header_t)+fw_size);
    htd_free(fw_head);
    print_debug("ImageB addr: 0x%08x. fw_size=0x%08x", p_env->bankB_addr+temp, fw_size+sizeof(boot_firmware_header_t));

    // 写配置信息
    boot_state_update_in_application(BOOT_UPDATE_TYPE_FULL, BOOT_STATE_DOWNLOAD_FINISH);
}

// @brief 导出数据到文件
void htd_boot_dump_data(bool is_bankA, const char* p_filepath)
{
    htd_boot_env_t* p_env = htd_boot_env_info_get();
    uint32_t addr = is_bankA ? p_env->bankA_addr : p_env->bankB_addr;
    uint32_t size = is_bankA ? p_env->bankA_size : p_env->bankB_size;

    uint8_t data[256];
    uint32_t data_len = sizeof(data);
    const boot_flash_handle_t* p_flash = boot_flash_handle_get(addr);
    ASSERT(p_flash);
    int fd = open(p_filepath, O_CREAT|O_RDWR|O_BINARY);
    while (size) {
        if (data_len > size) {
            data_len = size;
        }
        boot_flash_read(p_flash, addr, data, data_len);
        write(fd, data, data_len);
        addr += data_len;
        size -= data_len;
    }
    close(fd);
}

// @brief boot 命令
int boot_cmd(int argc, char **argv) 
{
    if (argc < 2) {
        print_error("Usage: %s full/fastlz/quicklz/swap/dump_a/dump_b", argv[0]);
        return 0;
    }
    if (0 == memcmp(argv[1], "full", 5)) {
        htd_boot_fw_full_make();
    } else if (0 == memcmp(argv[1], "quicklz", 8)) {
        htd_boot_fw_compress_make(BOOT_UPDATE_TYPE_COMPRESS_QUICKLZ);
    } else if (0 == memcmp(argv[1], "fastlz", 7)) {
        htd_boot_fw_compress_make(BOOT_UPDATE_TYPE_COMPRESS_FASTLZ);
    } else if (0 == memcmp(argv[1], "swap", 5)) {
        htd_boot_process(NULL);
    } else if (0 == memcmp(argv[1], "dump_a", 6)) {
        if (argc != 3) {
            print_error("Usage: %s dump_a file", argv[0]);
            return 0;
        }
        htd_boot_dump_data(true, argv[2]);
    } else if (0 == memcmp(argv[1], "dump_b", 6)) {
        if (argc != 3) {
            print_error("Usage: %s dump_b file", argv[0]);
            return 0;
        }
        htd_boot_dump_data(false, argv[2]);
    } else {
        print_error("Usage: %s full/fastlz/quicklz/swap/dump_a/dump_b", argv[0]);
    }
    return 0;
}
CONSOLE_CMD_EXPORT(boot_cmd, boot);
#endif

