// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL       4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG       "drv.at24cxx"
#include "htd_common.h"
#include "htd_device.h"
#include "htd_dev_eeprom.h"


//////////////////////////////////////////////////////////////////////
// AT24Cxx 系列I2C地址，默认7位地址为 0x50
// 地址控制线A2，A1，A0；分别控制7位地址的 BIT2,BIT1,BIT0
#ifndef HTD_EEPROM_I2C_ADDR
    #define HTD_EEPROM_I2C_ADDR             0x50
#endif

// 定义容量和页大小
#if defined(HTD_EEPROM_USING_AT24C01)       // AT24C01, 容量(1Kbit), 页大小(8Byte)
    #define EEPROM_USING_AT24CXX            1
    #define EEPROM_SELFTIMED_WRITE_CYCLE    5
    #define AT24CXX_NAME                    "AT24C01"
    #define AT24CXX_CAPACITY                (1*1024/8)
    #define AT24CXX_PAGE_SIZE               8
#elif defined(HTD_EEPROM_USING_AT24C02)     // AT24C02, 容量(2Kbit), 页大小(8Byte)
    #define EEPROM_USING_AT24CXX            1
    #define EEPROM_SELFTIMED_WRITE_CYCLE    5
    #define AT24CXX_NAME                    "AT24C02"
    #define AT24CXX_CAPACITY                (2*1024/8)
    #define AT24CXX_PAGE_SIZE               8
#elif defined(HTD_EEPROM_USING_AT24C04)     // AT24C04, 容量(4Kbit), 页大小(16Byte)
    #define EEPROM_USING_AT24CXX            1
    #define EEPROM_SELFTIMED_WRITE_CYCLE    5
    #define AT24CXX_NAME                    "AT24C041"
    #define AT24CXX_CAPACITY                (4*1024/8)
    #define AT24CXX_PAGE_SIZE               16
#elif defined(HTD_EEPROM_USING_AT24C08)     // AT24C08, 容量(8Kbit), 页大小(16Byte)
    #define EEPROM_USING_AT24CXX            1
    #define EEPROM_SELFTIMED_WRITE_CYCLE    5
    #define AT24CXX_NAME                    "AT24C08"
    #define AT24CXX_CAPACITY                (8*1024/8)
    #define AT24CXX_PAGE_SIZE               16
#elif defined(HTD_EEPROM_USING_AT24C16)     // AT24C16, 容量(16Kbit), 页大小(16Byte)
    #define EEPROM_USING_AT24CXX            1
    #define EEPROM_SELFTIMED_WRITE_CYCLE    5
    #define AT24CXX_NAME                    "AT24C16"
    #define AT24CXX_CAPACITY                (16*1024/8)
    #define AT24CXX_PAGE_SIZE               16
#elif defined(HTD_EEPROM_USING_AT24C32)     // AT24C32, 容量(32Kbit), 页大小(32Byte)
    #define EEPROM_USING_AT24CXX            1
    #define EEPROM_SELFTIMED_WRITE_CYCLE    5
    #define AT24CXX_NAME                    "AT24C32"
    #define AT24CXX_CAPACITY                (32*1024/8)
    #define AT24CXX_PAGE_SIZE               32
#elif defined(HTD_EEPROM_USING_AT24C64)     // AT24C64, 容量(64Kbit), 页大小(32Byte)
    #define EEPROM_USING_AT24CXX            1
    #define EEPROM_SELFTIMED_WRITE_CYCLE    5
    #define AT24CXX_NAME                    "AT24C64"
    #define AT24CXX_CAPACITY                (64*1024/8)
    #define AT24CXX_PAGE_SIZE               32
#elif defined(HTD_EEPROM_USING_AT24C128)    // AT24C128, 容量(128Kbit), 页大小(64Byte)
    #define EEPROM_USING_AT24CXX            1
    #define EEPROM_SELFTIMED_WRITE_CYCLE    5
    #define AT24CXX_NAME                    "AT24C128"
    #define AT24CXX_CAPACITY                (128*1024/8)
    #define AT24CXX_PAGE_SIZE               64
