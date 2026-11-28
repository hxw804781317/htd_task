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
}flashvar_head_t;

// 最大的变量大小
#ifndef BOOT_FLASHVAR_MAX_SIZE
    #define BOOT_FLASHVAR_MAX_SIZE              256   // TODO: 需要根据需求调整
#endif
#define FLASHVAR_INVALID_ADDR                    0xffffffff
#define FLASHVAR_ALIGN_UP(n, align)              (((n) + (align) - 1) & ~((align) - 1))
#define FLASHVAR_ALIGN(n)                        FLASHVAR_ALIGN_UP(n, sizeof(flashvar_head_t))
#define FLASHVAR_MAGIC                           0x8a
const static uint32_t m_flashvar_init_flag = 0x3535bcbc;
const static uint32_t m_flashvar_index_max = 0xfffffff8;


// @brief 内存页剩余多少字节
static uint32_t boot_flash_page_remain_len(uint32_t addr, uint32_t erase_unit)
{
    return (erase_unit - (addr & (erase_unit - 1)));
}

// @brief 判断地址
static bool flashvar_addr_assert(uint32_t start, uint32_t end, uint32_t erase_unit)
{
    // 判断是否为 2 的进制数
    if (! IS_POWER_OF_TWO(erase_unit)) {
        print_error("erase_unit=%d", erase_unit);
        return false;
    }
    // 必须按擦除单位来划分
    if (! IS_ERASE_ADDR(start, erase_unit)) {
        print_error("start=0x%08x, erase_unit=%d", start, erase_unit);
        return false;
    }
    if (! IS_ERASE_ADDR(end, erase_unit)) {
        print_error("end=0x%08x, erase_unit=%d", end, erase_unit);
        return false;
    }
    // 必须两个擦除单位
    if (end - start < erase_unit * 2) {
        print_error("end-start=%u, erase_unit=%u", end - start, erase_unit);
        return false;
    }
    return true;
}

// @brief 从 addr 开始(按环形)找下一个空白槽位, 跳过残渣/已写记录
static uint32_t flashvar_waddr_find(boot_flashvar_t* p_var, uint32_t addr)
{
    flashvar_head_t head;
    const boot_flash_handle_t* p_flash = p_var->p_flash;
    uint32_t guard = (p_var->end - p_var->start) / FLASHVAR_ALIGN(1) + 2;
    uint32_t begin;

    if (addr < p_var->start || addr >= p_var->end) {
        addr = p_var->start;
    }
    begin = addr;
    while (guard--) {
        htd_memset(&head, 0xff, sizeof(flashvar_head_t));
        boot_flash_read(p_flash, addr, (uint8_t*)&head, sizeof(flashvar_head_t));
        if (head.magic == 0xff && head.len == 0xffff && head.crc == 0xff && head.index == 0xffffffff) {
            return addr;
        }
        if (head.magic == FLASHVAR_MAGIC &&
            head.len <= (BOOT_FLASHVAR_MAX_SIZE - sizeof(flashvar_head_t)) &&
            head.index <= m_flashvar_index_max) {
            addr += sizeof(flashvar_head_t) + FLASHVAR_ALIGN(head.len);
        } else {
            addr += FLASHVAR_ALIGN(1); 
        }
        if (addr >= p_var->end) {
            addr = p_var->start;
        }
    }
    // 整片写满: 从"起点所在页"的下一页(最旧的一页)开始, set 里会擦除
    addr = ERASE_ADDR(begin, p_var->erase_unit) + p_var->erase_unit;
    if (addr >= p_var->end) {
        addr = p_var->start;
    }
    return addr;
}

