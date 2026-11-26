// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL               4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG               "eeprom.simulator"
#include "htd_dev_eeprom.h"


#if defined(HTD_EEPROM_USING_SIMULATE)
//////////////////////////////////////////////////////////////////////
#define EEPROM_SELFTIMED_WRITE_CYCLE    5
#define EEPROM_NAME                     "simulator"
#define EEPROM_CAPACITY                 (8*1024/8)
#define EEPROM_PAGE_SIZE                16
static uint8_t m_eeprom_mem[EEPROM_CAPACITY];


// @brief 初始化
static int32_t eeprom_simulator_open_handle(void)
{
    memset(m_eeprom_mem, 0xff, sizeof(m_eeprom_mem));
    return 0;
}

// @brief 写数据
int32_t eeprom_simulator_write_handle(uint32_t addr, const uint8_t* p_buff, uint32_t buff_len)
{
    if ((addr + buff_len) > EEPROM_CAPACITY) {
        print_error("write out of range.");
        return (-ERROR_CODE_INVALID_LENGTH); 
    }
    
    const uint8_t* p_start = p_buff;
    uint32_t max_wlen = EEPROM_PAGE_SIZE - (addr & (EEPROM_PAGE_SIZE-1)); 
    if (buff_len <= max_wlen)  {
        max_wlen = buff_len; 
    }
    while (buff_len) {
        while (htd_time_ms_diff(htd_time_ms_get(), m_at24cxx_last_write_ms) <= EEPROM_SELFTIMED_WRITE_CYCLE) {
            htd_delay_us(10);
        }
        // 写数据
        memcpy(&m_eeprom_mem[addr], p_buff, max_wlen);
        addr += max_wlen; 
        p_buff += max_wlen; 
        buff_len -= max_wlen; 
        if (buff_len < EEPROM_PAGE_SIZE) {
            max_wlen = buff_len;
        } else {
            max_wlen = EEPROM_PAGE_SIZE; 
        }
        m_at24cxx_last_write_ms = htd_time_ms_get();
    }
    return p_buff - p_start;
}

// @brief 读数据
int32_t eeprom_simulator_read_handle(uint32_t addr, uint8_t* p_buff, uint32_t buff_len)
{
    if ((addr + buff_len) > EEPROM_CAPACITY) {
        print_error("read out of range.");
        return (-ERROR_CODE_INVALID_LENGTH); 
    }
    
    uint8_t* p_start = p_buff;
    uint32_t max_rlen = EEPROM_PAGE_SIZE - (addr & (EEPROM_PAGE_SIZE-1)); 
    if (buff_len <= max_rlen)  {
        max_rlen = buff_len; 
    }
    while (buff_len) {
        // 读数据
        memcpy(p_buff, &m_eeprom_mem[addr], max_rlen);
        addr += max_rlen; 
        p_buff += max_rlen; 
        buff_len -= max_rlen; 
        if (buff_len < EEPROM_PAGE_SIZE) {
            max_rlen = buff_len;
        } else {
            max_rlen = EEPROM_PAGE_SIZE; 
        }
    }
    return p_buff - p_start;
}

// @brief 擦除函数
static int32_t eeprom_simulator_ctrl_handle(htd_eeprom_cmd_t cmd, void* p_param, uint32_t param_len)
{
    if (cmd == EEPROM_CMD_CLOSE) {
        return 0;
    } else if (cmd == EEPROM_CMD_SHOW_INFO) {
        print_debug("name=%s, capacity=%u, page_size=%u", EEPROM_NAME, EEPROM_CAPACITY, EEPROM_PAGE_SIZE);
    } else if (cmd == EEPROM_CMD_SIZE_GET) {
        if (p_param) {
            UINT32_VAL(p_param) = EEPROM_CAPACITY;
        }
        return EEPROM_CAPACITY;
        
    } else {
        return (-ERROR_CODE_INVALID_PARAM);
    }
    return 0;
}

// @brief 注册到上层应用中
static int32_t eeprom_simulator_register(void)
{
    const static htd_eeprom_dev_t m_eeprom_dev = {
        .p_name = EEPROM_NAME,
        .open_handle = eeprom_simulator_open_handle,
        .write_handle = eeprom_simulator_write_handle,
        .read_handle = eeprom_simulator_read_handle,
        .ctrl_handle = eeprom_simulator_ctrl_handle,
    };
    print_debug("eeprom simulator register.");
    return htd_dev_eeprom_register(&m_eeprom_dev);
}
DEVICE_INIT_FUNC_REGISTER(eeprom_simulator_register);
#endif /*defined(HTD_VFS_MODULE_ENABLE) && defined(_WIN32)*/

