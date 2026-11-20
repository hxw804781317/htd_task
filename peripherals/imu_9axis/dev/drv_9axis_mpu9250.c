#define DEBUG_LVL       4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG       "9axis.mpu9250"
#include "htd_common.h"
#include "htd_9axis_dev.h"
#include "htd_device.h"
#include "drv_9axis_mpu9250.h"


#if (defined(HTD_9AXIS_USING_MPU9250) && (HTD_9AXIS_USING_MPU9250 == 1))
//////////////////////////////////////////////////////////////////////
static int16_t ak8963_asa[3];


// @brief 寄存器写
static void mpu9250_register_write(uint8_t reg, uint8_t byte)
{
    drv_9axis_register_write(reg, byte);
}
// @brief 寄存器读
static uint8_t mpu9250_register_read(uint8_t reg)
{
    return drv_9axis_register_read(reg);
}
// @brief 读取数据
static uint32_t mpu9250_data_read(uint8_t reg, uint8_t* p_data, uint16_t data_len)
{
    return drv_9axis_bus_data_read(reg, p_data, data_len);
}

// @brief ak8963 寄存器写入
// @return 成功返回写入的数据长度，失败返回0
void mpu9250_ak8963_register_write(uint8_t reg, uint8_t byte)
{
    // 选择 ak8963 
    mpu9250_register_write(MPU9250_0X25_I2C_SLV0_ADDR, AK8963_MAGN_ADDRESS);
    //ak8963_spi_delay();
    // 设置 ak8963 寄存器地址
    mpu9250_register_write(MPU9250_0X26_I2C_SLV0_REG, reg);
    //ak8963_spi_delay();
    // 设置 ak8963 寄存器值
    mpu9250_register_write(MPU9250_0X63_I2C_SLV0_DO, byte);
    // 使能 i2c 并开始发送数据
    mpu9250_register_write(MPU9250_0X27_I2C_SLV0_CTRL, 0x81);
}

// @brief ak8963 寄存器读取
// @return 成功返回读取的数据长度，失败返回0
uint8_t mpu9250_ak8963_register_read(uint8_t reg)
{
    // 选择 ak8963 
    mpu9250_register_write(MPU9250_0X25_I2C_SLV0_ADDR, AK8963_MAGN_ADDRESS|0x80);
    // 设置 ak8963 寄存器地址
    mpu9250_register_write(MPU9250_0X26_I2C_SLV0_REG, reg);
    // 使能 i2c 并开始读取数据到对应的缓存中
    mpu9250_register_write(MPU9250_0X27_I2C_SLV0_CTRL, 0x81);
    // 延迟等待数据读取完毕
    htd_delay_us(50);
    // 从对应缓存中读取数据
    return mpu9250_register_read(MPU9250_0X49_I2C_SLV0_DI);
}

// @brief ak8963 数据读取
// @return 成功返回读取的数据长度，失败返回0
int32_t mpu9250_ak8963_data_read(uint8_t reg, uint8_t* p_data, uint8_t data_len)
{
    ASSERT(data_len < 8);
    // 选择 ak8963 
    mpu9250_register_write(MPU9250_0X25_I2C_SLV0_ADDR, AK8963_MAGN_ADDRESS|0x80);
    // 设置 ak8963 寄存器地址
    mpu9250_register_write(MPU9250_0X26_I2C_SLV0_REG, reg);
    // 使能 i2c 并开始读取数据到对应的缓存中
    mpu9250_register_write(MPU9250_0X27_I2C_SLV0_CTRL, 0x80|data_len);
    // 延迟等待数据读取完毕
    htd_delay_us(data_len * 40);
    // 从对应缓存中读取数据
    mpu9250_data_read(MPU9250_0X49_I2C_SLV0_DI, p_data, data_len);
    return data_len;
}




//////////////////////////////////////////////////////////////////////
// @brief 读取 mpu9250 设备id
uint8_t drv_mpu9250_who_am_i(void)
{
    uint8_t whoami = 0xff;
    
    // 读取id
    for (int i=0; i<5; ++i) {
        whoami = 0xff;
        whoami = mpu9250_register_read(MPU9250_0X75_WHO_AM_I);
        if (whoami != 0x71) {
            print_error("mpu9250 read device id failed. 0x%02x", whoami);
            htd_delay_ms(200);
            continue;
        }
        print_warning("mpu9250 read device id success. whoami=0x%02x", whoami);
        return whoami;
    }
    return 0xff;
}

