// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL               4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG               "norflash"
#include "htd_device.h"
#include "htd_dev_spim.h"
#include "htd_dev_norflash.h"
#include "htd_components.h"
#include "drv_norflash_sff_parser.h"


#if (!defined(_WIN32)) && (defined(BSP_USING_SPIM)) && 1
//////////////////////////////////////////////////////////////////////
static dev_norflash_t m_norflash = {0};
static spim_handle_t* m_bus_handle = NULL;


// @brief 地址数组
static uint32_t address_array_make(const dev_norflash_t* p_flash, uint32_t addr, uint8_t *array)
{
    uint32_t offset = 0;
    if (p_flash->capacity > 0xffffff) {
        array[offset++] = (addr & 0xff000000) >> 24;
    }
    array[offset++] = (addr & 0xff0000) >> 16;
    array[offset++] = (addr & 0xff00) >> 8;
    array[offset++] = (addr & 0xff) >> 0;
    return offset;
}

// @brief 写命令
static int32_t norflash_write_cmd(spim_handle_t* p_bus, uint8_t cmd)
{
    uint8_t cmd_buff[2];
    cmd_buff[0] = cmd, cmd_buff[1] = 0;
    int32_t ret = htd_spim_transfer(p_bus, cmd_buff, sizeof(uint8_t), NULL, 0);
    if (ret != sizeof(uint8_t)) {
        print_error("write comand failed. ret=%d", ret);
        return -1;
    }
    return 0;
}

// @brief 读取状态
static int32_t norflash_status_read(spim_handle_t* p_bus, uint8_t *status)
{
    ASSERT(status);

    uint8_t wbuff[2];
    uint8_t rbuff[2];
    wbuff[0] = 0x05, wbuff[1] = 0;
    htd_memset(rbuff, 0, sizeof(rbuff));

    int32_t ret = htd_spim_transfer(p_bus, wbuff, sizeof(uint8_t), rbuff, sizeof(uint8_t));
    if (ret != sizeof(uint8_t)) {
        print_error("read status failed. ret=%d", ret);
        return -1;
    }
    *status = rbuff[0];
    return 0;
}

// @brief 写状态寄存器值
static void norflash_status_write(spim_handle_t* p_bus, uint8_t status)
{
    uint8_t data[2];
    data[0] = 0x01;
    data[1] = status;
    htd_spim_transfer(p_bus, data, 2, NULL, 0);
}

// @brief 等待空闲
static int32_t norflash_waite_for_idle(spim_handle_t* p_bus)
{
    uint8_t status;
    for (;;) {
        status = 0;
        if (0 != norflash_status_read(p_bus, &status)) {
            break;
        }
        //print_warning("flash status: 0x%02x", status);
        if (!(status & 0x01)) {
            return 0;
        }
        htd_delay_us(100);
    }
    return -1;
}

// @brief 禁用写保护
static void norflash_program_protect_disable(spim_handle_t* p_bus, uint8_t mf_id)
{
    if (mf_id == SFF_MF_ID_WINBOND || mf_id == SFF_MF_ID_GIGA) {
        // 华邦芯片，不同芯片方法不同，需要根据手册补充
        uint8_t status = 0;
        norflash_status_read(p_bus, &status);
        if ((status & (__BIT(3)|__BIT(4))) == 0) {
            return;
        }
        // 使能写
        norflash_write_cmd(p_bus, 0x06);
        // 控制状态寄存器值
        norflash_status_write(p_bus, 0);
        return;
    }
}

// @brief 使能/禁用写
static int32_t norflash_write_enable_set(spim_handle_t* p_bus, bool is_enable)
{
    return norflash_write_cmd(p_bus, is_enable ? 0x06 : 0x04);
}

// @brief 设置4字节地址模式
static int32_t norflash_4byte_addr_mode_set(spim_handle_t* p_bus, bool is_enable)
{
    // 使能写
    int32_t ret = norflash_write_enable_set(p_bus, true);
    ASSERT(ret == 0);
    // 写命令
    ret = norflash_write_cmd(p_bus, is_enable ? 0xb7 : 0xe9);
    ASSERT(ret == 0);
    return 0;
}

// @brief 休眠
static void norflash_powerdown(spim_handle_t* p_bus)
{
    if (p_bus == NULL) {
        return;
    }
    int32_t ret = norflash_waite_for_idle(p_bus);
    ASSERT(ret == 0);
    ret = norflash_write_cmd(p_bus, 0xb9);
    ASSERT(ret == 0);
}

