#define __ENABLE_DEBUG__
#include "hal_sys.h"
#include "drv_icm20948.h"
#include "drv_icm20948_spi.h"


//////////////////////////////////////////////////////////////////////
// @brief 中断信号任务函数
void icm20948_int_task_handle(void* arg)
{
    accelerometer_t acc;
    gyroscope_t gyro;
    mag_val_t mag;

    drv_icm20948_9axis_data_read(&acc, &gyro, &mag);
    print_debug("acc=(%6d,%6d,%6d); gyro=(%6d,%6d,%6d), mag=(%6d,%6d,%6d)", acc.x, acc.y, acc.z, gyro.x, gyro.y, gyro.z, mag.x, mag.y, mag.z);
}

// @brief 中断引脚回调
static void icm20948_int_handle(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    //print_debug("now_ms=%lu", hal_time_ms_get());
    if (pin == ICM20948_INT_PIN) {
        task_fifo_load(icm20948_int_task_handle, NULL);
        //icm20948_int_task_handle(NULL);
    }
}

// @brief 计步器中断引脚初始化
static int32_t icm20948_IRQ_pin_init(void)
{
    ret_code_t err_code;
    
    if (!nrf_drv_gpiote_is_init()) {
        err_code = nrf_drv_gpiote_init();
        VERIFY_SUCCESS(err_code);
    }
    
    nrf_drv_gpiote_in_config_t config = NRFX_GPIOTE_CONFIG_IN_SENSE_LOTOHI(true);
    config.pull = NRF_GPIO_PIN_PULLDOWN;
    err_code = nrf_drv_gpiote_in_init(ICM20948_INT_PIN, &config, icm20948_int_handle);
    //VERIFY_SUCCESS(err_code);
    nrf_drv_gpiote_in_event_enable(ICM20948_INT_PIN, true);
    return err_code;
}

// @brief 寄存器写
static void icm20948_register_write(uint8_t reg, uint8_t byte)
{
    drv_9axis_register_write(reg, byte);
}

// @brief 寄存器读
static uint8_t icm20948_register_read(uint8_t reg)
{
    return drv_9axis_register_read(reg);
}

// @brief 读取数据
static uint8_t icm20948_data_read(uint8_t reg, uint8_t* p_data, uint16_t data_len)
{
    return drv_9axis_bus_data_read(reg, p_data, data_len);
}

// @brief 设置 user bank
static void icm20948_user_bank_set(uint8_t bank)
{
    icm20948_register_write(ICM20948_0x7f_REG_BANK_SEL, bank);
    htd_delay_us(10);
}

// @brief ak09916 寄存器写入
// @return 成功返回写入的数据长度，失败返回0
static void icm20948_ak09916_register_write(uint8_t reg, uint8_t byte)
{
    // 选择 ak09916 
    icm20948_user_bank_set(USER_BANK_3);
    icm20948_register_write(ICM20948B3_0x03_I2C_SLV0_ADDR, AK09916_MAGN_ADDR);
    // 设置 ak09916 寄存器地址
    icm20948_register_write(ICM20948B3_0x04_I2C_SLV0_REG, reg);
    // 设置 ak09916 寄存器值
    icm20948_register_write(ICM20948B3_0x06_I2C_SLV0_DO, byte);
    // 使能 i2c 并开始发送数据
    icm20948_register_write(ICM20948B3_0x05_I2C_SLV0_CTRL, 0x81);
    htd_delay_us(50);
    icm20948_user_bank_set(USER_BANK_0);
}

// @brief ak09916 寄存器读取
// @return 成功返回读取的数据长度，失败返回0
static uint8_t icm20948_ak09916_register_read(uint8_t reg)
{
    // 选择 ak09916 
    icm20948_user_bank_set(USER_BANK_3);
    icm20948_register_write(ICM20948B3_0x03_I2C_SLV0_ADDR, AK09916_MAGN_ADDR|0x80);
    // 设置 ak09916 寄存器地址
    icm20948_register_write(ICM20948B3_0x04_I2C_SLV0_REG, reg);
    // 使能 i2c 并开始读取数据到对应的缓存中
    icm20948_register_write(ICM20948B3_0x05_I2C_SLV0_CTRL, 0x81);
    // 延迟等待数据读取完毕
    htd_delay_us(40);
    // 从对应缓存中读取数据
    icm20948_user_bank_set(USER_BANK_0);
    return icm20948_register_read(ICM20948B0_0x3b_EXTEN_DATA);
}