// @brief 读取 ak8963 设备 id
uint8_t drv_ak8963_who_am_i(void)
{
    uint8_t whoami = 0xff;
    
    // 读取id
    for (int i=0; i<5; ++i) {
        whoami = 0xff;
        whoami = mpu9250_ak8963_register_read(AK8963_0X00_WIA);
        if (whoami != 0x71) {
            print_error("ak8963 read device id failed. %02x", whoami);
            htd_delay_ms(200);
            continue;
        }
        return whoami;
    }
    return 0xff;
}

// @brief 设置校准值
void drv_mpu9250_mag_checkval_init(void)
{
    // 获取 AK8963 校准值; Madj = M * ((((asa - 128) * 0.5) / 128) + 1) = (M * (asa + 128)) >> 8
    ak8963_asa[0] = mpu9250_ak8963_register_read(AK8963_0X10_ASAX);
    ak8963_asa[1] = mpu9250_ak8963_register_read(AK8963_0X11_ASAY);
    ak8963_asa[2] = mpu9250_ak8963_register_read(AK8963_0X12_ASAZ);
}

// @brief 初始化 mpu9250 寄存器
void drv_mpu9250_register_init(void)
{
    // 初始化 i2c slave0
    //mpu9250_register_write(0x68, 7);                                            // 复位加速度、陀螺仪和温度计
    mpu9250_register_write(MPU9250_0X37_INT_PIN_CFG, BIT(1)|BIT(4)|BIT(5));  // 配置中断引脚，使能bypass
    //mpu9250_register_write(MPU9250_REG0x38_INT_ENABLE, 1);                    // 使能中断
    mpu9250_register_write(MPU9250_0X24_I2C_MST_CTRL, 0x4d);                 // I2C MAster mode and Speed 400 kHz
    mpu9250_register_write(MPU9250_0X6A_USER_CTRL, BIT(5));                  // I2C_MST _EN 
    mpu9250_register_write(0x67, 0x01);                                      // 延时使能I2C_SLV0 _DLY_ enable 
    //mpu9250_register_write(MPU9250_REG0x27_I2C_SLV0_CTRL, 0x81);                // enable IIC and EXT_SENS_DATA==1 Byte

    // 初始化陀螺仪和加速度	
    mpu9250_register_write(MPU9250_0X1A_CONFIG, 0x07);                       // 低通滤波频率，典型值：0x07(3600Hz)此寄存器内决定Internal_Sample_Rate==8K
    mpu9250_register_write(MPU9250_0X19_SAMPLE_DIV, 7);                      // 陀螺仪采样率，典型值：0x07(1kHz) (SAMPLE_RATE= Internal_Sample_Rate / (1 + SMPLRT_DIV) )
    mpu9250_register_write(MPU9250_0X1B_GYRO_CONFIG, (3<<3));                // 设置陀螺仪量程范围; 0->250dps; 1->500dps; 2->1000dps; 3->2000dps
    mpu9250_register_write(MPU9250_0X1D_ACCEL_CONFIG2, 0x08);                // 加速计高通滤波频率 典型值 ：0x08  （1.13kHz）	
    mpu9250_register_write(MPU9250_0X1C_ACCEL_CONFIG1, (2<<3));              // 加速计自检、测量范围及高通滤波频率，典型值：量程范围; 0->2g; 1->4g; 2->8g; 3->16g

    // 初始化地磁
    mpu9250_ak8963_register_write(AK8963_0X0B_CNTL2, AK8963_CNTL2_SRST);     // Reset AK8963
    mpu9250_ak8963_register_write(AK8963_0X0A_CNTL, 0x12);                   // use i2c to set AK8963 working on Continuous measurement mode1 & 16-bit output	

    //mpu9250_register_write(0x23, 0xFF);
    //htd_delay_ms(10);
    //mpu9250_register_write(0x37, 0x00);
    //mpu9250_register_write(0x38, 0x01); 

    mpu9250_register_write(MPU9250_0X38_INT_ENABLE, 1);                    // 使能中断
}

