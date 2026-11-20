// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL       2       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG       "boot.config"
#include "htd_boot_port.h"
#include "htd_boot_config.h"


//////////////////////////////////////////////////////////////////////
// 头部信息
typedef struct {
    uint8_t magic;                  // 魔数
    uint8_t crc;                    // 校验和
    uint16_t len;                   // 数据长度
    uint32_t index;                 // 序列号递增
}ftl_val_head_t;

// 最大的变量大小
#ifndef BOOT_FTL_VAR_MAX_SIZE
    #define BOOT_FTL_VAR_MAX_SIZE               256   // TODO: 需要根据需求调整
#endif
#define FTL_VAR_INVALID_ADDR                    0xffffffff
#define FTL_VAR_ALIGN_UP(n, align)              (((n) + (align) - 1) & ~((align) - 1))
#define FTL_VAR_ALIGN(n)                        FTL_VAR_ALIGN_UP(n, sizeof(ftl_val_head_t))
#define FTL_VAR_MAGIC                           0x8a
const static uint32_t ftl_var_init_flag = 0x3535bcbc;


// @brief 内存页剩余多少字节
static uint32_t boot_flash_page_remain_len(uint32_t addr, uint32_t erase_unit)
{
    return (erase_unit - (addr & (erase_unit - 1)));
}

// @brief 判断地址
static bool ftl_assert_addr(uint32_t start, uint32_t end, uint32_t erase_unit)
{
    // 判断是否为 2 的进制数
    if (! IS_POWER_OF_TWO(erase_unit)) {
        boot_error("erase_unit=%d", erase_unit);
        return false;
    }
    // 必须按擦除单位来划分
    if (! IS_ERASE_ADDR(start, erase_unit)) {
        boot_error("start=0x%08x, erase_unit=%d", start, erase_unit);
        return false;
    }
    if (! IS_ERASE_ADDR(end, erase_unit)) {
        boot_error("end=0x%08x, erase_unit=%d", end, erase_unit);
        return false;
    }
    // 必须两个擦除单位
    if (end - start < erase_unit * 2) {
        boot_error("end-start=%u, erase_unit=%u", end - start, erase_unit);
        return false;
    }
    return true;
}

// @brief 检查数据
static void ftl_var_check_data(boot_ftl_var_t* p_var)
{
    ftl_val_head_t head;
    uint8_t data[BOOT_FTL_VAR_MAX_SIZE];

    // 获取最新的地址
    uint32_t addr = p_var->start;
    while (addr + sizeof(ftl_val_head_t) < p_var->end) {
        // 先读取头部
        htd_memset(&head, 0xff, sizeof(ftl_val_head_t));
        boot_flash_read(p_var->p_flash, addr, (uint8_t*)&head, sizeof(ftl_val_head_t));
        if (head.magic == 0xff && head.len == 0xffff && head.crc == 0xff && head.index == 0xffffffff) {
            // 跳到下一页
            addr = ERASE_ADDR(addr, p_var->erase_unit) + p_var->erase_unit;
            continue;
        } 
        if (head.magic != FTL_VAR_MAGIC || head.len > (BOOT_FTL_VAR_MAX_SIZE - sizeof(ftl_val_head_t))) {
            addr += FTL_VAR_ALIGN(1);
            continue;
        }
        
        // 读取数据
        htd_memset(data, 0xff, head.len);
        boot_flash_read(p_var->p_flash, addr + sizeof(ftl_val_head_t), data, head.len);
        // 判断校验是否通过
        if (boot_checksum_get(data, head.len) != head.crc) {
            boot_error("ftl param checksum failed. addr=0x%08x, var_addr=0x%08x", addr, p_var->var_addr);
            addr += FTL_VAR_ALIGN(1);
            continue;
        }
        // 判断序列号是否更大
        if (head.index > p_var->index) {
            p_var->index = head.index;
            p_var->var_addr = addr;
            p_var->var_len = head.len;
        }
        addr += (sizeof(ftl_val_head_t) + FTL_VAR_ALIGN(head.len));
        boot_debug("index=%d->%d, addr=%08x, next_addr=%08x", head.index, p_var->index, p_var->var_addr, addr);
    }
    
    // 未写入过数据
    if (p_var->var_addr == FTL_VAR_INVALID_ADDR) {
        p_var->var_addr = p_var->start;
        p_var->index = 0;
        p_var->var_len = 0;
        boot_flash_erase(p_var->p_flash, p_var->start, p_var->erase_unit);
    }
}