// @brief ak09916 数据读取
// @return 成功返回读取的数据长度，失败返回0
static int32_t icm20948_ak09916_data_read(uint8_t reg, uint8_t* p_data, uint8_t data_len)
{
    ASSERT(data_len < 8);
    // 选择 ak09916 
    icm20948_user_bank_set(USER_BANK_3);
    icm20948_register_write(ICM20948B3_0x03_I2C_SLV0_ADDR, AK09916_MAGN_ADDR|0x80);
    // 设置 ak09916 寄存器地址
    icm20948_register_write(ICM20948B3_0x04_I2C_SLV0_REG, reg);
    // 使能 i2c 并开始读取数据到对应的缓存中
    icm20948_register_write(ICM20948B3_0x05_I2C_SLV0_CTRL, 0x80|data_len);
    // 延迟等待数据读取完毕
    htd_delay_us(data_len * 40 - 10);
    // 从对应缓存中读取数据
    icm20948_user_bank_set(USER_BANK_0);
    icm20948_data_read(ICM20948B0_0x3b_EXTEN_DATA, p_data, data_len);
    return data_len;
}


//////////////////////////////////////////////////////////////////////
// @brief 读取 icm20948 设备id
uint8_t drv_icm20948_who_am_i(void)
{
    uint8_t whoami = 0xff;
    for (int i=0; i<5; ++i) {
        whoami = 0xff;
        whoami = icm20948_register_read(ICM20948B0_0x00_WHO_AM_I);
        if (whoami != 0xEA) {
            print_error("icm20948 read device id failed. %02x", whoami);
            htd_delay_ms(200);
            continue;
        }
        print_warning("icm20948 read device id success. whoami=%02x", whoami);
        return whoami;
    }
    return 0xff;
}

// @brief 读取 ak09916 设备 id
uint8_t drv_ak09916_who_am_i(void)
{
    uint8_t whoami = 0xff;
    for (int i=0; i<5; ++i) {
        whoami = 0xff;
        whoami = icm20948_ak09916_register_read(AK09916_0x01_WIA2);
        if (whoami != 0x09) {
            print_error("ak09916 read device id failed. %02x", whoami);
            htd_delay_ms(200);
            continue;
        }
        return whoami;
    }
    return 0xff;
}

// @brief 设置加速度量程范围
void drv_icm20948_accel_range_set(accel_range_t range)
{
    icm20948_user_bank_set(USER_BANK_2);
    icm20948_register_write(ICM20948B2_0x14_ACCEL_CONFIG1, range | ACCEL_DLPFCFG_1);
    icm20948_user_bank_set(USER_BANK_0);
}

// @brief 设置陀螺仪量程范围
void drv_icm20948_gyro_range_set(gyro_range_t range)
{
    icm20948_user_bank_set(USER_BANK_2);
    icm20948_register_write(ICM20948B2_0x01_GYRO_CONFIG_1, range | GYRO_DLPFCFG_1);
    icm20948_user_bank_set(USER_BANK_0);
}

// @brief 读取加速度原始数值
// @return 成功返回读取到的数据长度6，失败返回0
uint8_t drv_icm20948_acce_data_get(int16_t *ax, int16_t *ay, int16_t *az)
{
    uint8_t data[6];
    
    memset(data, 0, sizeof(data));
    icm20948_user_bank_set(USER_BANK_0);
    icm20948_data_read(ICM20948B0_0x2d_ACCEL_XOUT_H, data, sizeof(data));
    *ax = (((int16_t)data[0] << 8) | data[1]);
    *ay = (((int16_t)data[2] << 8) | data[3]);
    *az = (((int16_t)data[4] << 8) | data[5]);
    return 6;
}

// @brief 读取陀螺仪原始数值
// @return 成功返回读取到的数据长度6，失败返回0
uint8_t drv_icm20948_gyroscope_data_get(int16_t *gx, int16_t *gy, int16_t *gz)
{
    uint8_t data[6];

    memset(data, 0, sizeof(data));
    icm20948_user_bank_set(USER_BANK_0);
    icm20948_data_read(ICM20948B0_0x33_GYRO_XOUT_H, data, sizeof(data));
    *gx = (((int16_t)data[0] << 8) | data[1]); 
    *gy = (((int16_t)data[2] << 8) | data[3]);
    *gz = (((int16_t)data[4] << 8) | data[5]);
    return 6;
}

// @brief 读取温度
// @return 成功返回读取到的数据长度2，失败返回0
uint8_t drv_icm20948_temperature_get(int16_t* temperature)
{
    uint8_t data[2];
    memset(data, 0, sizeof(data));
    icm20948_user_bank_set(USER_BANK_0);
    icm20948_data_read(ICM20948B0_0x39_TEMPERATURE_H, data, sizeof(data));
    *temperature = (int16_t)((data[0] << 8) | data[1]);
    return 2;
}

// @brief 读取地磁数据
// @return 成功返回读取到的数据长度6，失败返回0
uint8_t drv_icm20948_mag_data_get(int16_t *mx, int16_t *my, int16_t *mz)
{
    return 6;
}

