// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
//#define DEBUG_LVL       4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG       "sfw.i2cm"
#include "htd_device.h"
#include "htd_dev_i2cm.h"


#if (defined(BSP_USING_SFW_I2CM0) || defined(BSP_USING_SFW_I2CM1) || defined(BSP_USING_SFW_I2CM2) || defined(BSP_USING_SFW_I2CM3))
//////////////////////////////////////////////////////////////////////
#define I2C_READ_BIT                    (0x01)
enum {
#ifdef BSP_USING_SFW_I2CM0
    I2CM0_SFW_INDEX,
#endif
#ifdef BSP_USING_SFW_I2CM1
    I2CM1_SFW_INDEX,
#endif
#ifdef BSP_USING_SFW_I2CM2
    I2CM2_SFW_INDEX,
#endif
#ifdef BSP_USING_SFW_I2CM3
    I2CM3_SFW_INDEX,
#endif
    I2CM_SFW_COUNT,
};

typedef struct {
    i2cm_handle_t* who;
    uint32_t nop_count;
}i2c_master_t;
static i2c_master_t m_i2cm_sfw_object[I2CM_SFW_COUNT];


// @brief i2c 延迟
static void i2c_delay(const i2c_master_t* p_i2c)
{
    uint32_t count = p_i2c->nop_count;
    while (count--) {
        __ASM("nop");
    }
}

// @brief 拉高时钟线
static bool i2c_master_sck_high_level(const i2c_master_t* p_i2c)
{
    const i2cm_user_config_t* p_user_config = p_i2c->who->p_user_config;
    // 拉高 SCK
    htd_pin_write(p_user_config->pin_sck, 1); 
    i2c_delay(p_i2c);
    // 等待 SCK 已经拉高
    uint32_t volatile timeout_counter = 10;
    while (htd_pin_read(p_user_config->pin_sck) == 0) { 
        if (timeout_counter-- == 0) {
            // FIXME: 总线被锁住，需要检查连接
            print_error("sck is low level"); 
            return false;
        }
    }
    return true;
}

// @brief 发送开始信号
static bool i2c_master_issue_startcondition(const i2c_master_t* p_i2c)
{
    const i2cm_user_config_t* p_user_config = p_i2c->who->p_user_config;
    // SDA 拉高
    htd_pin_write(p_user_config->pin_sda, 1);
    i2c_delay(p_i2c);
    // 等待 SCL 拉高
    if (! i2c_master_sck_high_level(p_i2c)) {
        return false;
    }
    // SDA，SCL都拉低
    htd_pin_write(p_user_config->pin_sda, 0);
    i2c_delay(p_i2c);
    htd_pin_write(p_user_config->pin_sck, 0);
    i2c_delay(p_i2c);
    return true;
}

// @brief 发送结束信号
static bool i2c_master_issue_stopcondition(const i2c_master_t* p_i2c)
{
    const i2cm_user_config_t* p_user_config = p_i2c->who->p_user_config;
    htd_pin_write(p_user_config->pin_sda, 0);
    i2c_delay(p_i2c);
    if (! i2c_master_sck_high_level(p_i2c)) {
        return false;
    }
    htd_pin_write(p_user_config->pin_sda, 1);
    i2c_delay(p_i2c);
    return true;
}

// @brief 清除总线，可以不做
static bool i2c_master_clear_bus(const i2c_master_t* p_i2c)
{
    const i2cm_user_config_t* p_user_config = p_i2c->who->p_user_config;
    htd_pin_write(p_user_config->pin_sda, 1);
    htd_pin_write(p_user_config->pin_sck, 1); 
    i2c_delay(p_i2c);

    bool bus_clear;
    if (htd_pin_read(p_user_config->pin_sda) && htd_pin_read(p_user_config->pin_sck)) {
        bus_clear = true;
    } else if (htd_pin_read(p_user_config->pin_sck)) {
        bus_clear = false;
        for (uint8_t i=18; i--;) {
            htd_pin_write(p_user_config->pin_sck, 0);
            i2c_delay(p_i2c);
            htd_pin_write(p_user_config->pin_sck, 1); 
            i2c_delay(p_i2c);
            if (htd_pin_read(p_user_config->pin_sda)) {
                bus_clear = true;
                break;
            }
        }
    } else {
        bus_clear = false;
    }
    return bus_clear;
}

