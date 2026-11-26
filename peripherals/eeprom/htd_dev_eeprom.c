// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL       4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG       "eeprom"
#include "htd_dev_eeprom.h"
#include "htd_common.h"


//////////////////////////////////////////////////////////////////////
static const htd_eeprom_dev_t *m_eeprom_dev = NULL;
static htd_mutex_lock_t m_eeprom_lock;
#define eeprom_dev_lock_init()           do { bool ret = htd_mutex_lock_init(&m_eeprom_lock, "eeprom"); ASSERT(ret); }while(0)
#define eeprom_dev_lock()                do { htd_mutex_lock(&m_eeprom_lock);} while(0)
#define eeprom_dev_unlock()              do { htd_mutex_unlock(&m_eeprom_lock);} while(0)


// @brief 注册(只能调用一次)
int32_t htd_dev_eeprom_register(const htd_eeprom_dev_t* p_eeprom)
{
    ASSERT(m_eeprom_dev == NULL);
    ASSERT(p_eeprom);
    ASSERT(p_eeprom->read_handle);
    ASSERT(p_eeprom->write_handle);
    ASSERT(p_eeprom->ctrl_handle);
    ASSERT(p_eeprom->open_handle);
    m_eeprom_dev = p_eeprom;
    return 0;
}

// @brief 展示已经挂载的eeprom信息
void htd_dev_eeprom_show(void)
{
#if defined(DEBUG_LVL)
    if (m_eeprom_dev == NULL) {
        print_warning("eeprom unregistered.");
        return;
    }
    eeprom_dev_lock();
    print_log("eeprom dev: %s", m_eeprom_dev->p_name);
    m_eeprom_dev->ctrl_handle(EEPROM_CMD_SHOW_INFO, NULL, 0);
    eeprom_dev_unlock();
#endif
}

// @brief 读数据
int32_t htd_dev_eeprom_read(uint32_t addr, uint8_t* p_data, uint32_t data_len)
{
    int32_t rlen;
    ASSERT(m_eeprom_dev);
    eeprom_dev_lock();
    rlen = m_eeprom_dev->read_handle(addr, p_data, data_len);
    eeprom_dev_unlock();
    return rlen;
}

// @brief 写数据
int32_t htd_dev_eeprom_write(uint32_t addr, const uint8_t* p_data, uint32_t data_len)
{
    int32_t w_len;
    ASSERT(m_eeprom_dev);
    eeprom_dev_lock();
    w_len = m_eeprom_dev->write_handle(addr, p_data, data_len);
    eeprom_dev_unlock();
    return w_len;
}

// @brief 获取大小
uint32_t htd_dev_eeprom_size_get(void)
{
    ASSERT(m_eeprom_dev);
    
    uint32_t param;
    eeprom_dev_lock();
    if (sizeof(param) != m_eeprom_dev->ctrl_handle(EEPROM_CMD_SIZE_GET, &param, sizeof(param))) {
        param = 0;
    } 
    eeprom_dev_unlock();
    return param;
}

// @brief 初始化
int32_t htd_dev_eeprom_init(void)
{
    ASSERT(m_eeprom_dev);
    eeprom_dev_lock_init();
    return m_eeprom_dev->open_handle();
}

// @brief 去初始化
int32_t htd_dev_eeprom_uninit(void)
{
    ASSERT(m_eeprom_dev);
    return 0;
}


//////////////////////////////////////////////////////////////////////
#if defined(HTD_CONSOLE_ENABLE) && (DEBUG_LVL > 0) && 0
#include "htd_console_process.h"


// @brief 测试接口
uint8_t eeprom_test(void)
{
    #define TEST_BUFFER_SIZE     256
    uint8_t wbuff[TEST_BUFFER_SIZE];
    uint8_t rbuff[TEST_BUFFER_SIZE];
    uint32_t addr = 0;

    // 写入数据
    print_debug("EEPROM writing...");
    for(uint32_t i=0; i< sizeof(wbuff); i++) {
        wbuff[i] = i;
    }
    htd_dev_eeprom_write(addr, wbuff, sizeof(wbuff));

    // 读取数据
    print_debug("EEPROM reading...");
    htd_dev_eeprom_read(addr, rbuff, sizeof(rbuff));

    // 比较内容
    if (memcmp(rbuff, wbuff, sizeof(rbuff))) {
        print_error("data read and write aren't matching.");
        print_hex("rbuff", rbuff, sizeof(rbuff));
        return 1;
    }
    print_debug("EEPROM test success.");
    return 0;
}


// @brief 调试命令
static int cmd_eeprom(int argc, char **argv)
{
    if (argc != 2) {
        print_error("Usage: %s show/test", argv[0]);
        return 0;
    }
    
    if (memcmp(argv[1], "show", 5) == 0) {
        htd_dev_eeprom_show();
    } else if (memcmp(argv[1], "test", 5) == 0) {
        eeprom_test();
    } else {
        print_error("Usage: %s show/test", argv[0]);
    }
    return 0;
}
CONSOLE_CMD_EXPORT(cmd_eeprom, eeprom);

#endif /**/