#elif defined(HTD_EEPROM_USING_AT24C256)    // AT24C256, 容量(256Kbit), 页大小(64Byte)
    #define EEPROM_USING_AT24CXX            1
    #define EEPROM_SELFTIMED_WRITE_CYCLE    5
    #define AT24CXX_NAME                    "AT24C256"
    #define AT24CXX_CAPACITY                (256*1024/8)
    #define AT24CXX_PAGE_SIZE               64
#elif defined(HTD_EEPROM_USING_AT24C512)    // AT24C512, 容量(512Kbit), 页大小(64Byte)
    #define EEPROM_USING_AT24CXX            1
    #define EEPROM_SELFTIMED_WRITE_CYCLE    5
    #define AT24CXX_NAME                    "AT24C512"
    #define AT24CXX_CAPACITY                (512*1024/8)
    #define AT24CXX_PAGE_SIZE               128
#elif defined(HTD_EEPROM_USING_AT24CM01)    // AT24CM01, 容量(1024Kbit), 页大小(64Byte)
    #define EEPROM_USING_AT24CXX            1
    #define EEPROM_SELFTIMED_WRITE_CYCLE    5
    #define AT24CXX_NAME                    "AT24CM01"
    #define AT24CXX_CAPACITY                (1024*1024/8)
    #define AT24CXX_PAGE_SIZE               256
#elif defined(HTD_EEPROM_USING_AT24CM02)    // AT24CM02, 容量(2048Kbit), 页大小(64Byte)
    #define EEPROM_USING_AT24CXX            1
    #define EEPROM_SELFTIMED_WRITE_CYCLE    10
    #define AT24CXX_NAME                    "AT24CM02"
    #define AT24CXX_CAPACITY                (2048*1024/8)
    #define AT24CXX_PAGE_SIZE               256
#else
    
#endif



#if (defined(EEPROM_USING_AT24CXX) && (EEPROM_USING_AT24CXX == 1))
//////////////////////////////////////////////////////////////////////
static i2cm_handle_t* m_at24cxx_bus = NULL;
static uint32_t m_at24cxx_last_write_ms = 0;

// @brief 总线初始化
static int32_t at24cxx_bus_init(void)
{
    static const i2cm_user_config_t i2c_config = {
        .pin_sck = (pin_num_t)HTD_EEPROM_I2C_PIN_SCK,
        .pin_sda = (pin_num_t)HTD_EEPROM_I2C_PIN_SDA,
        .MSB = 1,
        .freq_hz = 400*1000,
    };
    if (m_at24cxx_bus == NULL) {
        m_at24cxx_bus = htd_i2cm_open(HTD_EEPROM_BUS_NAME, &i2c_config);
    }
    return 0;
}

// @brief 总线去初始化
static int32_t at24cxx_bus_deinit(void)
{
    if (m_at24cxx_bus) {
        htd_i2cm_close(m_at24cxx_bus);
        m_at24cxx_bus = NULL;
    }
    return 0;
}

// @brief 总线读数据
static int32_t at24cxx_bus_data_read(uint32_t addr, uint8_t* p_data, uint32_t data_size)
{
    uint8_t wbuff[4];
    uint32_t wbuff_len = 0;
#if (AT24CXX_CAPACITY > 2048)
    uint8_t slave_addr = HTD_EEPROM_I2C_ADDR + (addr / 65536);
    wbuff[wbuff_len++] = (addr >> 8) & 0xff;
#else
    uint8_t slave_addr = HTD_EEPROM_I2C_ADDR + (addr / 256);
#endif
    wbuff[wbuff_len++] = addr & 0xff;

    uint32_t msg_count = 0;
    i2cm_msg_t msgs[2];
    msgs[msg_count].slave_addr = slave_addr; 
    msgs[msg_count].flags = I2CM_MSG_FLAG_TX|I2CM_MSG_FLAG_BLOCK; 
    msgs[msg_count].buff = wbuff;
    msgs[msg_count].len = wbuff_len;
    msg_count++;
    msgs[msg_count].slave_addr = slave_addr;
    msgs[msg_count].flags = I2CM_MSG_FLAG_RX|I2CM_MSG_FLAG_BLOCK;
    msgs[msg_count].buff = p_data;
    msgs[msg_count].len = data_size;
    msg_count++;
    if (htd_i2cm_xfer(m_at24cxx_bus, msgs, msg_count, 3000) == msg_count) {
        return ERROR_CODE_SUCCESS;
    } 
    return (-ERROR_CODE_FAILED);
}

