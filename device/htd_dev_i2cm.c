#define DEBUG_LVL           1       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG           "dev.i2cm"
#include "htd_device.h"
#include "htd_dev_i2cm.h"
#include "htd_components.h"


//////////////////////////////////////////////////////////////////////
HTD_SECTION_DEF(HTD_I2CM_SECTION_NAME, i2cm_dev_t);
static const i2cm_dev_t* htd_i2cm_dev_find(const char* p_name)
{
    const uint32_t* start = (uint32_t*)HTD_SECTION_START_ADDR(HTD_I2CM_SECTION_NAME);
    const uint32_t* end = (uint32_t*)HTD_SECTION_END_ADDR(HTD_I2CM_SECTION_NAME);

    const i2cm_dev_t* p_dev;
    uint32_t name_len = strlen(p_name) + 1;
    const uint32_t* p_item = htd_section_first_item_get(start, end);
    while (p_item) {
        p_dev = (i2cm_dev_t*)p_item;
        if (p_dev->magic == HTD_I2CM_DEV_MAGIC) {
            if (memcmp(p_name, p_dev->p_name, name_len) == 0) {
                return p_dev;
            }
        }
        p_item = htd_section_next_item_get(p_item, end, sizeof(i2cm_dev_t));
    }
    return NULL;
}

// @brief 打开I2C, 如果该设备已经打开了，参数以最先打开的为准
i2cm_handle_t* htd_i2cm_open(const char* p_name, const i2cm_user_config_t* p_user_config)
{
    const i2cm_dev_t* p_dev = htd_i2cm_dev_find(p_name);
    if (p_dev == NULL) {
        print_error("can't find i2c device. p_name=%s", p_name);
        return NULL;
    }

    // 创建新设备
    i2cm_handle_t* p_handle = (i2cm_handle_t*)htd_malloc(sizeof(i2cm_handle_t));
    ASSERT(p_handle);
    htd_memset(p_handle, 0, sizeof(i2cm_handle_t));
    p_handle->p_dev = p_dev;
    p_handle->p_user_config = p_user_config;

    // 打开设备
    print_warning("i2c open. %s. pin_sck=%d, pin_sda=%d", p_name, p_handle->p_user_config->pin_sck, p_handle->p_user_config->pin_sda);
    int32_t ret = p_dev->open_handle(p_handle);
    if (ret != 0) {
        htd_free(p_handle);
        print_error("i2cm open failed. p_name=%s", p_name);
        return NULL;
    }
    return p_handle;
}

// @brief 关闭I2C, 引脚默认会配置为模拟输入
int32_t htd_i2cm_close(i2cm_handle_t* p_handle)
{
    if (p_handle) {
        if (p_handle->p_dev) {
            if (ERROR_CODE_SUCCESS == p_handle->p_dev->close_handle(p_handle)) {
                // 引脚设置为高阻态(保持引脚处于低电平状态)
                if (PIN_NUM_INVALID != p_handle->p_user_config->pin_sck) {
                    htd_pin_input_analog_set(p_handle->p_user_config->pin_sck);
                }
                if ((PIN_NUM_INVALID != p_handle->p_user_config->pin_sda)) {
                    htd_pin_input_analog_set(p_handle->p_user_config->pin_sda);
                }
            }
        }
        htd_free(p_handle);
    }
    return 0;
}

// @brief 交互数据(从机地址为7bit有效地址，没有读写位)
int32_t htd_i2cm_xfer(i2cm_handle_t* p_handle, i2cm_msg_t* msg, uint32_t msg_count, uint32_t timeout_ms)
{
    ASSERT(p_handle);
    ASSERT(p_handle->p_dev);

    int32_t ret = 0;
    if (p_handle->p_dev->xfer_handle) {
        return p_handle->p_dev->xfer_handle(p_handle, msg, msg_count, timeout_ms);
    }
    return ret;
}

// @brief 写数据(从机地址为7bit有效地址，没有读写位)
int32_t htd_i2cm_write(i2cm_handle_t* p_handle, uint16_t slave_addr, const uint8_t* p_data, uint16_t data_size)
{
    ASSERT(p_handle);
    ASSERT(p_data);
    ASSERT(data_size);

    i2cm_msg_t msgs;
    htd_memset(&msgs, 0, sizeof(msgs));
    msgs.slave_addr = slave_addr;
    msgs.flags = I2CM_MSG_FLAG_TX|I2CM_MSG_FLAG_BLOCK; 
    if (slave_addr > 0xff) {
        msgs.flags |= I2CM_MSG_FLAG_ADDR_10BIT;
    }
    msgs.buff = (uint8_t*)p_data; 
    msgs.len = data_size;
    if (htd_i2cm_xfer(p_handle, &msgs, 1, 3000) == 1) {
        return 0;
    }
    
    print_error("i2c write error. slave_addr=0x%04x, data_size=%d", slave_addr, data_size);
    return -1;
}