// @brief 检查数据, 同时算出下次可写地址(从最新记录末尾往后找第一个空白)
static void flashvar_data_check(boot_flashvar_t* p_var)
{
    flashvar_head_t head;
    uint8_t data[BOOT_FLASHVAR_MAX_SIZE];

    // 获取最新的地址
    uint32_t addr = p_var->start;
    uint32_t wend = FLASHVAR_INVALID_ADDR;                  // 最新记录的末尾
    const boot_flash_handle_t* p_flash = p_var->p_flash;
    while (addr + sizeof(flashvar_head_t) < p_var->end) {
        // 先读取头部
        htd_memset(&head, 0xff, sizeof(flashvar_head_t));
        boot_flash_read(p_flash, addr, (uint8_t*)&head, sizeof(flashvar_head_t));
        if (head.magic == 0xff && head.len == 0xffff && head.crc == 0xff && head.index == 0xffffffff) {
            if (IS_ERASE_ADDR(addr, p_var->erase_unit)) {
                // 页首就是空白: 整页跳过
                addr = ERASE_ADDR(addr, p_var->erase_unit) + p_var->erase_unit;
            } else {
                // 页内空洞(掉电残渣之后): 继续按步长往后扫, 不能跳页
                addr += FLASHVAR_ALIGN(1);
            }
            continue;
        }
        if (head.magic != FLASHVAR_MAGIC || 
            head.len > (BOOT_FLASHVAR_MAX_SIZE - sizeof(flashvar_head_t)) ||
            head.index > m_flashvar_index_max) {
            addr += FLASHVAR_ALIGN(1);
            continue;
        }
        
        // 读取数据(按 8 字节补齐读, 尾部多读几个 0xff 不影响 crc)
        htd_memset(data, 0xff, FLASHVAR_ALIGN(head.len));
        boot_flash_read(p_flash, addr + sizeof(flashvar_head_t), data, FLASHVAR_ALIGN(head.len));
        if (boot_checksum_get(data, head.len) != head.crc) {
            // 掉电写坏/写了一半的记录: 整条越过
            print_debug("var checksum failed. addr=%08x, len=%d", addr, head.len);
            addr += sizeof(flashvar_head_t) + FLASHVAR_ALIGN(head.len);
            continue;
        }
        // 判断序列号是否更大
        if (head.index > p_var->index) {
            p_var->index = head.index;
            p_var->var_addr = addr;
            p_var->var_len = head.len;
            wend = addr + sizeof(flashvar_head_t) + FLASHVAR_ALIGN(head.len);
        }
        addr += (sizeof(flashvar_head_t) + FLASHVAR_ALIGN(head.len));
    }

    // 未写入过数据
    if (p_var->var_addr == FLASHVAR_INVALID_ADDR) {
        p_var->var_addr = p_var->start;
        p_var->index = 0;
        p_var->var_len = 0;
        p_var->var_waddr = p_var->start;
        boot_flash_erase(p_flash, p_var->var_addr, p_var->erase_unit);
    } else {
        // 从最新记录末尾开始找空白, 掉电残渣/已写记录整条越过(环形)
        p_var->var_waddr = flashvar_waddr_find(p_var, wend);
    }
}

// @brief 初始化参数
static int32_t boot_flashvar_init(boot_flashvar_t* p_var, uint32_t start, uint32_t end)
{
    ASSERT(p_var);
    const boot_flash_handle_t* p_flash = boot_flash_handle_get(start);
    uint32_t erase_unit = boot_flash_erase_unit_get(p_flash, start);
    if (!flashvar_addr_assert(start, end, erase_unit)) {
        return (-ERROR_CODE_INVALID_PARAM);
    }
    
    htd_memset(p_var, 0, sizeof(boot_flashvar_t));
    p_var->p_flash = p_flash;
    p_var->var_addr = FLASHVAR_INVALID_ADDR;
    p_var->start = start;
    p_var->end = end;
    p_var->erase_unit = erase_unit;
    flashvar_data_check(p_var);
    p_var->init_flag = m_flashvar_init_flag;
    print_debug("====>var init finish. p_flash=%p, var_addr=%08x, start=%08x, end=%08x", p_flash, p_var->var_addr, start, end);
    return ERROR_CODE_SUCCESS;
}

// @brief 去初始化
static int32_t boot_flashvar_deinit(boot_flashvar_t* p_var)
{
    if (p_var) {
        htd_memset(p_var, 0, sizeof(boot_flashvar_t));
    }
    return ERROR_CODE_SUCCESS;
}

// @brief 判断是否已经初始化
static bool boot_flashvar_is_init(boot_flashvar_t* p_var)
{
    return (p_var->init_flag == m_flashvar_init_flag);
}

// @brief 更新变量值, 成功返回写入的数据长度，失败返回对应的错误码(负值)
int32_t boot_flashvar_set(boot_flashvar_t* p_var, const void* p_buff, uint16_t buff_len)
{
    ASSERT(p_var);
    ASSERT(p_var->init_flag == m_flashvar_init_flag);
    ASSERT(p_buff);
    if (p_buff == NULL || buff_len == 0 || buff_len + sizeof(flashvar_head_t) > BOOT_FLASHVAR_MAX_SIZE) {
        return ERROR_CODE_INVALID_PARAM;
    }
    
    const boot_flash_handle_t* p_flash = p_var->p_flash;
    uint32_t addr = p_var->var_waddr;
    if (addr < p_var->start || addr >= p_var->end) {
        addr = p_var->start;                        // 越界(整片写满)时从头开始
    }
    uint16_t align_len = FLASHVAR_ALIGN(buff_len);
    // 当前页已经不够写，跳到下一页
    uint16_t remained_len = boot_flash_page_remain_len(addr, p_var->erase_unit);
    if (remained_len < align_len + sizeof(flashvar_head_t)) {
        addr += remained_len;
        if (addr >= p_var->end) {
            addr = p_var->start;
        }
    }

    // 擦除内存页
    if (ERASE_ADDR(addr, p_var->erase_unit) != ERASE_ADDR(p_var->var_addr, p_var->erase_unit)) {
        boot_flash_erase(p_flash, ERASE_ADDR(addr, p_var->erase_unit), p_var->erase_unit);
    }

    uint8_t data[BOOT_FLASHVAR_MAX_SIZE];
    flashvar_head_t* p_head = (flashvar_head_t*)data;
    // 写入头部数据
    htd_memset(data, 0xff, sizeof(data));
    p_head->magic = FLASHVAR_MAGIC;
    p_head->crc = boot_checksum_get(p_buff, buff_len);
    p_head->len = buff_len;
    p_head->index = p_var->index + 1;
    htd_memcpy(&data[sizeof(flashvar_head_t)], p_buff, buff_len);
    // 写入数据
    int32_t ret = boot_flash_write(p_flash, addr, data, sizeof(flashvar_head_t) + align_len);
    if (ret != ERROR_CODE_SUCCESS) {
        print_error("hal flash write failed. ret=%d, addr=0x%08x", ret, addr);
        return ret;
    }
    p_var->index++;
    p_var->var_len = buff_len;
    p_var->var_addr = addr;
    // 下次可写地址
    addr += sizeof(flashvar_head_t) + align_len;
    if (addr >= p_var->end) {
        addr = p_var->start;
    } 
    p_var->var_waddr = addr;
    //print_debug("addr=0x%08x->0x%08x, var_len=%d, index=%u", addr, p_var->var_addr, p_var->var_len, head.index);
    return p_var->var_len;
}