// @brief 总线写数据
static int32_t at24cxx_bus_data_write(uint32_t addr, const uint8_t* p_data, uint32_t data_len)
{
    ASSERT(p_data && data_len > 0);

    i2cm_msg_t msgs;
    uint32_t buff_len = 0;
    uint8_t* p_buff = htd_malloc(data_len + 4);
    ASSERT(p_buff);

#if (AT24CXX_CAPACITY > 2048)
    msgs.slave_addr = HTD_EEPROM_I2C_ADDR + (addr / 65536);
    p_buff[buff_len++] = (addr >> 8) & 0xff;
#else
    msgs.slave_addr = HTD_EEPROM_I2C_ADDR + (addr / 256);
#endif
    p_buff[buff_len++] = addr & 0xff;
    htd_memcpy(&p_buff[buff_len], p_data, data_len);
    buff_len += data_len;
    
    msgs.flags = I2CM_MSG_FLAG_TX|I2CM_MSG_FLAG_BLOCK; 
    msgs.buff = p_buff;
    msgs.len = buff_len;
    if (htd_i2cm_xfer(m_at24cxx_bus, &msgs, 1, 3000) == 1) {
        htd_free(p_buff);
        return ERROR_CODE_SUCCESS;
    } 
    htd_free(p_buff);
    return (-ERROR_CODE_FAILED);
}

// @brief 上电控制
static void at24cxx_power_ctrl(bool enable)
{
#if ((defined HTD_EEPROM_POWER_PIN) && (HTD_EEPROM_POWER_PIN > 0))
    uint8_t level = !!enable;
  #if (!defined(HTD_EEPROM_POWER_PIN_ACTIVE_HIGH))
    level = !level;
  #endif
    htd_pin_output_pp_set(HTD_EEPROM_POWER_PIN);
    htd_pin_write(HTD_EEPROM_POWER_PIN, level);
    print_warning("at24cxx power ctrl. enable=%d", enable);
    if (enable) {
        htd_sleep_ms(20);
    } 
#endif 
}

// @brief 初始化，启动设备
static int32_t drv_at24cxx_open(void)
{
    // 上电
    at24cxx_power_ctrl(true);
    // 初始化硬件接口
    at24cxx_bus_init();
    print_debug("at24cxx open finish.");
    return 0;
}

// @brief 去初始化，设备处于低功耗状态
static void drv_at24cxx_close(bool is_power_off)
{
    print_warning("at24cxx uninit. is_power_off=%d", is_power_off);
    if (is_power_off) {
        at24cxx_bus_deinit();
        at24cxx_power_ctrl(false);
    } 
}