// @brief 初始化参数
static int32_t ftl_var_init(boot_ftl_var_t* p_var, uint32_t start, uint32_t end)
{
    ASSERT(p_var);
    const boot_flash_handle_t* p_flash = boot_flash_handle_get(start);
    uint32_t erase_unit = boot_flash_erase_unit_get(p_flash, start);
    if (!ftl_assert_addr(start, end, erase_unit)) {
        return (-ERROR_CODE_INVALID_PARAM);
    }
    
    htd_memset(p_var, 0, sizeof(boot_ftl_var_t));
    p_var->p_flash = p_flash;
    p_var->init_flag = ftl_var_init_flag;
    p_var->var_addr = FTL_VAR_INVALID_ADDR;
    p_var->start = start;
    p_var->end = end;
    p_var->erase_unit = erase_unit;
    ftl_var_check_data(p_var);
    boot_debug("====>var init finish. p_flash=%p, var_addr=%08x, start=%08x, end=%08x", p_flash, p_var->var_addr, start, end);
    return ERROR_CODE_SUCCESS;
}

// @brief 去初始化
static int32_t ftl_var_deinit(boot_ftl_var_t* p_var)
{
    if (p_var) {
        htd_memset(p_var, 0, sizeof(boot_ftl_var_t));
    }
    return ERROR_CODE_SUCCESS;
}

// @brief 判断是否已经初始化
static bool ftl_var_is_init(boot_ftl_var_t* p_var)
{
    return (p_var->init_flag == ftl_var_init_flag);
}

// @brief 更新变量值
static int32_t ftl_var_set(boot_ftl_var_t* p_var, const void* p_buff, uint16_t buff_len)
{
    ASSERT(p_var);
    ASSERT(p_var->init_flag == ftl_var_init_flag);
    ASSERT(p_buff && buff_len > 0);
    ASSERT(buff_len + sizeof(ftl_val_head_t) <= BOOT_FTL_VAR_MAX_SIZE);
    
    uint32_t addr = p_var->var_addr;
    if (p_var->var_len) {
        addr += sizeof(ftl_val_head_t) + FTL_VAR_ALIGN(p_var->var_len);
        if (addr >= p_var->end) {
            addr = p_var->start;
        } 
    }

    uint16_t align_len = FTL_VAR_ALIGN(buff_len);
    uint16_t remained_len = boot_flash_page_remain_len(addr, p_var->erase_unit);
    if (remained_len < align_len + sizeof(ftl_val_head_t)) {
        addr += remained_len;
        if (addr >= p_var->end) {
            addr = p_var->start;
        }
    }

    // 擦除
    if (ERASE_ADDR(addr, p_var->erase_unit) != ERASE_ADDR(p_var->var_addr, p_var->erase_unit)) {
        boot_flash_erase(p_var->p_flash, ERASE_ADDR(addr, p_var->erase_unit), p_var->erase_unit);
    }
    
    // 写入头部数据
    uint8_t data[BOOT_FTL_VAR_MAX_SIZE];
    ftl_val_head_t head;
    head.magic = FTL_VAR_MAGIC;
    head.crc = boot_checksum_get(p_buff, buff_len);
    head.len = buff_len;
    head.index = p_var->index + 1;
    htd_memcpy(data, &head, sizeof(ftl_val_head_t));
    htd_memcpy(&data[sizeof(ftl_val_head_t)], p_buff, buff_len);
    
    // 写入数据
    int32_t ret = boot_flash_write(p_var->p_flash, addr, data, buff_len + sizeof(ftl_val_head_t));
    if (ret != buff_len + sizeof(ftl_val_head_t)) {
        boot_error("hal flash write failed. ret=%d, addr=0x%08x", ret, addr);
        return ret;
    }
    p_var->index++;
    p_var->var_len = buff_len;
    boot_debug("addr=0x%08x->0x%08x, var_len=%d, index=%u", addr, p_var->var_addr, p_var->var_len, head.index);
    p_var->var_addr = addr;
    return ret;
}

// @brief 读取变量值, 返回读取到的数据长度
static int32_t ftl_var_get(boot_ftl_var_t* p_var, void* p_buff, uint16_t buff_size)
{
    ASSERT(p_var);
    ASSERT(p_var->init_flag == ftl_var_init_flag);
    ASSERT(p_buff && buff_size > 0);
    
    // 没有数据
    if (p_var->var_len == 0 || p_var->var_len > buff_size) {
        boot_error("ftl var not find. var_len=%d, start=%08x", p_var->var_len, p_var->start);
        return (-ERROR_CODE_NOT_FOUND);
    }
    
    // 读取头部数据
    ftl_val_head_t head;
    uint32_t addr = p_var->var_addr;
    boot_flash_read(p_var->p_flash, addr, (uint8_t*)&head, sizeof(ftl_val_head_t));
    if (head.magic != FTL_VAR_MAGIC || head.len != p_var->var_len) {
        boot_error("flash read failed. var_len=%d->%d", p_var->var_len, head.len);
        return (-ERROR_CODE_INVALID_DATA);
    }
    
    // 读取数据内容
    boot_flash_read(p_var->p_flash, addr+sizeof(ftl_val_head_t), p_buff, head.len);
    if (head.crc != boot_checksum_get(p_buff, head.len)) {
        boot_error("flash read failed. head.len=%d, crc=%d, var_len=%d, addr=%u", head.len, head.crc, p_var->var_len, addr);
        return (-ERROR_CODE_INVALID_DATA);
    }
    //boot_debug("read data. addr=%08x, buff_size=%d, len=%d", addr, buff_size, head.len);
    return head.len;
}



