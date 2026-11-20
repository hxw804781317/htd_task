#define DEBUG_LVL           4
#define DEBUG_TAG           "baro.msp2520r"
#include "htd_barometer_dev.h"



#if defined(HTD_BAROMETER_USING_MSP2520R) && HTD_BAROMETER_USING_MSP2520R
//////////////////////////////////////////////////////////////////////
#define MSP2520R_SLAVE_ADDR             0x78
static const uint32_t D_MAX = 15099494;    //满量程ADC值
static const uint32_t D_MIN = 1677722;     //零点ADC值
//传感器校准量程
static const uint32_t P_MAX = 120;
static const uint32_t P_MIN = 30;
static i2cm_handle_t* m_msp2520_i2c_handle = NULL;


/////////////////////////////////////////////////////////////////////////////////
// @brief 供电引脚控制
static void barometer_dev_power_ctrl(bool enable)
{
#if ((defined HTD_BAROMETER_POWER_PIN) && (HTD_BAROMETER_POWER_PIN > 0))
    uint8_t level = !!enable;
    #if !defined(HTD_BAROMETER_POWER_PIN_ACTIVE_HIGH)
        level = !level;
    #endif

    htd_pin_output_pp_cfg(HTD_BAROMETER_POWER_PIN);
    htd_pin_write(HTD_BAROMETER_POWER_PIN, level);
    if (enable) {
        htd_sleep_ms(10);
    }
    print_debug("power ctrl[%d]: enable=%d", HTD_BAROMETER_POWER_PIN, enable);
#endif /*((defined HTD_BAROMETER_POWER_PIN) && (HTD_BAROMETER_POWER_PIN > 0))*/
}

// @brief 读寄存器数据
static bool msp2520r_reg_read(uint8_t *rbuf, uint8_t len)
{
    ASSERT(m_msp2520_i2c_handle);
    for (uint32_t i=0; i<3; ++i) {
        if (0 == htd_i2cm_read(m_msp2520_i2c_handle, MSP2520R_SLAVE_ADDR, rbuf, len)) {
            return true;
        }
    }
    return false;
}

// @brief 写寄存器数据，成功返回0
static int32_t msp2520r_reg_write(uint8_t reg)
{   
    ASSERT(m_msp2520_i2c_handle);
    for (uint32_t i=0; i<3; ++i) {
        if (0 == htd_i2cm_write(m_msp2520_i2c_handle, MSP2520R_SLAVE_ADDR, &reg, sizeof(uint8_t))) {
            return true;
        }
    }
    return false;
}

// @brief msp2520r 初始化
int32_t drv_msp2520r_init(void)
{
    if (m_msp2520_i2c_handle) {
        return 0;
    }

    // 上电
    barometer_dev_power_ctrl(true);
    // 总线初始化
    i2cm_user_config_t i2c_config = HTD_I2C_CONFIG_DEFAULT;
    i2c_config.pin_sck = HTD_BAROMETER_I2C_PIN_SCK;
    i2c_config.pin_sda = HTD_BAROMETER_I2C_PIN_SDA;
    i2c_config.freq_hz = 400*1000;
    m_msp2520_i2c_handle = htd_i2cm_open(HTD_BAROMETER_BUS_NAME, &i2c_config);
    ASSERT(m_msp2520_i2c_handle);
    return 0;
}

// @brief 去初始化
void drv_msp2520r_uninit(bool is_power_off)
{
    if (m_msp2520_i2c_handle) {
        htd_i2cm_close(m_msp2520_i2c_handle);
        m_msp2520_i2c_handle = NULL;
    }
    barometer_dev_power_ctrl(false);
}

// @brief 获取真实的压强值(Pa, 帕); 耗时操作
int32_t drv_msp2520r_pressure_get(uint32_t *pressure_pa) 
{
    // 设置采样率为温度:8x 气压 32x, 大概60ms左右出值
    if (msp2520r_reg_write(0xB2)) {
        //写0xB2指令开启一次测量
        print_debug("msp2520r_write_reg_err");
        return -1;
    }

    uint32_t start_ms = htd_time_ms_get();
    uint8_t status;
    htd_delay_ms(10);
    for (;;) {
        // 软件查询转换完成状态，若状态第5位为1表示为转换完成，若转换完状态位变0即可读取压力值
        htd_delay_ms(2);
        status = 0;
        msp2520r_reg_read(&status, 1);
        print_debug("status=%02x", status);
        if ((status & 0x20) == 0x20) {
            break;
        }
        // 最大测量时间 28ms
        if (htd_time_ms_diff(htd_time_ms_get(), start_ms) > 200) {
            return -1;
        }
    }

    uint32_t val;
    uint8_t reg_val[6];
    msp2520r_reg_read(reg_val, 6);
    print_hex("reg_val", reg_val, 6);

    // 气压值
    if (pressure_pa) {
        val = (reg_val[1]<<16)|(reg_val[2]<<8)|reg_val[3];
        *pressure_pa = 1000*(P_MAX-P_MIN)*(val-D_MIN) / (D_MAX-D_MIN) + P_MIN*1000;      //单位精确到Pa
    }
    // 温度值
    //val = (reg_val[4]<<8) | reg_val[5];
    //val = 100 * val * 190 / 65535 - 100 * 40;
    //temperature_value = val;
    return 0;
}

// @brief 注册设备
static int32_t drv_barometer_dev_register(void)
{
    static const barometer_dev_t m_barometer_msp2520r = {
        .chip_name = "msp2520r",
        .init = drv_msp2520r_init,
        .uninit = drv_msp2520r_uninit,
        .read_handle = drv_msp2520r_pressure_get,
    };
    return htd_barometer_dev_register(&m_barometer_msp2520r);
}
DEVICE_INIT_FUNC_REGISTER(drv_barometer_dev_register);
#endif /*defined(HTD_BAROMETER_USING_MSP2520R) && HTD_BAROMETER_USING_MSP2520R*/