// @brief 写数据
int32_t drv_at24cxx_write_handle(uint32_t addr, const uint8_t* p_buff, uint32_t buff_len)
{
    if ((addr + buff_len) > AT24CXX_CAPACITY) {
        print_error("write out of range.");
        return (-ERROR_CODE_INVALID_LENGTH); 
    }
    
    const uint8_t* p_start = p_buff;
    uint32_t max_wlen = AT24CXX_PAGE_SIZE - (addr & (AT24CXX_PAGE_SIZE-1)); 
    if (buff_len <= max_wlen)  {
        max_wlen = buff_len; 
    }
    while (buff_len) {
        while (htd_time_ms_diff(htd_time_ms_get(), m_at24cxx_last_write_ms) <= EEPROM_SELFTIMED_WRITE_CYCLE) {
            htd_delay_us(10);
        }
        if (ERROR_CODE_SUCCESS != at24cxx_bus_data_write(addr, p_buff, max_wlen)) {
            print_error("read data failed. address=0x%08x, max_rlen=%u", addr, max_wlen);
            return (-ERROR_CODE_FAILED);
        }
        addr += max_wlen; 
        p_buff += max_wlen; 
        buff_len -= max_wlen; 
        if (buff_len < AT24CXX_PAGE_SIZE) {
            max_wlen = buff_len;
        } else {
            max_wlen = AT24CXX_PAGE_SIZE; 
        }
        m_at24cxx_last_write_ms = htd_time_ms_get();
    }
    return p_buff - p_start;
}

// @brief 读数据
int32_t drv_at24cxx_read_handle(uint32_t addr, uint8_t* p_buff, uint32_t buff_len)
{
    if ((addr + buff_len) > AT24CXX_CAPACITY) {
        print_error("read out of range.");
        return (-ERROR_CODE_INVALID_LENGTH); 
    }
    
    uint8_t* p_start = p_buff;
    uint32_t max_rlen = AT24CXX_PAGE_SIZE - (addr & (AT24CXX_PAGE_SIZE-1)); 
    if (buff_len <= max_rlen)  {
        max_rlen = buff_len; 
    }

    // 等待写入完成
    while (htd_time_ms_diff(htd_time_ms_get(), m_at24cxx_last_write_ms) <= EEPROM_SELFTIMED_WRITE_CYCLE) {
        htd_delay_us(10);
    }
    
    while (buff_len) {
        if (ERROR_CODE_SUCCESS != at24cxx_bus_data_read(addr, p_buff, max_rlen)) {
            print_error("read data failed. address=0x%08x, max_rlen=%u", addr, max_rlen);
            return (-ERROR_CODE_FAILED);
        }
        addr += max_rlen; 
        p_buff += max_rlen; 
        buff_len -= max_rlen; 
        if (buff_len < AT24CXX_PAGE_SIZE) {
            max_rlen = buff_len;
        } else {
            max_rlen = AT24CXX_PAGE_SIZE; 
        }
    }
    return p_buff - p_start;
}

// @brief 控制句柄
int32_t drv_at24cxx_ctrl(htd_eeprom_cmd_t cmd, void* p_param, uint32_t param_len)
{
    if (cmd == EEPROM_CMD_CLOSE) {
        drv_at24cxx_close(true);
    } else if (cmd == EEPROM_CMD_SHOW_INFO) {
        print_debug("name=%s, capacity=%u, page_size=%u", AT24CXX_NAME, AT24CXX_CAPACITY, AT24CXX_PAGE_SIZE);
    } else if (cmd == EEPROM_CMD_SIZE_GET) {
        if (p_param) {
            UINT32_VAL(p_param) = AT24CXX_CAPACITY;
        }
        return AT24CXX_CAPACITY;
        
    } else {
        return (-ERROR_CODE_INVALID_PARAM);
    }
    return 0;
}

// @brief 注册到上层应用中
static int32_t drv_at24cxx_register(void)
{
    const static htd_eeprom_dev_t m_eeprom_dev = {
        .p_name = AT24CXX_NAME,
        .open_handle = drv_at24cxx_open,
        .write_handle = drv_at24cxx_write_handle,
        .read_handle = drv_at24cxx_read_handle,
        .ctrl_handle = drv_at24cxx_ctrl,
    };
    return htd_dev_eeprom_register(&m_eeprom_dev);
}
DEVICE_INIT_FUNC_REGISTER(drv_at24cxx_register);
#endif /*(defined(EEPROM_USING_AT24CXX) && (EEPROM_USING_AT24CXX == 1))*/