//////////////////////////////////////////////////////////////////////
static boot_ftl_var_t m_ftl_var_boot_config;
static boot_info_t m_boot_info;


// @brief 初始化 boot 配置信息，成功返回 true
bool boot_config_init(void)
{
    if (ftl_var_is_init(&m_ftl_var_boot_config)) {
        return true;
    }

    uint32_t erase_unit;
    uint32_t start_addr = HTD_BOOT_CONFIG_ADDR;
    uint32_t end_addr = HTD_BOOT_CONFIG_ADDR + HTD_BOOT_CONFIG_SIZE;
    const boot_flash_handle_t* p_flash;
    if (start_addr == 0xffffffff) {
        end_addr = HTD_BOOTLOADER_ADDR + HTD_BOOTLOADER_SIZE;
        p_flash = boot_flash_handle_get(end_addr-1);
        erase_unit = boot_flash_erase_unit_get(p_flash, end_addr-1);
        start_addr = end_addr - erase_unit * 2;
    } else {
        p_flash = boot_flash_handle_get(start_addr);
        erase_unit = boot_flash_erase_unit_get(p_flash, start_addr);
    }
    
    // 擦除地址必须一致
    uint32_t offset = erase_unit;
    while (start_addr + offset < end_addr) {
        if (erase_unit != boot_flash_erase_unit_get(p_flash, start_addr+offset)) {
            boot_error("config erase unit must be consistent.");
            ASSERT(0);
        }
        offset += erase_unit;
    }
    
    // 初始化配置信息
    int32_t ret = ftl_var_init(&m_ftl_var_boot_config, start_addr, end_addr);
    if (ERROR_CODE_SUCCESS != ret) {
        return false;
    }
    ftl_var_get(&m_ftl_var_boot_config, &m_boot_info, sizeof(boot_info_t));
    if (m_boot_info.magic != BOOT_INFO_MAGIC) {
        htd_memset(&m_boot_info, 0, sizeof(boot_info_t));
        m_boot_info.magic = BOOT_INFO_MAGIC;
        ftl_var_set(&m_ftl_var_boot_config, &m_boot_info, sizeof(boot_info_t));
    } else {
        boot_debug("state=%d, update_type=%d", m_boot_info.state, m_boot_info.update_type);
    }
    return true;
}

// @brief 销毁配置信息
void boot_config_deinit(void)
{
    ftl_var_deinit(&m_ftl_var_boot_config);
}
    
// @brief 获取升级类型
boot_update_type_t boot_config_update_type_get(void)
{
    return m_boot_info.update_type;
}

// @brief 设置升级类型
void boot_config_update_type_set(boot_update_type_t update_type)
{
    m_boot_info.update_type = update_type;
}

// @brief 获取状态
boot_state_t boot_config_state_get(void)
{
    return m_boot_info.state;
}

// @brief 获取偏移量
uint32_t boot_config_offset_get(void)
{
    return m_boot_info.offset;
}

// @brief 保存进度和状态
int32_t boot_config_offset_state_set(uint32_t offset, boot_state_t state)
{
    m_boot_info.offset = offset;
    m_boot_info.state = state;
    return ftl_var_set(&m_ftl_var_boot_config, &m_boot_info, sizeof(boot_info_t));
}

// @brief 获取配置信息
void boot_config_get(boot_info_t* p_config)
{
    ASSERT(p_config);
    htd_memcpy(p_config, &m_boot_info, sizeof(boot_info_t));
}

// @brief 写入配置信息
int32_t boot_config_set(boot_info_t* p_config)
{
    ASSERT(p_config);
    htd_memcpy(&m_boot_info, p_config, sizeof(boot_info_t));
    return ftl_var_set(&m_ftl_var_boot_config, &m_boot_info, sizeof(boot_info_t));
}

// @brief 应用程序配置固件状态
void boot_state_update_in_application(boot_update_type_t update_type, boot_state_t state)
{
    boot_config_init();
    if (m_boot_info.state != state || m_boot_info.update_type != update_type) {
        m_boot_info.update_type = update_type;
        m_boot_info.state = state;
        ftl_var_set(&m_ftl_var_boot_config, &m_boot_info, sizeof(boot_info_t));
    }
}