// @brief 唤醒
static void norflash_wakeup(spim_handle_t* p_bus)
{
    if (p_bus == NULL) {
        return;
    }
    int32_t ret = norflash_waite_for_idle(p_bus);
    ASSERT(ret == 0);
    ret = norflash_write_cmd(p_bus, 0xab);
    ASSERT(ret == 0);
}

// @brief 写函数, 只能从1->0, 不能从0->1, 不能跨页写入
static int32_t drv_norflash_write(uint32_t addr, const uint8_t* p_data, uint32_t data_len)
{
    ASSERT(m_bus_handle);
    ASSERT((addr & (m_norflash.write_unit - 1)) == 0);
    ASSERT((data_len & (m_norflash.write_unit - 1)) == 0);
    ASSERT(((addr & (m_norflash.page_size - 1)) + data_len) <= m_norflash.page_size);

#if 0
    int32_t ret;
    uint8_t buff[6];
    uint32_t buff_len = 0;
    spim_msg_t msg[2];
    
    // 等待空闲
    ret = norflash_waite_for_idle(m_bus_handle);
    ASSERT(ret == 0);
    // 使能写
    ret = norflash_write_enable_set(m_bus_handle, true);
    ASSERT(ret == 0);
    // 写入数据
    buff[buff_len++] = 0x02;
    buff_len += address_array_make(&m_norflash, addr, &buff[buff_len]);
    msg[0].flags = SPIM_MSG_FLAG_TX|SPIM_MSG_FLAG_BLOCK;
    msg[0].buff = buff;
    msg[0].len = buff_len;
    msg[1].flags = SPIM_MSG_FLAG_TX|SPIM_MSG_FLAG_BLOCK;
    msg[1].buff = (uint8_t*)p_data;
    msg[1].len = data_len;
    htd_spim_xfer(m_bus_handle, msg, 2);
    // 禁用写
    //ret = norflash_write_enable_set(m_bus_handle, false);
    //ASSERT(ret == 0);
#else
    int32_t ret;
    spim_msg_t msg[2];
    uint32_t buff_len = 0;
    uint8_t* p_buff = (uint8_t*)htd_malloc(data_len + 6);
    ASSERT(p_buff);
    // 等待空闲
    ret = norflash_waite_for_idle(m_bus_handle);
    ASSERT(ret == 0);
    // 使能写
    ret = norflash_write_enable_set(m_bus_handle, true);
    ASSERT(ret == 0);
    p_buff[buff_len++] = 0x02;
    buff_len += address_array_make(&m_norflash, addr, &p_buff[buff_len]);
    htd_memcpy(&p_buff[buff_len], p_data, data_len);
    buff_len += data_len;
    msg[0].flags = SPIM_MSG_FLAG_TX|SPIM_MSG_FLAG_BLOCK;
    msg[0].buff = p_buff;
    msg[0].len = buff_len;
    htd_spim_xfer(m_bus_handle, msg, 1);
    htd_free(p_buff);
#endif
    
    return data_len;
}

// @brief 读函数, 不能跨页读取
static int32_t drv_norflash_read(uint32_t addr, uint8_t* p_data, uint32_t data_len)
{
    ASSERT(m_bus_handle);
    ASSERT((addr & (m_norflash.write_unit - 1)) == 0);
    ASSERT((data_len & (m_norflash.write_unit - 1)) == 0);
    ASSERT(((addr & (m_norflash.page_size - 1)) + data_len) <= m_norflash.page_size);

    // FIXME: 是否支持 xip
    
    int32_t ret;
    uint8_t buff[6];
    uint32_t buff_len = 0;
    
    // 等待空闲
    ret = norflash_waite_for_idle(m_bus_handle);
    ASSERT(ret == 0);
    // 写入命令和地址
    buff[buff_len++] = 0x03;
    buff_len += address_array_make(&m_norflash, addr, &buff[buff_len]);
    ret = htd_spim_transfer(m_bus_handle, buff, buff_len, p_data, data_len);
    ASSERT(ret == data_len);
    return data_len;
}