// @brief 发送一个字节
static bool i2c_master_clock_byte(const i2c_master_t* p_i2c, uint8_t databyte)
{
    const i2cm_user_config_t* p_user_config = p_i2c->who->p_user_config;
    bool transfer_succeeded = true;

    // SDA 切换为输出
    htd_pin_dir_set(p_user_config->pin_sda, HTD_PIN_DIR_OUTPUT);
    // MSB first
    for (uint8_t i=0x80; i != 0; i>>=1) {
        // 时钟拉低
        htd_pin_write(p_user_config->pin_sck, 0);
        i2c_delay(p_i2c);
        // 接收数据
        if (databyte & i) {
            htd_pin_write(p_user_config->pin_sda, 1);
        } else {
            htd_pin_write(p_user_config->pin_sda, 0);
        }
        // 时钟拉高
        if (! i2c_master_sck_high_level(p_i2c)) {
            transfer_succeeded = false; // Timeout
            break;
        }
    }
    // 发送完数据，拉低SCK
    htd_pin_write(p_user_config->pin_sck, 0);
    i2c_delay(p_i2c);
    
    // 切换 SDA 为输入
    htd_pin_dir_set(p_user_config->pin_sda, HTD_PIN_DIR_INPUT);
    i2c_delay(p_i2c);
    // 拉高 SCK 时钟线
    transfer_succeeded &= i2c_master_sck_high_level(p_i2c);
    // 读取SDA, 高电平为 NACK, 低电平则为ACK
    if (htd_pin_read(p_user_config->pin_sda)) {
        transfer_succeeded = false;
    }
    // 结束，拉低时钟线 SCK
    htd_pin_write(p_user_config->pin_sck, 0);
    i2c_delay(p_i2c);
    // SDA 切换为输出模式
    htd_pin_dir_set(p_user_config->pin_sda, HTD_PIN_DIR_OUTPUT);
    return transfer_succeeded;
}

// @brief 读取一个字节
static bool i2c_master_clock_byte_in(const i2c_master_t* p_i2c, uint8_t *databyte, bool ack)
{
    uint8_t byte_read = 0;
    bool transfer_succeeded = true;
    const i2cm_user_config_t* p_user_config = p_i2c->who->p_user_config;

    // SDA切换为输入
    htd_pin_dir_set(p_user_config->pin_sda, HTD_PIN_DIR_INPUT);
    // MSB first
    for (uint8_t i=0x80; i != 0; i>>=1) {
        if (!i2c_master_sck_high_level(p_i2c)) {
            transfer_succeeded = false;
            break;
        }
        if (htd_pin_read(p_user_config->pin_sda)) {
            byte_read |= i;
        }
        // 拉低时钟
        htd_pin_write(p_user_config->pin_sck, 0);
        // 适当延迟，等待对端接收ack
        i2c_delay(p_i2c);
    }

    // SDA切换为输出
    htd_pin_dir_set(p_user_config->pin_sda, HTD_PIN_DIR_OUTPUT);
    *databyte = (uint8_t)byte_read;
    
    // 发送ACK(SDA拉高为NACK；拉低为ACK)
    htd_pin_write(p_user_config->pin_sda, ack ? 0 : 1);
    i2c_delay(p_i2c);
    if (!i2c_master_sck_high_level(p_i2c)) {
        // 对端没有回复ack
        transfer_succeeded = false; // Timeout
        print_error("error: i2c not ack");
    }
    // ACK发送结束，拉低SCK
    htd_pin_write(p_user_config->pin_sck, 0);
    i2c_delay(p_i2c);
    return transfer_succeeded;
}

// @brief i2c 数据交互
static bool i2c_master_transfer(const i2c_master_t* p_i2c, uint8_t address, uint8_t* p_data, uint8_t data_length, bool issue_stop_condition)
{
    bool transfer_succeeded = i2c_master_issue_startcondition(p_i2c);
    if (transfer_succeeded == false) {
        // 总线连接有问题
        print_error("i2c bus error.");
        return false;
    }
    // 发送地址
    if (false == i2c_master_clock_byte(p_i2c, address)) {
        print_error("address NACK. address=0x%02x", address >> 1);
        return false;
    }
    // 收发数据
    //uint8_t* p_start = p_data;
    if (address & I2C_READ_BIT) {
        // 读取数据
        while (data_length-- && transfer_succeeded) {
            if (data_length == 0) {
                transfer_succeeded &= i2c_master_clock_byte_in(p_i2c, p_data, (bool)false);
            } else {
                transfer_succeeded &= i2c_master_clock_byte_in(p_i2c, p_data, (bool)true);
            }
            p_data++;
        }
    } else {
        // 发送数据
        while (data_length-- && transfer_succeeded) {
            transfer_succeeded &= i2c_master_clock_byte(p_i2c, *p_data);
            p_data++;
        }
    }
    if (issue_stop_condition || !transfer_succeeded) {
        transfer_succeeded &= i2c_master_issue_stopcondition(p_i2c);
    }
    //print_debug("txrx data_len=%d", p_data - p_start);
    return transfer_succeeded;
}