// @brief 读取变量值, 成功返回读取到的数据长度，失败返回对应的错误码(负值)
int32_t boot_flashvar_get(boot_flashvar_t* p_var, void* p_buff, uint16_t buff_size)
{
    ASSERT(p_var);
    ASSERT(p_var->init_flag == m_flashvar_init_flag);
    ASSERT(p_buff && buff_size > 0);
    
    // 没有数据
    if (p_var->var_len == 0) {
        print_error("flash var not find. var_len=%d, start=%08x", p_var->var_len, p_var->start);
        return ERROR_CODE_NOT_FOUND;
    }
    if (p_var->var_len > buff_size) {
        print_error("flash var buffer too small. var_len=%d->%d", p_var->var_len, buff_size);
        return ERROR_CODE_INVALID_PARAM;
    }
    
    flashvar_head_t head;
    uint32_t addr = p_var->var_addr;
    const boot_flash_handle_t* p_flash = p_var->p_flash;
    
    // 读取头部数据
    boot_flash_read(p_flash, addr, (uint8_t*)&head, sizeof(flashvar_head_t));
    if (head.magic != FLASHVAR_MAGIC || head.len != p_var->var_len) {
        print_error("flash read failed. var_len=%d->%d", p_var->var_len, head.len);
        return ERROR_CODE_INVALID_DATA;
        
    }
    addr += sizeof(flashvar_head_t);
    
    // 读取数据内容
    uint8_t data[BOOT_FLASHVAR_MAX_SIZE];
    boot_flash_read(p_flash, addr, data, FLASHVAR_ALIGN(head.len));
    if (head.crc != boot_checksum_get(data, head.len)) {
        print_error("flash read failed. head.len=%d, crc=%d, var_len=%d, addr=%u", head.len, head.crc, p_var->var_len, addr);
        return ERROR_CODE_INVALID_DATA;
    }
    htd_memcpy(p_buff, data, head.len);
    //print_debug("read data. addr=%08x, buff_size=%d, len=%d", addr, buff_size, head.len);
    return head.len;
}



//////////////////////////////////////////////////////////////////////
static boot_flashvar_t m_ftl_var_boot_config;
static boot_info_t m_boot_info;


// @brief 初始化 boot 配置信息，成功返回 true
bool boot_config_init(void)
{
    if (boot_flashvar_is_init(&m_ftl_var_boot_config)) {
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
            print_error("config erase unit must be consistent.");
            ASSERT(0);
        }
        offset += erase_unit;
    }
    
    // 初始化配置信息
    int32_t ret = boot_flashvar_init(&m_ftl_var_boot_config, start_addr, end_addr);
    if (ERROR_CODE_SUCCESS != ret) {
        return false;
    }
    boot_flashvar_get(&m_ftl_var_boot_config, &m_boot_info, sizeof(boot_info_t));
    if (m_boot_info.magic != BOOT_INFO_MAGIC) {
        htd_memset(&m_boot_info, 0, sizeof(boot_info_t));
        m_boot_info.magic = BOOT_INFO_MAGIC;
        boot_flashvar_set(&m_ftl_var_boot_config, &m_boot_info, sizeof(boot_info_t));
    } else {
        print_debug("state=%d, update_type=%d", m_boot_info.state, m_boot_info.update_type);
    }
    return true;
}

// @brief 销毁配置信息
void boot_config_deinit(void)
{
    boot_flashvar_deinit(&m_ftl_var_boot_config);
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
    return boot_flashvar_set(&m_ftl_var_boot_config, &m_boot_info, sizeof(boot_info_t));
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
    return boot_flashvar_set(&m_ftl_var_boot_config, &m_boot_info, sizeof(boot_info_t));
}

// @brief 应用程序配置固件状态
void boot_state_update_in_application(boot_update_type_t update_type, boot_state_t state)
{
    boot_config_init();
    if (m_boot_info.state != state || m_boot_info.update_type != update_type) {
        m_boot_info.update_type = update_type;
        m_boot_info.state = state;
        boot_flashvar_set(&m_ftl_var_boot_config, &m_boot_info, sizeof(boot_info_t));
    }
}