// @brief 读数据(从机地址为7bit有效地址，没有读写位)
int32_t htd_i2cm_read(i2cm_handle_t* p_handle, uint16_t slave_addr, uint8_t* p_data, uint16_t data_size)
{
    ASSERT(p_handle);
    ASSERT(p_data);
    ASSERT(data_size);

    i2cm_msg_t msgs;
    msgs.slave_addr = slave_addr;
    msgs.flags = I2CM_MSG_FLAG_RX|I2CM_MSG_FLAG_BLOCK; 
    if (slave_addr > 0xff) {
        msgs.flags |= I2CM_MSG_FLAG_ADDR_10BIT;
    }
    msgs.buff = p_data;
    msgs.len = data_size;
    if (htd_i2cm_xfer(p_handle, &msgs, 1, 3000) == 1) {
        return 0;
    }
    print_error("i2c read error. slave_addr=0x%04x, data_size=%d", slave_addr, data_size);
    return -1;
}

// @brief 读数据, 成功返回0，失败返回-1(从机地址为7bit有效地址，没有读写位)
int32_t htd_i2cm_reg8_read(i2cm_handle_t* p_handle, uint16_t slave_addr, uint8_t reg, uint8_t* p_data, uint16_t data_size)
{
    ASSERT(p_handle);
    if (p_data == NULL || data_size == 0) {
        print_error("slave_addr=0x%04x, reg=%02x, p_data=%p, data_size=%d", slave_addr, reg, p_data, data_size);
        return -1;
    }

    uint32_t msg_count = 0;
    uint8_t buff[2];
    buff[0] = reg;
    
    i2cm_msg_t msgs[2];
    msgs[msg_count].slave_addr = slave_addr; 
    msgs[msg_count].flags = I2CM_MSG_FLAG_TX|I2CM_MSG_FLAG_BLOCK; 
    if (slave_addr > 0xff) {
        msgs[msg_count].flags |= I2CM_MSG_FLAG_ADDR_10BIT;
    }
    msgs[msg_count].buff = buff;
    msgs[msg_count].len = 1;
    msg_count++;
    msgs[msg_count].slave_addr = slave_addr;
    msgs[msg_count].flags = I2CM_MSG_FLAG_RX|I2CM_MSG_FLAG_BLOCK;
    if (slave_addr > 0xff) {
        msgs[msg_count].flags |= I2CM_MSG_FLAG_ADDR_10BIT;
    }
    msgs[msg_count].buff = p_data;
    msgs[msg_count].len = data_size;
    msg_count++;
    if (htd_i2cm_xfer(p_handle, msgs, msg_count, 3000) == msg_count) {
        return 0;
    } 
    
    print_error("i2c read error. slave_addr=0x%04x, reg=0x%02x, data_size=%d", slave_addr, reg, data_size);
    return -1;
}

// @brief 写数据(从机地址为7bit有效地址，没有读写位)
int32_t htd_i2cm_reg8_write(i2cm_handle_t* p_handle, uint16_t slave_addr, uint8_t reg, const uint8_t* p_data, uint16_t data_size)
{
    ASSERT(p_handle);
    ASSERT(p_data);
    ASSERT(data_size);

    uint8_t buff[64];
    buff[0] = reg;
    htd_memcpy((buff + 1), p_data, data_size);
    data_size++;
    ASSERT(data_size < sizeof(buff));
    
    i2cm_msg_t msgs;
    htd_memset(&msgs, 0, sizeof(msgs));
    msgs.slave_addr = slave_addr;
    msgs.flags = I2CM_MSG_FLAG_TX|I2CM_MSG_FLAG_BLOCK; 
    if (slave_addr > 0xff) {
        msgs.flags |= I2CM_MSG_FLAG_ADDR_10BIT;
    }
    msgs.buff = buff; 
    msgs.len = data_size;
    if (htd_i2cm_xfer(p_handle, &msgs, 1, 3000) == 1) {
        return 0;
    } 
    
    print_error("i2c write error. addr=0x%04x, reg=0x%02x, data_size=%d", slave_addr, reg, data_size);
    return -1;
}




//////////////////////////////////////////////////////////////////////
#if (defined(HTD_CONSOLE_ENABLE) && (DEBUG_LVL == 4) && 1)
// @brief 调试命令
static int i2c_list_cmd(int argc, char **argv)
{
    const uint32_t* start = (uint32_t*)HTD_SECTION_START_ADDR(HTD_I2CM_SECTION_NAME);
    const uint32_t* end = (uint32_t*)HTD_SECTION_END_ADDR(HTD_I2CM_SECTION_NAME);
    const i2cm_dev_t* p_dev;
    const uint32_t* p_item = htd_section_first_item_get(start, end);
    print_debug("have I2C device: ");
    while (p_item) {
        p_dev = (i2cm_dev_t*)p_item;
        if (p_dev->magic == HTD_I2CM_DEV_MAGIC) {
            print_debug("\t%s", p_dev->p_name);
        }
        p_item = htd_section_next_item_get(p_item, end, sizeof(i2cm_dev_t));
    }
    return 0;
}
CONSOLE_CMD_EXPORT(i2c_list_cmd, i2c_list);
#endif /**/