// @brief 设置采样率
void drv_mpu9250_sample_rate_div_set(uint8_t div)
{
    mpu9250_register_write(MPU9250_0X19_SAMPLE_DIV, 19);

    // set AK8963 to Power Down
    mpu9250_ak8963_register_write(AK8963_0X0A_CNTL, 0x00);
    htd_delay_ms(100);      // long wait between AK8963 mode changes

    if (div > 9) {
        // set AK8963 to 16 bit resolution, 8 Hz update rate
        mpu9250_ak8963_register_write(AK8963_0X0A_CNTL, 0x12);
    } else {
        // set AK8963 to 16 bit resolution, 100 Hz update rate
        mpu9250_ak8963_register_write(AK8963_0X0A_CNTL, 0x16);
    }
    htd_delay_ms(100);      // long wait between AK8963 mode changes
    mpu9250_register_write(MPU9250_0X19_SAMPLE_DIV, div);
    
    // instruct the MPU9250 to get 7 bytes of data from the AK8963 at the sample rate
    uint8_t buffer[8];
    mpu9250_ak8963_data_read(AK8963_0X03_XOUT_L, buffer, 7);
}


// @brief 读取加速度原始数值
// @return 成功返回读取到的数据长度6，失败返回0
uint8_t drv_mpu9250_acce_data_get(int16_t *ax, int16_t *ay, int16_t *az)
{
    uint8_t data[6];
    memset(data, 0, sizeof(data));
    mpu9250_data_read(MPU9250_0X3B_ACCEL_XOUT_H, data, sizeof(data));
    *ax = (((int16_t)data[0] << 8) | data[1]);
    *ay = (((int16_t)data[2] << 8) | data[3]);
    *az = (((int16_t)data[4] << 8) | data[5]);
    return 6;
}

// @brief 读取陀螺仪原始数值
// @return 成功返回读取到的数据长度6，失败返回0
uint8_t drv_mpu9250_gyroscope_data_get(int16_t *gx, int16_t *gy, int16_t *gz)
{
    uint8_t data[6];

    memset(data, 0, sizeof(data));
    mpu9250_data_read(MPU9250_0X43_GYRO_XOUT_H, data, sizeof(data));
    *gx = (((int16_t)data[0] << 8) | data[1]); 
    *gy = (((int16_t)data[2] << 8) | data[3]);
    *gz = (((int16_t)data[4] << 8) | data[5]);
    return 6;
}

// @brief 读取地磁数据
// @return 成功返回读取到的数据长度6，失败返回0
uint8_t drv_mpu9250_mag_data_get(int16_t *mx, int16_t *my, int16_t *mz)
{
    uint8_t data[6];
    int32_t x, y, z;

    while (!(mpu9250_ak8963_register_read(AK8963_0X02_ST1) & AK8963_ST1_DRDY)) {
        htd_delay_us(10);
    }

    memset(data, 0, sizeof(data));
    mpu9250_ak8963_data_read(AK8963_0X03_XOUT_L, data, sizeof(data));
    //AK8963每次读完以后都需要重新设置为单次测量模式，否则数据不会更新
    mpu9250_ak8963_register_write(AK8963_0X0A_CNTL, 0x11); 
    x = UINT16_VAL(&data[0]);
    y = UINT16_VAL(&data[2]);
    z = UINT16_VAL(&data[4]);

    // (M * (asa + 128)) >> 8
    *mx = ((ak8963_asa[0] + 128) * x) >> 8;
    *my = ((ak8963_asa[1] + 128) * y) >> 8;
    *mz = ((ak8963_asa[2] + 128) * z) >> 8;
    return 6;
}

// @brief 读取温度
// @return 成功返回读取到的数据长度2，失败返回0
uint8_t drv_mpu9250_temperature_get(int16_t* temperature)
{
    uint8_t data[2];
    memset(data, 0, sizeof(data));
    mpu9250_data_read(MPU9250_0X41_TEMPERATURE_H, data, sizeof(data));
    *temperature = (int16_t)((data[0] << 8) | data[1]);
    return 2;
}

