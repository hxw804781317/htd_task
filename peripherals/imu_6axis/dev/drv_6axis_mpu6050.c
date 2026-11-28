// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#define DEBUG_LVL       4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG       "6axis.mpu6050"
#include "htd_device.h"
#include "htd_6axis_dev.h"
#include "drv_6axis_mpu6050.h"


#if (defined(HTD_6AXIS_USING_MPU6050) && (HTD_6AXIS_USING_MPU6050 == 1))
//////////////////////////////////////////////////////////////////////
#include "htd_dev_i2cm.h"
static i2cm_handle_t* m_mpu6050_bus = NULL;

// @brief 总线初始化
static int32_t mpu6050_bus_init(void)
{
    static const i2cm_user_config_t i2c_config = {
        .pin_sck = (pin_num_t)HTD_6AXIS_I2C_PIN_SCK,
        .pin_sda = (pin_num_t)HTD_6AXIS_I2C_PIN_SDA,
        .MSB = 1,
        .freq_hz = 400*1000,
    };
    if (m_mpu6050_bus == NULL) {
        m_mpu6050_bus = htd_i2cm_open(HTD_6AXIS_BUS_NAME, &i2c_config);
    }
    return 0;
}

// @brief 总线去初始化
static int32_t mpu6050_bus_deinit(void)
{
    if (m_mpu6050_bus) {
        htd_i2cm_close(m_mpu6050_bus);
        m_mpu6050_bus = NULL;
    }
    return 0;
}

// @brief 总线读数据
static int32_t mpu6050_bus_data_read(uint8_t slave_addr, uint8_t reg, uint8_t* p_data, uint16_t data_size)
{
    return htd_i2cm_reg8_read(m_mpu6050_bus, slave_addr, reg, p_data, data_size);
}

// @brief 写寄存器地址
static int32_t mpu6050_register_write(uint8_t reg, uint8_t val)
{
    return htd_i2cm_reg8_write(m_mpu6050_bus, MPU6050_ADDRESS, reg, &val, 1);
}

// @brief 读寄存器
static int32_t mpu6050_register_read(uint8_t reg, uint8_t* p_data)
{
    return mpu6050_bus_data_read(MPU6050_ADDRESS, reg, p_data, 1);
}

// @brief 上电控制
static void mpu6050_power_ctrl(bool enable)
{
#if ((defined HTD_6AXIS_POWER_PIN) && (HTD_6AXIS_POWER_PIN > 0))
    uint8_t level = !!enable;
  #if (!defined(HTD_6AXIS_POWER_PIN_ACTIVE_HIGH))
    level = !level;
  #endif
    htd_pin_output_pp_set(HTD_6AXIS_POWER_PIN);
    htd_pin_write(HTD_6AXIS_POWER_PIN, level);
    print_warning("mpu6050 power ctrl. enable=%d", enable);
    if (enable) {
        htd_sleep_ms(20);
    } 
#endif 
}

// @brief 初始化，启动设备
static int32_t drv_mpu6050_init(uint8_t* chip_id)
{
    // 上电
    mpu6050_power_ctrl(true);
    // 初始化硬件接口
    mpu6050_bus_init();

    // 读取芯片id
    uint8_t who_am_i;
    for (int i=0; i<3; ++i) {
        who_am_i = 0;
        mpu6050_register_read(ADDRESS_WHO_AM_I, &who_am_i);
        if (who_am_i == MPU6050_WHO_AM_I) {
            break;
        }
    }
    if (who_am_i != MPU6050_WHO_AM_I) {
        print_error("mpu6050 read chip id failed. chip_id=0x%02x", who_am_i);
        return -1;
    }
    if (chip_id) {
        *chip_id = who_am_i;
    }
    print_debug("mpu6050 chip_id: 0x%02x", who_am_i);
    
    //唤醒MPU6050
    mpu6050_register_write(MPU_PWR_MGMT1_REG, 0x00); 
    // 设置采样率(Sample Rate = Gyroscope Output Rate / (1 + SMPLRT_DIV))：1KHz 
    mpu6050_register_write(MPU_SAMPLE_RATE_REG , 0x07); 
    // 设置低通滤波器，截止频率是1K，带宽是5K
    mpu6050_register_write(MPU_CFG_REG, 0x06); 
    // 关闭中断
    mpu6050_register_write(MPU_INT_EN_REG, 0x00);
    // 陀螺仪自检及测量范围，典型值：0x18(不自检，2000deg/s)
    mpu6050_register_write(MPU_GYRO_CFG_REG , 0x18); 
    //配置加速度传感器量程 +-2G s，不自检
    mpu6050_register_write(MPU_ACCEL_CFG_REG, 0x00);
    return 0;
}

