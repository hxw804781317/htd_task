// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL       1       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG       "3axis.sc7a20h"
#include "htd_common.h"
#include "htd_device.h"
#include "htd_gsensor_dev.h"


#if defined(HTD_GSENSOR_USING_SC7A20H) && (HTD_GSENSOR_USING_SC7A20H == 1)
//////////////////////////////////////////////////////////////////////
// 芯片ID
#define SC7A20H_CHIP_ID_VAL                 0x11
// I2C 地址
#define SC7A20H_SDO_VDD_GND                 0
#if SC7A20H_SDO_VDD_GND == 0
    #define SC7A20H_ADDR                    0x18
    #define SC7A20H_IIC_8BITS_WRITE_ADDR    0x30
    #define SC7A20H_IIC_8BITS_READ_ADDR     0x31
#else
    #define SC7A20H_ADDR                    0x19
    #define SC7A20H_IIC_8BITS_WRITE_ADDR    0x32
    #define SC7A20H_IIC_8BITS_READ_ADDR     0x33
#endif



//////////////////////////////////////////////////////////////////////
#include "htd_dev_i2cm.h"
static i2cm_handle_t* m_sc7a20h_bus = NULL;


// @brief 总线初始化
static int32_t sc7a20h_bus_init(void)
{
    static const i2cm_user_config_t i2c_config = {
        .pin_sck = HTD_GSENSOR_I2C_PIN_SCK,
        .pin_sda = HTD_GSENSOR_I2C_PIN_SDA,
        .MSB = 1,
        .freq_hz = 400*1000,
    };
    if (m_sc7a20h_bus == NULL) {
        m_sc7a20h_bus = htd_i2cm_open(HTD_GSENSOR_BUS_NAME, &i2c_config);
    }
    return 0;
}

// @brief 总线去初始化
static int32_t sc7a20h_bus_deinit(void)
{
    if (m_sc7a20h_bus) {
        htd_i2cm_close(m_sc7a20h_bus);
        m_sc7a20h_bus = NULL;
    }
    return 0;
}

// @brief 总线读数据
static int32_t sc7a20h_bus_data_read(uint8_t slave_addr, uint8_t reg, uint8_t* p_data, uint16_t data_size)
{
    return htd_i2cm_reg8_read(m_sc7a20h_bus, slave_addr, reg, p_data, data_size);
}

// @brief 写寄存器地址
static int32_t sc7a20h_register_write(uint8_t reg, uint8_t val)
{
    return htd_i2cm_reg8_write(m_sc7a20h_bus, SC7A20H_ADDR, reg, &val, 1);
}

// @brief 读寄存器
static int32_t sc7a20h_register_read(uint8_t reg, uint8_t* p_data)
{
    return sc7a20h_bus_data_read(SC7A20H_ADDR, reg, p_data, 1);
}

// @brief 上电控制
static void sc7a20h_power_ctrl(bool enable)
{
#if ((defined HTD_GSENSOR_POWER_PIN) && (HTD_GSENSOR_POWER_PIN > 0))
    uint8_t level = !!enable;
  #if (!defined(HTD_GSENSOR_POWER_PIN_ACTIVE_HIGH))
    level = !level;
  #endif
    htd_pin_output_pp_set(HTD_GSENSOR_POWER_PIN);
    htd_pin_write(HTD_GSENSOR_POWER_PIN, level);
    print_warning("sc7a2x power ctrl. enable=%d", enable);
    if (enable) {
        htd_sleep_ms(20);
    } 
#endif 
}

// @brief 初始化，启动设备
static int32_t drv_sc7a20h_init(uint8_t* p_chip_id)
{
    // 上电
    sc7a20h_power_ctrl(true);
    // 初始化硬件接口
    sc7a20h_bus_init();
    
    // 读取芯片id
    uint8_t id = 0;
    for (int i=0; i<3; ++i) {
        id = 0;
        sc7a20h_register_read(0x0F, &id);
        if (id == SC7A20H_CHIP_ID_VAL) {
            break;
        }
    }
    if (id != SC7A20H_CHIP_ID_VAL) {
        print_warning("sc7a2x register init failed. sc7a20_id=%02x", id);
        return -1;
    }
    if (p_chip_id) {
        *p_chip_id = id;
    }

    /*sc7a20h_register_write(0x1f, 0x00);    // 高性能模式
    sc7a20h_register_write(0x23, 0x98);      // 0b01-4G  BDU-en  DLPF_EN-en
    sc7a20h_register_write(0x2E, 0x00);      // BY PASS MODE  
    sc7a20h_register_write(0x21, 0x68);      // 使能内部滤波，

    //sc7a20h_register_write(0x24, 0x80);    // 锁存中断输出电平状态
    //sc7a20h_register_write(0x25, 0x02);    // 中断输出高电平，默认低电平
    sc7a20h_register_write(0x0E, 0x00);      // 使能fifodata地址不自增

    sc7a20h_register_write(0x2E, 0x9F);      // Stream mode and fth=0x0f
    sc7a20h_register_write(0x20, 0x37);      // 25hz
    //sc7a20h_register_write(0x20, 0x47);    // 50hz
    //sc7a20h_register_write(0x58, 0x02);    // z轴方向 */
    sc7a20h_register_write(0x24,0x80);
    htd_delay_ms(10);
    sc7a20h_register_write(0x2E,0x00);
    sc7a20h_register_write(0x1f,0x00);
    sc7a20h_register_write(0x20,0x47);
    sc7a20h_register_write(0x21,0x00);
    sc7a20h_register_write(0x23,0x98);
    sc7a20h_register_write(0x24,0x40);
    sc7a20h_register_write(0x2E,0x9F);
    sc7a20h_register_write(0x57,0x0c);        // 关闭LD0和I2C内部上拉电阻，使用外部电阻电平
    return 0;
}