// @brief 擦除函数
static int32_t drv_norflash_erase(uint32_t addr, uint32_t size)
{
    ASSERT(m_bus_handle);
    
    int32_t ret;
    uint8_t buff[6];
    uint32_t buff_len;
    
    // 等待空闲
    ret = norflash_waite_for_idle(m_bus_handle);
    ASSERT(ret == 0);
    // 使能写
    ret = norflash_write_enable_set(m_bus_handle, true);
    ASSERT(ret == 0);
    addr = ALIGN_DOWN(addr, m_norflash.erase_unit);
    while (size) {
        // 等待空闲
        ret = norflash_waite_for_idle(m_bus_handle);
        ASSERT(ret == 0);
        // 写命令和地址
        buff_len = 0;
        buff[buff_len++] = m_norflash.erase_cmd;
        buff_len += address_array_make(&m_norflash, addr, &buff[buff_len]);
        ret = htd_spim_transfer(m_bus_handle, buff, buff_len, NULL, 0);
        ASSERT(ret == buff_len);
        addr += m_norflash.erase_unit;
        if (size <= m_norflash.erase_unit) {
            break;
        } 
        size -= m_norflash.erase_unit;
    }
    return 0;
}

// @brief 展示每一页的擦除次数
static void drv_norflash_info_show(void)
{
#if defined(DEBUG_LVL) && (DEBUG_LVL > 1)
    print_warning("norflash name: %s, bus_name=%s", m_norflash.p_name, m_bus_handle->p_dev->p_name);
    print_warning("phy_saddr=0x%08x, erase_unit=%d", NORFLASH_MAP_START_ADDR, m_norflash.erase_unit);
    print_warning("page_size=%d, capacity=0x%08x", m_norflash.page_size, m_norflash.capacity);
    print_warning("mf_id=0x%02x, type_id=0x%02x", m_norflash.mf_id, m_norflash.type_id);
    print_warning("erase_cmd=0x%02x, write_unit=%d", m_norflash.erase_cmd, m_norflash.write_unit);
#endif 
}

// @brief 擦除函数
static int32_t drv_norflash_ctrl(norflash_cmd_t cmd, void* p_param, uint32_t param_len)
{
    if (cmd == NORFLASH_CMD_SECTOR_ERASE) {
        norflash_erase_param_t* p_info = (norflash_erase_param_t*)p_param;
        drv_norflash_erase(p_info->addr, p_info->size);
        
    } else if (cmd == NORFLASH_CMD_SIZE_GET) {
        ASSERT(p_param);
        *(uint32_t*)p_param = m_norflash.capacity;
        return sizeof(uint32_t);
        
    } else if (cmd == NORFLASH_CMD_SHOW_INFO) {
        drv_norflash_info_show();
        
    } else if (cmd == NORFLASH_CMD_POWER_DOWN) {
        norflash_powerdown(m_bus_handle);
        
    } else if (cmd == NORFLASH_CMD_WAKE_UP) {
        norflash_wakeup(m_bus_handle);
        
    } else if (cmd == NORFLASH_CMD_CLOSE) {
        if (m_bus_handle) {
            norflash_powerdown(m_bus_handle);
            htd_spim_close(m_bus_handle);
            m_bus_handle = NULL;
        }
    }
    return 0;
}

// @brief 数据交互
static int32_t sff_spi_transfer(uint8_t* p_wbuff, uint32_t wbuff_len, uint8_t* p_rbuff, uint32_t rbuff_len)
{
    return htd_spim_transfer(m_bus_handle, p_wbuff, wbuff_len, p_rbuff, rbuff_len);
}

