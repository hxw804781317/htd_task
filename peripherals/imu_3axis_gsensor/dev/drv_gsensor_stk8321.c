// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL       1       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG       "3axis.stk832x"
#include "htd_common.h"
#include "htd_device.h"
#include "htd_gsensor_dev.h"
#include "drv_gsensor_stk832x.h"


#if defined(HTD_GSENSOR_USING_STK8321) && (HTD_GSENSOR_USING_STK8321 == 1)
//////////////////////////////////////////////////////////////////////
#include "htd_dev_i2cm.h"
static i2cm_handle_t* m_stk8321_bus = NULL;


// @brief 总线初始化
static int32_t stk8321_bus_init(void)
{
    static const i2cm_user_config_t i2c_config = {
        .pin_sck = HTD_GSENSOR_I2C_PIN_SCK,
        .pin_sda = HTD_GSENSOR_I2C_PIN_SDA,
        .MSB = 1,
        .freq_hz = 400*1000,
    };
    if (m_stk8321_bus == NULL) {
        m_stk8321_bus = htd_i2cm_open(HTD_GSENSOR_BUS_NAME, &i2c_config);
    }
    return 0;
}

// @brief 总线去初始化
static int32_t stk8321_bus_deinit(void)
{
    if (m_stk8321_bus) {
        htd_i2cm_close(m_stk8321_bus);
        m_stk8321_bus = NULL;
    }
    return 0;
}

// @brief 总线读数据
static int32_t stk8321_bus_data_read(uint8_t slave_addr, uint8_t reg, uint8_t* p_data, uint16_t data_size)
{
    return htd_i2cm_reg8_read(m_stk8321_bus, slave_addr, reg, p_data, data_size);
}

// @brief 写寄存器地址
static int32_t stk8321_register_write(uint8_t reg, uint8_t val)
{
    return htd_i2cm_reg8_write(m_stk8321_bus, STK832x_ADDR, reg, &val, 1);
}

// @brief 读寄存器
static int32_t stk8321_register_read(uint8_t reg, uint8_t* p_data)
{
    return sc7a20e_bus_data_read(STK832x_ADDR, reg, p_data, 1);
}

// @brief 上电控制
void stk8321_power_ctrl(bool enable)
{
#if ((defined HTD_GSENSOR_POWER_PIN) && (HTD_GSENSOR_POWER_PIN > 0))
    uint8_t level = !!enable;
  #if (!defined(HTD_GSENSOR_POWER_PIN_ACTIVE_HIGH))
    level = !level;
  #endif
    htd_pin_output_pp_set(HTD_GSENSOR_POWER_PIN);
    htd_pin_write(HTD_GSENSOR_POWER_PIN, level);
    print_warning("stk8321 power ctrl. enable=%d", enable);
    if (enable) {
        htd_sleep_ms(20);
    } 
#endif 
}

// @brief gsensor 寄存器初始化
static int32_t stk832x_register_init(uint8_t* p_chip_id)
{
    uint8_t stk8321_batch = 0;
    uint8_t stk8321_id = 0;

    //soft reset
    stk8321_register_write(STK832x_REG_0x14_SOFTRESET, 0xB6);
    htd_sleep_ms(20);
    stk8321_register_read(STK832x_REG_0x00_CHIPID, &stk8321_id);
    if (stk8321_id != STK832x_CHIP_ID_VAL) {
        print_error("gsensor read chip id failed.");
        return -1;
    }
    if (p_chip_id) {
        *p_chip_id = stk8321_id;
    }

#if (HTD_GSENSOR_SAMPLE_FREQ == 50)
    stk8321_register_write(STK832x_REG_0x11_POWER_MODE, STK832x_SUSPEND_BIT);             //‭ suspend mode enable
    stk8321_register_write(STK832x_REG_0x0F_G_RANGE, STK832x_SENSING_RANGE_4G);           // range:4g 0x05
    stk8321_register_write(STK832x_REG_0x10_BANDWIDTH, STK832X_BANDWIDTH_250HZ);
    stk8321_register_write(STK832x_REG_0x5E_RES_1, 0xC0);                                  //low-power 1000HZ (ORD=1000HZ/10ms=100)
    stk8321_register_write(STK832x_REG_0x3D_FIFOCFG1, STK8321_TOTAL_FIFO_LENGTH);
    stk8321_register_write(STK832x_REG_0x3E_FIFOCFG2, STK832x_INIT_MODE);
    stk8321_register_write(STK832x_REG_0x19_INTMAP1, 0);
    stk8321_register_write(STK832x_REG_0x1A_INTMAP2, 0);
    stk8321_register_write(STK832x_REG_0x34_INTFCFG, 0);
    stk8321_register_write(STK832x_REG_0x20_INTCFG1, STK832x_INT1_LV_BIT);
    stk8321_register_write(STK832x_REG_0x17_INTEN2, 0); 

    stk8321_register_read(STK832x_REG_0x70_RES_3, &stk8321_batch);
    if (stk8321_batch == 0xE1) {
        stk8321_register_write(0x33, 0x5A);
        print_warning("stk8321 is new batch");
    } else if (stk8321_batch == 0xE0) {
        print_warning("stk8321 is old batch");
    } else {
        print_warning("read stk8321 batch flag error. %02x", stk8321_batch);
    } 
    stk8321_register_write(STK832x_REG_0x11_POWER_MODE, 0x74);

#else
    uint8_t val = 0;
    stk8321_register_write(STK832x_REG_0x11_POWER_MODE, 0x80);     //‭suspend mode enable
    stk8321_register_write(0x0F, 0x05);     //range:4g 0x05
    stk8321_register_write(0x10, 0x0D);     //0x0F bandwidth:1000Hz //0x0D bandwidth:250Hz //0x0C bandwidth:125Hz
    
    stk8321_register_write(0x5E, 0xC0);     //low-power 1000HZ (ORD=1000HZ/10ms=100)
    htd_sleep_ms(5);
    stk8321_register_write(0x3D, 0x20);     //FIFO watermark level:32
    stk8321_register_write(0x3E, 0xC8);     //stream mode and store interval 4 (ODR=100 and FIFO ODR=100/4=25Hz)
    stk8321_register_write(0x19, 0x00);     //Do not map interrupt to INT1/INT2
    stk8321_register_write(0x1A, 0x00);     //Do not map interrupt to INT1/INT2
    stk8321_register_write(0x34, 0x00);     //4-wire SPI interface used  Disable I2C watchdog timer
    stk8321_register_write(0x20, 0x01);     //INT1 active level selection:Active high
    stk8321_register_write(0x17, 0x00);     //Disable all interrupt
    
    stk8321_register_read(0x45, &val);
    val += STK8321_ODR_6HZ;
    stk8321_register_write(0x45, val);

    stk8321_register_read(0x70, &stk8321_batch);
    if (stk8321_batch == 0xE1) {
        stk8321_register_write(0x33, 0x5A); //set SPI for primary interface
        print_debug("stk8321 is new batch. batch=%02x, chip_id=%02x", stk8321_batch, stk8321_id);
    } else if(stk8321_batch == 0xE0) {
        print_debug("stk8321 is old batch. batch=%02x, chip_id=%02x", stk8321_batch, stk8321_id);
    } else {
        print_debug("read stk8321 batch flag error. %02x", stk8321_batch);
    }
    stk8321_register_write(STK832x_REG_0x11_POWER_MODE, 0x74);     //low-power mode enable. equidistant sampling Sleep time duration:10ms
#endif
    return 0;
}

