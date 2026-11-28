// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL   4
#define DEBUG_TAG              "flash.var"
#include "htd_flash_common.h"
#include "htd_flash_var.h"


#if defined(HTD_FLASH_VAR_ENABLE)
//////////////////////////////////////////////////////////////////////
// 头部信息
typedef struct {
    uint8_t magic;                  // 魔数
    uint8_t crc;                    // 校验和
    uint16_t len;                   // 数据长度
    uint32_t index;                 // 序列号递增
}flashvar_head_t;
#define FLASHVAR_INVALID_ADDR           0xffffffff
#define FLASHVAR_ALIGN_UP(n, align)     (((n) + (align) - 1) & ~((align) - 1))
#define FLASHVAR_ALIGN(n)               FLASHVAR_ALIGN_UP(n, sizeof(flashvar_head_t))
#define FLASHVAR_MAGIC                  0x8a
const static uint32_t m_flashvar_init_flag = 0x3535bcbc;
const static uint32_t m_flashvar_index_max = 0xfffffff8;



//////////////////////////////////////////////////////////////////////
// @brief 校验值
uint8_t flashvar_checksum_get(const uint8_t* p_buff, uint16_t buff_len)
{
    return htd_flash_crc8(p_buff, buff_len, 0);
}

// @brief 判断地址
static bool flashvar_addr_assert(const flash_config_t* p_cfg, const flash_handle_t* p_flash)
{
    ASSERT(p_cfg);
    ASSERT(p_flash);
    ASSERT(p_flash->flash_erase);
    ASSERT(p_flash->flash_read);
    ASSERT(p_flash->flash_write);
    
    // 判断是否为 2 的进制数
    if (! IS_POWER_OF_TWO(p_flash->erase_unit)) {
        print_error("erase_unit=%d", p_flash->erase_unit);
        return false;
    }
    // 必须按擦除单位来划分
    if (! IS_ERASE_ADDR(p_cfg->start, p_flash->erase_unit)) {
        print_error("start=0x%08x, erase_unit=%d", p_cfg->start, p_flash->erase_unit);
        return false;
    }
    if (! IS_ERASE_ADDR(p_cfg->end, p_flash->erase_unit)) {
        print_error("end=0x%08x, erase_unit=%d", p_cfg->end, p_flash->erase_unit);
        return false;
    }
    // 必须两个擦除单位
    if (p_cfg->end - p_cfg->start < p_flash->erase_unit * 2) {
        print_error("end-start=%u, erase_unit=%u", p_cfg->end - p_cfg->start, p_flash->erase_unit);
        return false;
    }
    return true;
}

// @brief 从 addr 开始(按环形)找下一个空白槽位, 跳过残渣/已写记录
static uint32_t flashvar_waddr_find(flashvar_t* p_var, uint32_t addr)
{
    flashvar_head_t head;
    const flash_handle_t* p_flash = p_var->p_flash;
    const flash_config_t* p_cfg = p_var->p_cfg;
    uint32_t guard = (p_cfg->end - p_cfg->start) / FLASHVAR_ALIGN(1) + 2;
    uint32_t begin;

    if (addr < p_cfg->start || addr >= p_cfg->end) {
        addr = p_cfg->start;
    }
    begin = addr;
    while (guard--) {
        htd_memset(&head, 0xff, sizeof(flashvar_head_t));
        htd_flash_read(p_flash, addr, &head, sizeof(flashvar_head_t));
        if (head.magic == 0xff && head.len == 0xffff && head.crc == 0xff && head.index == 0xffffffff) {
            return addr;                                            // 空白槽位, 可写
        }
        if (head.magic == FLASHVAR_MAGIC &&
            head.len <= (FLASHVAR_MAX_SIZE - sizeof(flashvar_head_t)) &&
            head.index <= m_flashvar_index_max) {
            addr += sizeof(flashvar_head_t) + FLASHVAR_ALIGN(head.len);  // 残渣/记录: 整条越过
        } else {
            addr += FLASHVAR_ALIGN(1);                              // 头都坏了: 按步长找
        }
        if (addr >= p_cfg->end) {
            addr = p_cfg->start;
        }
    }
    // 整片写满: 从"起点所在页"的下一页(最旧的一页)开始, set 里会擦除
    addr = ERASE_ADDR(begin, p_flash->erase_unit) + p_flash->erase_unit;
    if (addr >= p_cfg->end) {
        addr = p_cfg->start;
    }
    return addr;
}