// @brief 读取九轴数据
int32_t drv_icm20948_9axis_data_read(accelerometer_t* p_acc, gyroscope_t* p_gyro, mag_val_t* p_mag)
{
    uint8_t data[32];
    
    memset(data, 0, sizeof(data));
    icm20948_user_bank_set(USER_BANK_0);
    icm20948_data_read(ICM20948B0_0x2d_ACCEL_XOUT_H, data, 20);
    if (p_acc) {
        p_acc->x = (((int16_t)data[0]) << 8) | data[1];
        p_acc->y = (((int16_t)data[2]) << 8) | data[3];
        p_acc->z = (((int16_t)data[4]) << 8) | data[5];
    }
    if (p_gyro) {
        p_gyro->x = (((int16_t)data[6]) << 8) | data[7];
        p_gyro->y = (((int16_t)data[8]) << 8) | data[9];
        p_gyro->z = (((int16_t)data[10]) << 8) | data[11];
    }
    if (p_mag) {
        p_mag->x = (((int16_t)data[15]) << 8) | data[14];
        p_mag->y = (((int16_t)data[17]) << 8) | data[16];
        p_mag->z = (((int16_t)data[19]) << 8) | data[18];
    }
    //transform Temperature data into float 32 bit values 
    //*TempData = (float32_t)((((((int16_t)data[6]) << 8) | data[7]) - RoomTemp_Offset) / Temp_Sensitivity + 21.0f); 
    //according to the register map file 4.23, TEMP_degC = ((TEMP_OUT - RoomTemp_Offset)/ Temp_Sensitivity) + 21degC
    //according to the datasheet 3.4.2, RoomTemp_Offset = 21, Sensitivity = 333.87
    return 0;
}


// @brief 初始化 icm20948
int32_t drv_icm20948_init(void)
{
    icm20948_pin_init();
    htd_delay_ms(1);

    icm20948_user_bank_set(USER_BANK_0);
    htd_delay_us(10);

    // 读取设备 id
    if (0xff == drv_icm20948_who_am_i()) {
        return ERROR_FAILED;
    }
    
    // 复位
    icm20948_register_write(ICM20948B0_0x06_PWR_MGMT1, PWR_MGMT1_BIT_RESET);
    htd_delay_ms(100);
    // 读取设备 id
    if (0xff == drv_icm20948_who_am_i()) {
        return ERROR_FAILED;
    }

    // 选择时钟源
    icm20948_register_write(ICM20948B0_0x06_PWR_MGMT1, 0x01);
    // 使能 i2c 主模式
    icm20948_register_write(ICM20948B0_0x03_USER_CTRL, USER_CTRL_BIT_I2C_MST_EN|USER_CTRL_BIT_I2C_IF_DIS);
    // 设置 i2c 主频 400 kHz
    icm20948_user_bank_set(USER_BANK_3);
    icm20948_register_write(ICM20948B3_0x01_I2C_MST_CTRL, 7);
    // set AK09916 to Power Down
    icm20948_ak09916_register_write(AK09916_0x31_CNTL2, CNTL2_MODE_POWER_DOWN);
    htd_delay_ms(10);
    // 复位 AK09916
    icm20948_ak09916_register_write(AK09916_0x32_CNTL3, CNTL3_BIT_SOFT_RST);
    
    // 使能加速度和陀螺仪
    icm20948_register_write(ICM20948B0_0x07_PWR_MGMT2, 0);
    // 设置加速度量程范围 8g
    icm20948_user_bank_set(USER_BANK_2);
    icm20948_register_write(ICM20948B2_0x14_ACCEL_CONFIG1, ACCEL_DLPFCFG_6|ACCEL_CFG1_BIT_FCHOICE|ACCEL_RANGE_8G);
    // 设置陀螺仪量程 2000DPS 
    icm20948_register_write(ICM20948B2_0x01_GYRO_CONFIG_1, GYRO_DLPFCFG_6|GYRO_CFG1_BIT_FCHOICE|GYRO_RANGE_2000DPS);
    // 设置陀螺仪采样率
    icm20948_register_write(ICM20948B2_0x00_GYRO_SAMPLE_DIV, 21);
    // 设置加速度采样率
    icm20948_register_write(ICM20948B2_0x10_ACCEL_SAMLRT_DIV_1, 0);
    icm20948_register_write(ICM20948B2_0x11_ACCEL_SMPLRT_DIV_2, 21);
    icm20948_user_bank_set(USER_BANK_3);
    icm20948_register_write(ICM20948B3_0x02_I2C_MST_DELAY_CTRL, 0x01);
    //icm20948_register_write(ICM20948B3_0x05_I2C_SLV0_CTRL, 0x81);
    
    // 设置为连续测量模式
    icm20948_ak09916_register_write(AK09916_0x31_CNTL2, CNTL2_MODE_CONTINUE3_MEASURE);

    // 配置中断
    icm20948_register_write(ICM20948B0_0x0f_INT_PIN_CFG, INT_CFG_BIT_LATCH_INT_EN|INT_CFG_BIT_INT_ANYRD_2CLEAR);
    icm20948_register_write(ICM20948B0_0x11_INT1_ENABLE, INT1_ENABLE_BIT_RAW_DATA_0_RDY_EN);
    return ERROR_SUCCESS;
}