// @brief 初始化
int32_t drv_stk8321_init(uint8_t* p_chip_id)
{
    int32_t err_code;
    // 上电
    stk8321_power_ctrl(true);
    // 初始化硬件接口
    stk8321_bus_init(400*1000);
    err_code = stk832x_register_init(p_chip_id);
    return err_code;
}

// @brief 关闭
void drv_stk8321_uninit(bool is_power_off)
{
    if (is_power_off) {
        stk8321_power_ctrl(false);
    } else {
        // 进入低功耗
        stk8321_bus_init(400*1000);
        stk8321_register_write(0x11, 0x80);
    }
}

// @brief 读取缓冲数据
void drv_stk8321_fifo_data_get(gsensor_val_t *p_xyz, uint8_t *count)
{
    uint8_t fifo_len;
    uint8_t i;
    uint8_t data[6];

    ASSERT(p_xyz);
    i = *count;
    ASSERT(i > 0 && i <= 32);
    i = MIN(i, 32);
    
    // 读取数据长度
    stk8321_register_read(STK832x_REG_0x0C_FIFOSTS, &fifo_len);
    
    print_debug("fifo_len=%d, i=%d", fifo_len, i);
    fifo_len = MIN(i, fifo_len) * sizeof(gsensor_val_t);
    if (fifo_len > 0) {
        stk8321_bus_data_read(STK832x_ADDR, STK832x_REG_0x3F_FIFODATA, (uint8_t*)p_xyz, fifo_len);
        
        // 数据转换
        fifo_len /= sizeof(gsensor_val_t);
        for (i=0; i<fifo_len; ++i) {
            memcpy(data, &p_xyz[i], sizeof(gsensor_val_t));
            p_xyz[i].ax = ((int16_t)(data[1] << 8 | (data[0] & 0xF0))) >> 2;
            p_xyz[i].ay = ((int16_t)(data[3] << 8 | (data[2] & 0xF0))) >> 2;
            p_xyz[i].az = ((int16_t)(data[5] << 8 | (data[4] & 0xF0))) >> 2;
            print_debug("gsensor[%d] x=%d y=%d z=%d", i, p_xyz[i].ax, p_xyz[i].ay, p_xyz[i].az);
        }
    } else {
        i = 0;
    }
    *count = i;
}

// @brief 控制句柄
int32_t drv_stk8321_ctrl(htd_gsensor_cmd_t cmd, void* p_param, uint32_t param_len)
{
    if (cmd == HTD_GSENSOR_CMD_FIFO_DATA_READ) {
        uint8_t count = param_len / sizeof(gsensor_val_t);
        drv_stk8321_fifo_data_get((gsensor_val_t*) p_param, &count);
        return count * sizeof(gsensor_val_t);
    }
    return 0;
}

// @brief 注册到上层应用中
static int32_t drv_stk8321_register(void)
{
    const static htd_gsensor_dev_t mems_dev = {
        .chip_name = "stk8321",
        .fifo_max_size = 32,
        .init = drv_stk8321_init,
        .uninit = drv_stk8321_uninit,
        .ctrl_handle = drv_stk8321_ctrl,
    };
    return htd_gsensor_dev_register(&mems_dev);
}
DEVICE_INIT_FUNC_REGISTER(drv_stk8321_register);
#endif /*defined(HTD_GSENSOR_USING_STK8321) && (HTD_GSENSOR_USING_STK8321 == 1)*/