// @brief 去初始化，设备处于低功耗状态
static void drv_sc7a20h_uninit(bool is_power_off)
{
    print_warning("gsensor sc7a2x uninit. is_power_off=%d", is_power_off);
    
    if (is_power_off) {
        sc7a20h_bus_deinit();
        sc7a20h_power_ctrl(false);
    } else {
        sc7a20h_register_write(0x1f,0x00);
        sc7a20h_register_write(0x20,0x00);
        htd_delay_ms(1);
        sc7a20h_bus_deinit();
        sc7a20h_power_ctrl(false);
    }
}

// @brief 读取缓冲数据
static void drv_sc7a20h_fifo_data_get(gsensor_val_t *p_xyz, uint8_t *count)
{
    uint8_t fifo_len;
    uint32_t i;
    uint8_t data[sizeof(gsensor_val_t)*32];
    uint8_t* p_data;

    ASSERT(p_xyz);
    i = *count;
    *count = 0;
    ASSERT(i > 0 && i <= 32);

    // 读取数据长度
    sc7a20h_register_read(0x2f, &fifo_len);
    if (fifo_len & 0x40) {
        fifo_len = 32;
    } else {
        fifo_len = fifo_len&0x1f; 
    }
    
    //uint8_t val;
    //sc7a20h_register_read(0x21, &val);
    //print_debug("fifo_len=%d, i=%d", fifo_len, i);
    
    p_data = data;
    if (fifo_len > 0) {       
        sc7a20h_bus_data_read(SC7A20H_ADDR, 0x69, (uint8_t*)p_data, 6*fifo_len);
        for (i=0; i<fifo_len; ++i) {
            //sc7a20h_bus_data_read(SC7A20H_ADDR, 0xa8, (uint8_t*)p_data, 6);
            p_xyz[i].ax = ((int16_t)p_data[0]<<8 | p_data[1]);
            p_xyz[i].ay = ((int16_t)p_data[2]<<8 | p_data[3]);
            p_xyz[i].az = ((int16_t)p_data[4]<<8 | p_data[5]);
            p_xyz[i].ax >>= 4;
            p_xyz[i].ay >>= 4;
            p_xyz[i].az >>= 4;
            //print_debug("data: 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x\n\r", p_data[0], p_data[1], p_data[2], p_data[3], p_data[4], p_data[5]);
            //print_debug("gsensor[%d] x=%d y=%d z=%d\n", i, p_xyz[i].x, p_xyz[i].y, p_xyz[i].z);
            p_data += 6;
        }
    } else {
        i = 0;
    }
    
    sc7a20h_register_write(0x2E, 0x00);      // BY PASS MODE
    sc7a20h_register_write(0x2E, 0x9F);      // Stream mode and fth=0x0f
    *count = i;
}

// @brief 控制句柄
static int32_t drv_sc7a20h_ctrl(htd_gsensor_cmd_t cmd, void* p_param, uint32_t param_len)
{
    if (cmd == HTD_GSENSOR_CMD_FIFO_DATA_READ) {
        uint8_t count = param_len / sizeof(gsensor_val_t);
        drv_sc7a20h_fifo_data_get((gsensor_val_t*) p_param, &count);
        return count * sizeof(gsensor_val_t);
    }
    return 0;
}

// @brief 注册到上层应用中
static int32_t drv_sc7a20h_register(void)
{
    const static htd_gsensor_dev_t mems_dev = {
        .chip_name = "sc7a20h",
        .fifo_max_size = 32,
        .init = drv_sc7a20h_init,
        .uninit = drv_sc7a20h_uninit,
        .ctrl_handle = drv_sc7a20h_ctrl,
    };
    return htd_gsensor_dev_register(&mems_dev);
}
DEVICE_INIT_FUNC_REGISTER(drv_sc7a20h_register);
#endif /*defined(HTD_GSENSOR_USING_SC7A20H) && (HTD_GSENSOR_USING_SC7A20H == 1)*/