// @brief 检查数据, 同时算出下次可写地址(从最新记录末尾往后找第一个空白)
static void flashvar_data_check(flashvar_t* p_var)
{
    flashvar_head_t head;
    uint8_t data[FLASHVAR_MAX_SIZE];

    // 获取最新的地址
    uint32_t addr = p_var->p_cfg->start;
    uint32_t wend = FLASHVAR_INVALID_ADDR;                  // 最新记录的末尾
    const flash_handle_t* p_flash = p_var->p_flash;
    while (addr + sizeof(flashvar_head_t) < p_var->p_cfg->end) {
        // 先读取头部
        htd_memset(&head, 0xff, sizeof(flashvar_head_t));
        htd_flash_read(p_flash, addr, &head, sizeof(flashvar_head_t));
        if (head.magic == 0xff && head.len == 0xffff && head.crc == 0xff && head.index == 0xffffffff) {
            if (IS_ERASE_ADDR(addr, p_flash->erase_unit)) {
                // 页首就是空白: 整页跳过
                addr = ERASE_ADDR(addr, p_flash->erase_unit) + p_flash->erase_unit;
            } else {
                // 页内空洞(掉电残渣之后): 继续按步长往后扫, 不能跳页
                addr += FLASHVAR_ALIGN(1);
            }
            continue;
        }
        if (head.magic != FLASHVAR_MAGIC || 
            head.len > (FLASHVAR_MAX_SIZE - sizeof(flashvar_head_t)) ||
            head.index > m_flashvar_index_max) {
            addr += FLASHVAR_ALIGN(1);
            continue;
        }
        
        // 读取数据(按 8 字节补齐读, 尾部多读几个 0xff 不影响 crc)
        htd_memset(data, 0xff, FLASHVAR_ALIGN(head.len));
        htd_flash_read(p_flash, addr + sizeof(flashvar_head_t), data, FLASHVAR_ALIGN(head.len));
        if (flashvar_checksum_get(data, head.len) != head.crc) {
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
        p_var->var_addr = p_var->p_cfg->start;
        p_var->index = 0;
        p_var->var_len = 0;
        p_var->var_waddr = p_var->p_cfg->start;
        htd_flash_erase(p_flash, p_var->var_addr);
    } else {
        // 从最新记录末尾开始找空白, 掉电残渣/已写记录整条越过(环形)
        p_var->var_waddr = flashvar_waddr_find(p_var, wend);
    }
}

// @brief 初始化参数
int32_t htd_flashvar_init(flashvar_t* p_var, const flash_config_t* p_cfg)
{
    ASSERT(p_var);
    ASSERT(p_cfg);
    htd_memset(p_var, 0, sizeof(flashvar_t));
    p_var->p_cfg = p_cfg;
    p_var->var_addr = FLASHVAR_INVALID_ADDR;
    
    p_var->p_flash = htd_flash_handle_find(p_cfg->p_name);
    if (p_var->p_flash == NULL) {
        return ERR_FLASH_NOT_FOUND;
    }
    if (!flashvar_addr_assert(p_cfg, p_var->p_flash)) {
        return ERR_FLASH_INVALID_ARG;
    }

    flashvar_data_check(p_var);
    p_var->init_flag = m_flashvar_init_flag;
    print_debug("init. var_len=%d, var_addr=%08x, waddr=%08x, index=%u",
                p_var->var_len, p_var->var_addr, p_var->var_waddr, p_var->index);
    return ERR_FLASH_SUCCESS;
}

// @brief 更新变量值, 成功返回写入的数据长度，失败返回对应的错误码(负值)
int32_t htd_flashvar_set(flashvar_t* p_var, const void* p_buff, uint16_t buff_len)
{
    ASSERT(p_var);
    ASSERT(p_var->init_flag == m_flashvar_init_flag);
    ASSERT(p_buff);
    if (p_buff == NULL || buff_len == 0 || buff_len + sizeof(flashvar_head_t) > FLASHVAR_MAX_SIZE) {
        return ERR_FLASH_INVALID_ARG;
    }
    
    HTD_FLASH_MUTEX_LOCK(p_var->p_cfg);
    const flash_handle_t* p_flash = p_var->p_flash;
    uint32_t addr = p_var->var_waddr;
    if (addr < p_var->p_cfg->start || addr >= p_var->p_cfg->end) {
        addr = p_var->p_cfg->start;                        // 越界(整片写满)时从头开始
    }
    uint16_t align_len = FLASHVAR_ALIGN(buff_len);
    // 当前页已经不够写，跳到下一页
    uint16_t remained_len = htd_flash_page_remain_len(p_flash, addr);
    if (remained_len < align_len + sizeof(flashvar_head_t)) {
        addr += remained_len;
        if (addr >= p_var->p_cfg->end) {
            addr = p_var->p_cfg->start;
        }
    }

    // 擦除内存页
    if (ERASE_ADDR(addr, p_flash->erase_unit) != ERASE_ADDR(p_var->var_addr, p_flash->erase_unit)) {
        htd_flash_erase(p_flash, ERASE_ADDR(addr, p_flash->erase_unit));
    }

    uint8_t data[FLASHVAR_MAX_SIZE];
    flashvar_head_t* p_head = (flashvar_head_t*)data;
    // 写入头部数据
    htd_memset(data, 0xff, sizeof(data));
    p_head->magic = FLASHVAR_MAGIC;
    p_head->crc = flashvar_checksum_get(p_buff, buff_len);
    p_head->len = buff_len;
    p_head->index = p_var->index + 1;
    htd_memcpy(&data[sizeof(flashvar_head_t)], p_buff, buff_len);
    // 写入数据
    int32_t ret = htd_flash_write(p_flash, addr, data, sizeof(flashvar_head_t) + align_len);
    if (ret != ERR_FLASH_SUCCESS) {
        print_error("hal flash write failed. ret=%d, addr=0x%08x", ret, addr);
        HTD_FLASH_MUTEX_UNLOCK(p_var->p_cfg);
        return ret;
    }
    p_var->index++;
    p_var->var_len = buff_len;
    p_var->var_addr = addr;
    // 下次可写地址
    addr += sizeof(flashvar_head_t) + align_len;
    if (addr >= p_var->p_cfg->end) {
        addr = p_var->p_cfg->start;
    } 
    p_var->var_waddr = addr;
    //print_debug("addr=0x%08x->0x%08x, var_len=%d, index=%u", addr, p_var->var_addr, p_var->var_len, head.index);
    
    HTD_FLASH_MUTEX_UNLOCK(p_var->p_cfg);
    return p_var->var_len;
}

// @brief 读取变量值, 成功返回读取到的数据长度，失败返回对应的错误码(负值)
int32_t htd_flashvar_get(flashvar_t* p_var, void* p_buff, uint16_t buff_size)
{
    ASSERT(p_var);
    ASSERT(p_var->init_flag == m_flashvar_init_flag);
    ASSERT(p_buff && buff_size > 0);
    
    // 没有数据
    HTD_FLASH_MUTEX_LOCK(p_var->p_cfg);
    if (p_var->var_len == 0) {
        HTD_FLASH_MUTEX_UNLOCK(p_var->p_cfg);
        print_error("flash var not find. var_len=%d, start=%08x", p_var->var_len, p_var->p_cfg->start);
        return ERR_FLASH_NOT_FOUND;
    }
    if (p_var->var_len > buff_size) {
        HTD_FLASH_MUTEX_UNLOCK(p_var->p_cfg);
        print_error("flash var buffer too small. var_len=%d->%d", p_var->var_len, buff_size);
        return ERR_FLASH_INVALID_ARG;
    }
    
    flashvar_head_t head;
    int32_t ret = ERR_FLASH_SUCCESS;
    uint32_t addr = p_var->var_addr;
    const flash_handle_t* p_flash = p_var->p_flash;
    
    // 读取头部数据
    htd_flash_read(p_flash, addr, &head, sizeof(flashvar_head_t));
    if (head.magic != FLASHVAR_MAGIC || head.len != p_var->var_len) {
        print_error("flash read failed. var_len=%d->%d", p_var->var_len, head.len);
        ret = ERR_FLASH_INVALID_DATA;
        goto Return;
    }
    addr += sizeof(flashvar_head_t);
    
    // 读取数据内容
    uint8_t data[FLASHVAR_MAX_SIZE];
    htd_flash_read(p_flash, addr, data, FLASHVAR_ALIGN(head.len));
    if (head.crc != flashvar_checksum_get(data, head.len)) {
        print_error("flash read failed. head.len=%d, crc=%d, var_len=%d, addr=%u", head.len, head.crc, p_var->var_len, addr);
        ret = ERR_FLASH_INVALID_DATA;
        goto Return;
    }
    htd_memcpy(p_buff, data, head.len);
    ret = head.len;
    //print_debug("read data. addr=%08x, buff_size=%d, len=%d", addr, buff_size, head.len);
    
Return:
    HTD_FLASH_MUTEX_UNLOCK(p_var->p_cfg);
    return ret;
}



//////////////////////////////////////////////////////////////////////
#if (0 && defined(DEBUG_LVL) && (DEBUG_LVL >= 4) && defined(HTD_CONSOLE_ENABLE) && defined(HTD_USING_NORFLASH))
#include "htd_dev_norflash.h"
#include "htd_console_process.h"

// @brief 初始化
void flashvar_test_cmd(void) 
{
    uint32_t var_count = 0;
    flashvar_t m_flashvar_object;
    flash_config_t flash_handle = {
        .p_name = "norflash_4k",
        .start = NORFLASH_MAP_START_ADDR,
        .end = NORFLASH_MAP_START_ADDR + (2 * 4096),
        .lock_handle = NULL,
        .unlock_handle = NULL,
    };
    
    
    int32_t err_code = htd_flashvar_init(&m_flashvar_object, &flash_handle);
    print_debug("var init. err_num=%d, var_len=%d", err_code, m_flashvar_object.var_len);

    uint8_t buff[64];
    const uint8_t buff_len = 56;
    while (1) {
        if (var_count >= 118) {
            htd_memset(&m_flashvar_object, 0, sizeof(flashvar_t));
            err_code = htd_flashvar_init(&m_flashvar_object, &flash_handle);
            print_debug("ftl variable init. ret=%d", err_code, m_flashvar_object.var_len);
            
            htd_memset(buff, 0, sizeof(buff));
            err_code = htd_flashvar_get(&m_flashvar_object, buff, sizeof(buff));
            print_debug("ftl_variable get. ret=%d, buff=%02x%02x%02x%02x", err_code, buff[0], buff[1], buff[2], buff[4]);
            return;
        }

        htd_memset(buff, var_count, sizeof(buff));
        err_code = htd_flashvar_set(&m_flashvar_object, buff, buff_len);
        print_debug("var_count=%d, ret=%d, buff=%02x%02x%02x%02x", var_count, err_code, buff[0], buff[1], buff[2], buff[4]);
        var_count++;
    }
}
CONSOLE_CMD_EXPORT(flashvar_test_cmd, flashvar_test);
#endif
#endif /*defined(HTD_FLASH_VAR_ENABLE)*/