// @brief 去初始化，设备处于低功耗状态
static void drv_mpu6050_uninit(bool is_power_off)
{
    print_warning("mpu6050 uninit. is_power_off=%d", is_power_off);
    if (is_power_off) {
        mpu6050_bus_deinit();
        mpu6050_power_ctrl(false);
    } else {
        // 进入低功耗
        mpu6050_register_write(0x11, 0x80);
        htd_delay_ms(1);
        mpu6050_bus_deinit();
    }
}

// @brief 读陀螺仪数据
bool mpu6050_gyro_read(htd_6axis_val_t* p_imu)
{
    uint8_t buff[6]; 
    if (mpu6050_bus_data_read(MPU6050_ADDRESS, MPU6050_GYRO_OUT, buff, 6) == 0) {
        p_imu->gx = (buff[0] << 8) | buff[1];
        if (p_imu->gx & 0x8000) {
            p_imu->gx -= 65536;
        }
        p_imu->gy= (buff[2] << 8) | buff[3];
        if (p_imu->gy & 0x8000) {
            p_imu->gy -= 65536;
        }
        p_imu->gz = (buff[4] << 8) | buff[5];
        if (p_imu->gz & 0x8000) {
            p_imu->gz -= 65536;
        }
        return true;
    }
    return false;
}

// @brief 读取加速度数据
bool mpu6050_acc_read(htd_6axis_val_t* p_imu)
{
    uint8_t buff[6];
    if (mpu6050_bus_data_read(MPU6050_ADDRESS, MPU6050_ACC_OUT, buff, 6) == 0) {
        mpu6050_bus_data_read(MPU6050_ADDRESS, MPU6050_ACC_OUT, buff, 6);
        p_imu->ax = (buff[0] << 8) | buff[1];
        if (p_imu->ax & 0x8000) {
            p_imu->ax -= 65536;
        }
        p_imu->ay= (buff[2] << 8) | buff[3];
        if (p_imu->ay & 0x8000) {
            p_imu->ay -= 65536;
        }
        p_imu->az = (buff[4] << 8) | buff[5];
        if (p_imu->az & 0x8000) {
            p_imu->az -= 65536;
        }
        return true;
    }
    return false;
}

// @brief 读取数据
static uint32_t mpu6050_fifo_data_read(htd_6axis_val_t* p_imu, uint32_t data_count)
{
    htd_6axis_val_t* p_start = p_imu;
    while (data_count--) {
        if (!mpu6050_acc_read(p_imu)) {
            break;
        }
        if (!mpu6050_gyro_read(p_imu)) {
            break;
        }
        p_imu++;
    }
    return (p_imu - p_start) * sizeof(htd_6axis_val_t);
}


// @brief 控制句柄
int32_t drv_mpu6050_ctrl(htd_6axis_cmd_t cmd, void* p_param, uint32_t param_len)
{
    if (cmd == HTD_6AXIS_CMD_FIFO_DATA_READ) {
        return mpu6050_fifo_data_read((htd_6axis_val_t*)p_param, param_len/sizeof(htd_6axis_val_t));
    }
    return 0;
}

// @brief 注册到上层应用中
static int32_t drv_mpu6050_register(void)
{
    const static htd_6axis_dev_t m_6axis_dev = {
        .chip_name = "mpu6050",
        .fifo_max_size = 32,
        .init = drv_mpu6050_init,
        .uninit = drv_mpu6050_uninit,
        .ctrl_handle = drv_mpu6050_ctrl,
    };
    return htd_6axis_dev_register(&m_6axis_dev);
}
DEVICE_INIT_FUNC_REGISTER(drv_mpu6050_register);
#endif /*(defined(HTD_6AXIS_USING_MPU6050) && (HTD_6AXIS_USING_MPU6050 == 1))*/