// @brief 打开
static int32_t sfw_i2cm_open(i2cm_handle_t* p_handle)
{
    ASSERT(p_handle);
    const i2cm_user_config_t* p_user_config = p_handle->p_user_config;
    i2c_master_t* p_object = (i2c_master_t*)p_handle->p_dev->p_object;
    if (p_object->who != NULL) {
        if (p_object->who == p_handle) {
            print_error("sfw i2cm(%s) is opened.", p_handle->p_dev->p_name);
            return -1;
        }
        ASSERT(p_object->who->p_user_config->pin_sda == p_user_config->pin_sda);
        ASSERT(p_object->who->p_user_config->pin_sck == p_user_config->pin_sck);
        ASSERT(p_object->who->p_user_config->freq_hz == p_user_config->freq_hz);
        return 0;
    } 
    
    p_object->who = p_handle;
    p_object->nop_count = htd_rcc_freq_hz_get() / p_user_config->freq_hz;
    p_object->nop_count = p_object->nop_count / 4;
    print_debug("open sfw i2c. pin_sck=%d, pin_sda=%d, nop_count=%d", p_user_config->pin_sck, p_user_config->pin_sda, p_object->nop_count);
    
    // 配置引脚为复用i2c模式
    htd_pin_afi2c_set(p_user_config->pin_sck);
    htd_pin_afi2c_set(p_user_config->pin_sda);
    // 输出高电平
    htd_pin_write(p_user_config->pin_sck, 1);
    htd_pin_write(p_user_config->pin_sda, 1);
    // 清除总线
    if (i2c_master_clear_bus(p_object)) {
        return 0;
    }
    // FIXME: 外部的上拉电阻出问题!!!!!!!!!
    print_error("open i2c master failed.");
    return -1;
}

// @brief 关闭
static int32_t sfw_i2cm_close(i2cm_handle_t* p_handle)
{
    if (p_handle == NULL) {
        return 0;
    }

    const i2cm_user_config_t* p_user_config = p_handle->p_user_config;
    // 将引脚设置为高阻态模式
    htd_pin_hiz_set(p_user_config->pin_sck);
    htd_pin_hiz_set(p_user_config->pin_sda);
    return 0;
}

// @brief 交互, slave_addr 为 7bit 地址，函数内部自动右移1bit
static int32_t sfw_i2cm_xfer(i2cm_handle_t* p_handle, i2cm_msg_t* msg, uint32_t msg_count, uint32_t timeout_ms)
{
    uint16_t slave_addr;
    i2c_master_t* p_object = (i2c_master_t*)p_handle->p_dev->p_object;
    i2cm_msg_t* p_start = msg;
    slave_addr = msg->slave_addr << 1;
    while (msg_count--) {
        ASSERT((msg->flags & I2CM_MSG_FLAG_ADDR_10BIT) == 0);
        if (msg->flags & I2CM_MSG_FLAG_RX) {
            i2c_master_transfer(p_object, slave_addr|I2C_READ_BIT, msg->buff, msg->len, msg_count == 0);
        } else {
            i2c_master_transfer(p_object, slave_addr, msg->buff, msg->len, msg_count == 0);
        }
        msg++;
    }
    return msg - p_start;
}

#if defined(BSP_USING_SFW_I2CM0)
    HTD_I2CM_DEV_REGISTER(sfw_i2cm0, sfw_i2cm_open, sfw_i2cm_close, sfw_i2cm_xfer, &m_i2cm_sfw_object[I2CM0_SFW_INDEX]);
#endif /*defined(BSP_USING_SFW_I2CM0)*/
#if defined(BSP_USING_SFW_I2CM1)
    HTD_I2CM_DEV_REGISTER(sfw_i2cm1, sfw_i2cm_open, sfw_i2cm_close, sfw_i2cm_xfer, &m_i2cm_sfw_object[I2CM1_SFW_INDEX]);
#endif /*defined(BSP_USING_SFW_I2CM1)*/
#if defined(BSP_USING_SFW_I2CM2)
    HTD_I2CM_DEV_REGISTER(sfw_i2cm2, sfw_i2cm_open, sfw_i2cm_close, sfw_i2cm_xfer, &m_i2cm_sfw_object[I2CM2_SFW_INDEX]);
#endif /*defined(BSP_USING_SFW_I2CM2)*/
#if defined(BSP_USING_SFW_I2CM3)
    HTD_I2CM_DEV_REGISTER(sfw_i2cm3, sfw_i2cm_open, sfw_i2cm_close, sfw_i2cm_xfer, &m_i2cm_sfw_object[I2CM3_SFW_INDEX]);
#endif /*defined(BSP_USING_SFW_I2CM3)*/
#endif /*(defined(BSP_USING_SFW_I2CM1) || defined(BSP_USING_SFW_I2CM2) || defined(BSP_USING_SFW_I2CM0))*/