// @brief 读取九轴数据
int32_t drv_mpu9250_9axis_data_read(accelerometer_t* p_acc, gyroscope_t* p_gyro, mag_val_t* p_mag)
{
#if 0
    drv_mpu9250_mag_checkval_init();
    if (p_acc) {
        p_acc->x = p_acc->y = p_acc->z = 0;
        drv_mpu9250_acce_data_get(&p_acc->x, &p_acc->y, &p_acc->z);
    }
    if (p_gyro) {
        p_gyro->x = p_gyro->y = p_gyro->z = 0;
        drv_mpu9250_gyroscope_data_get(&p_gyro->x, &p_gyro->y, &p_gyro->z);
    }
    if (p_mag) {
        p_mag->x = p_mag->y = p_mag->z = 0;
        drv_mpu9250_mag_data_get(&p_mag->x, &p_mag->y, &p_mag->z);
    }
#else
    uint8_t data[32];
    memset(data, 0, sizeof(data));
    mpu9250_data_read(MPU9250_0X3B_ACCEL_XOUT_H, data, 21);
    if (p_acc) {
        p_acc->x = (((int16_t)data[0]) << 8) | data[1];
        p_acc->y = (((int16_t)data[2]) << 8) | data[3];
        p_acc->z = (((int16_t)data[4]) << 8) | data[5];
    }
    if (p_gyro) {
        p_gyro->x = (((int16_t)data[8]) << 8) | data[9];
        p_gyro->y = (((int16_t)data[10]) << 8) | data[11];
        p_gyro->z = (((int16_t)data[12]) << 8) | data[13];
    }
    if (p_mag) {
        int16_t mx = (((int16_t)data[15]) << 8) | data[14];
        int16_t my = (((int16_t)data[17]) << 8) | data[16];
        int16_t mz = (((int16_t)data[19]) << 8) | data[18];
        p_mag->x = ((ak8963_asa[0] + 128) * mx) >> 8;
        p_mag->y = ((ak8963_asa[1] + 128) * my) >> 8;
        p_mag->z = ((ak8963_asa[2] + 128) * mz) >> 8;
    }
    //transform Temperature data into float 32 bit values 
    //*TempData = (float32_t)((((((int16_t)data[6]) << 8) | data[7]) - RoomTemp_Offset) / Temp_Sensitivity + 21.0f); 
    //according to the register map file 4.23, TEMP_degC = ((TEMP_OUT - RoomTemp_Offset)/ Temp_Sensitivity) + 21degC
    //according to the datasheet 3.4.2, RoomTemp_Offset = 21, Sensitivity = 333.87
#endif
    return 0;
}