// @brief 初始化, 创建一个对等大小的文件
static int32_t drv_norflash_init(void)
{
    if (m_bus_handle != NULL) {
        return -1;
    }

    const static spim_user_config_t param = { 
        .pin_cs = (pin_num_t)HTD_NORFLASH_PIN_CS, //PIN_NUM_INVALID,
        .pin_sck = (pin_num_t)HTD_NORFLASH_PIN_SCK,
        .pin_mosi = (pin_num_t)HTD_NORFLASH_PIN_MOSI,
        .pin_miso = (pin_num_t)HTD_NORFLASH_PIN_MISO,
        .cs_active_level = 0,
        .is_1line = 0,
        .is_bit_order_msb = 1,
        .data_width = 8,
        .mode = SPIM_MODE_0,
        .freq_hz = 4*1000*1000,
        .xfer_finish = NULL,
    };
    spim_handle_t* p_handle = htd_spim_open(HTD_NORFLASH_BUS_NAME, &param);
    if (p_handle == NULL) {
        print_error("norflash open failed. %s", HTD_NORFLASH_BUS_NAME);
        return -1;
    }
    m_bus_handle = p_handle;
    print_debug("norflash %s open finish.", HTD_NORFLASH_BUS_NAME);
    
    int32_t ret;
    sff_flash_info_t sff_info;
    ret = sff_parser_info_get(sff_spi_transfer, &sff_info);
    if (ret != 0) {
        htd_spim_close(p_handle);
        m_bus_handle = NULL;
        print_error("get flash info error. ret=%d", ret);
        return ret;
    }

    m_bus_handle = p_handle;
    ASSERT(sff_info.page_size <= 256);
    m_norflash.capacity = sff_info.capacity;
    m_norflash.page_size = sff_info.page_size;
    m_norflash.erase_unit = sff_info.erase_unit;
    m_norflash.erase_cmd = sff_info.erase_cmd;
    m_norflash.mf_id = sff_info.mf_id;
    m_norflash.type_id = sff_info.type_id;
    m_norflash.write_unit = sff_info.write_unit; 
    
    // 判断是否使能4字节地址模式
    norflash_4byte_addr_mode_set(m_bus_handle, m_norflash.capacity >= 0x1000000);
    // 使能写, 部分芯片出厂时进入写保护模式，需要清除掉，才能写入
    norflash_program_protect_disable(m_bus_handle, m_norflash.mf_id);
    // 显示信息
    drv_norflash_info_show();
    return 0;
}

// @brief 注册到上层应用中
static int32_t drv_norflash_register(void)
{
    htd_memset(&m_norflash, 0, sizeof(dev_norflash_t));
    m_norflash.p_name = "norflash_spi";
    m_norflash.erase_unit = 4096;
    m_norflash.page_size = 256;
    m_norflash.write_cmd = 0x03;
    m_norflash.write_handle = drv_norflash_write;
    m_norflash.read_handle = drv_norflash_read;
    m_norflash.ctrl_handle = drv_norflash_ctrl;
    m_norflash.init_handle = drv_norflash_init;
    return htd_dev_norflash_register(&m_norflash);
}
DEVICE_INIT_FUNC_REGISTER(drv_norflash_register);



//////////////////////////////////////////////////////////////////////
#if (defined(HTD_CONSOLE_ENABLE) && (DEBUG_LVL > 0) && 1)
#include "htd_console_process.h"

// @brief 调试命令
static int norflash_cmd(int argc, char **argv)
{
    const static uint32_t rwdata_len = 256;
    static uint32_t m_addr = 0;
    if (argc < 2) {
        print_error("Usage: %s open/erase/write/read/close", argv[0]);
        return 0;
    }
    
    if (0 == htd_memcmp(argv[1], "open", 5)) {
        drv_norflash_init();
        
    } else if (0 == htd_memcmp(argv[1], "erase", 6)) {
        // 擦除
        uint32_t page_id = atoi(argv[2]);
        m_addr = page_id * m_norflash.erase_unit;
        drv_norflash_erase(m_addr, m_norflash.erase_unit);
        print_warning("erase finish. addr=0x%08x, page_id=%d", m_addr, page_id);
        
    } else if (0 == htd_memcmp(argv[1], "write", 6)) {
        // 写入数据
        uint8_t *p_data = (uint8_t*)htd_malloc(rwdata_len);
        ASSERT(p_data);
        //htd_memset(p_data, 0xa5, rwdata_len);
        for (uint32_t i=0; i<rwdata_len; ++i) {p_data[i] = i & 0x1f;}
        drv_norflash_write(m_addr, p_data, rwdata_len);
        htd_free(p_data);
        print_warning("write finish. addr=0x%08x", m_addr);
        
    } else if (0 == htd_memcmp(argv[1], "read", 5)) {
        uint8_t *p_data = (uint8_t*)htd_malloc(rwdata_len);
        ASSERT(p_data);
        htd_memset(p_data, 0, rwdata_len);
        drv_norflash_read(m_addr, p_data, rwdata_len);
        print_warning("read finish. ");
        print_hex("data", p_data, rwdata_len);
        htd_free(p_data);

    } else if (0 == htd_memcmp(argv[1], "close", 6)) {
        
        
    } else {
        print_error("Usage: %s open/erase/write/read/close", argv[0]);
    }
    return 0;
}
CONSOLE_CMD_EXPORT(norflash_cmd, norflash);
#endif /*(defined(HTD_CONSOLE_ENABLE) && (DEBUG_LVL > 0) && 1)*/
#endif  /*(!defined(_WIN32)) && defined(BSP_USING_SPIM)*/