// @brief 初始化，启动设备
static int32_t drv_mpu9250_init(uint8_t* chip_id)
{
    // 上电
    drv_9axis_power_ctrl(true);
    // 初始化硬件接口
    drv_9axis_bus_init();
    // 初始化中断引脚

    // 复位
    drv_9axis_register_write(MPU9250_0X6B_PWR_MGMT1, PWR_MGMT1_BIT_RESET);
    htd_delay_ms(100);


    if (0xff == drv_mpu9250_who_am_i()) {
        return ERROR_FAILED;
    }
    //drv_mpu9250_register_init();

#if 1
    mpu9250_register_write(MPU9250_0X6B_PWR_MGMT1, 0x01);                        // 选择时钟源
    mpu9250_register_write(MPU9250_0X6A_USER_CTRL, USER_CTRL_BIT_I2C_MST_EN);    // 使能 i2c 主模式
    mpu9250_register_write(MPU9250_0X24_I2C_MST_CTRL, I2C_MST_CTRL_CLK_400_KHZ); // 设置 i2c 主频 400 kHz
    
    mpu9250_ak8963_register_write(AK8963_0X0A_CNTL, 0x00);                       // set AK8963 to Power Down
    mpu9250_register_write(MPU9250_0X6B_PWR_MGMT1, PWR_MGMT1_BIT_RESET);         // reset the MPU9250
    htd_delay_ms(10);
    mpu9250_ak8963_register_write(0x0b, 0x01);                                   // reset the AK8963
    
    mpu9250_register_write(MPU9250_0X6B_PWR_MGMT1, 0x01);        // 选择时钟源
    mpu9250_register_write(MPU9250_0X6C_PWR_MGMT2, 0);           // enable accelerometer and gyro
    mpu9250_register_write(MPU9250_0X1C_ACCEL_CONFIG1, ACCEL_CFG1_FS_8G);          // 设置加速度量程范围 8g
    mpu9250_register_write(MPU9250_0X1B_GYRO_CONFIG, GYRO_CFG_FS_2000);          // 设置陀螺仪量程 2000DPS 
    mpu9250_register_write(MPU9250_0X1D_ACCEL_CONFIG2, 0x01);   // setting bandwidth to 184Hz as default
    mpu9250_register_write(MPU9250_0X1A_CONFIG, 0x01);           // setting gyro bandwidth to 184Hz
    mpu9250_register_write(MPU9250_0X19_SAMPLE_DIV, 19);         // setting the sample rate divider to 0 as default
    mpu9250_register_write(MPU9250_0X6A_USER_CTRL, USER_CTRL_BIT_I2C_MST_EN);    // 使能 i2c 主模式
    mpu9250_register_write(MPU9250_0X24_I2C_MST_CTRL, I2C_MST_CTRL_CLK_400_KHZ); // 设置 i2c 主频 400 kHz
    
    mpu9250_ak8963_register_write(AK8963_0X0A_CNTL, 0x00);       // set AK8963 to Power Down
    htd_delay_ms(100);  // long wait between AK8963 mode changes
    mpu9250_ak8963_register_write(AK8963_0X0A_CNTL, 0x0f);       // set AK8963 to FUSE ROM access
    htd_delay_ms(100);  // long wait between AK8963 mode changes
    // read the AK8963 ASA registers and compute magnetometer scale factors
    drv_mpu9250_mag_checkval_init();
    mpu9250_register_write(MPU9250_0X6B_PWR_MGMT1, 0x01);        // select clock source to gyro
    
    //mpu9250_ak8963_register_write(AK8963_REG0X0A_CNTL, 0x00);       // set AK8963 to Power Down
    //htd_delay_ms(100);  // long wait between AK8963 mode changes
    //mpu9250_ak8963_register_write(AK8963_REG0X0A_CNTL, 0x12);       // set AK8963 to 16 bit resolution, 100 Hz update rate
    //htd_delay_ms(100);  // long wait between AK8963 mode changes
    //uint8_t buffer[8];
    //mpu9250_ak8963_data_read(AK8963_REG0x03_XOUT_L, buffer, 7);     // instruct the MPU9250 to get 7 bytes of data from the AK8963 at the sample rate
    drv_mpu9250_sample_rate_div_set(9);

    mpu9250_register_write(MPU9250_0X37_INT_PIN_CFG, BIT(4)|BIT(5));  // 配置中断引脚
    mpu9250_register_write(MPU9250_0X38_INT_ENABLE, 1);                    // 使能中断
#endif
    return 0;
}

// @brief 去初始化，设备处于低功耗状态
static void drv_mpu9250_uninit(bool is_power_off)
{
    print_warning("mpu9250 uninit. is_power_off=%d", is_power_off);
    if (is_power_off) {
        drv_9axis_bus_deinit();
        drv_9axis_power_ctrl(false);
    } else {
        // 进入低功耗
        mpu9250_register_write(0x11, 0x80);
        htd_delay_ms(1);
        drv_9axis_bus_deinit();
    }
}

// @brief 控制句柄
int32_t drv_mpu9250_ctrl(htd_9axis_cmd_t cmd, void* p_param, uint32_t param_len)
{
    if (cmd == HTD_9AXIS_CMD_FIFO_DATA_READ) {
        
    }
    return 0;
}

// @brief 注册到上层应用中
static int32_t drv_mpu9250_register(void)
{
    const static htd_9axis_dev_t m_9axis_dev = {
        .chip_name = "mpu9250",
        .fifo_max_size = 32,
        .init = drv_mpu9250_init,
        .uninit = drv_mpu9250_uninit,
        .ctrl_handle = drv_mpu9250_ctrl,
    };
    return htd_9axis_dev_register(&m_9axis_dev);
}
DEVICE_INIT_FUNC_REGISTER(drv_mpu9250_register);
#endif /*(defined(HTD_9AXIS_USING_MPU9250) && (HTD_9AXIS_USING_MPU9250 == 1))*/

