#define DEBUG_LVL           4       // 1:error; 2:warning; 3:debug; 4:log
#define DEBUG_TAG           "baro.bmp180"
#include "htd_barometer_dev.h"


#if defined(HTD_BAROMETER_USING_BMP180) && HTD_BAROMETER_USING_BMP180
/////////////////////////////////////////////////////////////////////////////////
// i2c 地址
#define BMP180_I2C_SLAVE_ADDR                       0xee
// 过采样(0-3)
#define BMP180_OSS                                  3

// 内部校准参数
typedef struct {
    int16_t ac1, ac2, ac3;
    uint16_t ac4, ac5, ac6;
    int16_t b1, b2, mb, mc, md;
} bmp180_calibrate_t;
static bmp180_calibrate_t *m_bmp180_calibrate_param = NULL;
static i2cm_handle_t* m_bmp180_i2c_handle = NULL;

// @brief 写寄存器数据
static bool bmp180_data_read(uint8_t addr, uint8_t *out, uint8_t out_len)
{
    return (0 == htd_i2cm_reg8_read(m_bmp180_i2c_handle, BMP180_I2C_SLAVE_ADDR, addr, out, out_len));
}

// @brief 写寄存器数据
static bool bmp180_data_write(uint8_t addr, uint8_t *p_data, uint8_t data_len)
{
    return (0 == htd_i2cm_reg8_write(m_bmp180_i2c_handle, BMP180_I2C_SLAVE_ADDR, addr, p_data, data_len));
}

// @brief 读取一个字节
static uint8_t bmp180_register_read(uint8_t addr)
{
    uint8_t byte = 0xff;
    bmp180_data_read(addr, &byte, sizeof(uint8_t));
    return byte;
}

// @brief 写寄存器数据
static bool bmp180_register_write(uint8_t addr, uint8_t val)
{
    uint8_t w2_data[2];
    w2_data[0] = addr;
    w2_data[1] = val;
    return bmp180_data_write(addr, w2_data, sizeof(w2_data));
}

// @brief 读两个字节
static uint16_t bmp180_register_word_read(uint8_t addr)
{
    uint8_t data[2];
    memset(data, 0, sizeof(data));
    bmp180_data_read(addr, data, sizeof(data));
    return ((data[0] << 8) | (data[1]));
}

// @brief 计算未补偿的温度 (UT)
static int32_t bmp180_read_ut(void) 
{
    uint8_t data[2];
    bmp180_register_write(0xF4, 0x2E); // 发送温度测量命令
    htd_delay_ms(5); // 等待转换
    bmp180_data_read(0xF6, data, 2);
    return (int32_t)((data[0] << 8) | data[1]);
}

// @brief 计算未补偿的压强 (UP)
// @param oss 过采样设置 (0-3)
static int32_t bmp180_read_up(uint8_t oss) 
{
    uint8_t data[3];
    uint32_t up;
    
    // 发送压强测量命令
    bmp180_register_write(0xF4, 0x34 + (oss << 6));
    // 等待转换时间 (取决于 oss)
    htd_delay_ms(2 + (3 << oss)); 
    
    bmp180_data_read(0xF6, data, 3);
    up = (((uint32_t)data[0] << 16) | ((uint32_t)data[1] << 8) | (uint32_t)data[2]);
    up = up >> (8 - oss);   // 根据过采样移位
    return (int32_t)up;
}

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


/////////////////////////////////////////////////////////////////////////////////
// @brief bmp180 初始化
int32_t drv_bmp180_init(void)
{
    if (m_bmp180_i2c_handle) {
        return 0;
    }

    // 上电
    barometer_dev_power_ctrl(true);
    
    // 打开 i2c 总线
    i2cm_user_config_t i2c_config = HTD_I2C_CONFIG_DEFAULT;
    i2c_config.pin_sck = HTD_BAROMETER_I2C_PIN_SCK;
    i2c_config.pin_sda = HTD_BAROMETER_I2C_PIN_SDA;
    m_bmp180_i2c_handle = htd_i2cm_open(HTD_BAROMETER_BUS_NAME, &i2c_config);
    ASSERT(m_bmp180_i2c_handle);
    htd_delay_ms(1);

    // 读取id
    uint8_t chip_id = 0;
    for (uint32_t i=0; i<3; ++i) {
        chip_id = bmp180_register_read(0xD0);
        if (chip_id == 0x55) {
            break;
        }
    }
    if (chip_id != 0x55) {
        print_error("bmp180 init failed. chip_id=0x%02x", chip_id);
        htd_i2cm_close(m_bmp180_i2c_handle);
        m_bmp180_i2c_handle = NULL;
        return -1;
    }

    m_bmp180_calibrate_param = htd_malloc(sizeof(bmp180_calibrate_t));
    ASSERT(m_bmp180_calibrate_param);
    
    // 读取校准参数
    bmp180_calibrate_t *p_calibrate = m_bmp180_calibrate_param;
    p_calibrate->ac1 = bmp180_register_word_read(0xAA);
    p_calibrate->ac2 = bmp180_register_word_read(0xAC);
    p_calibrate->ac3 = bmp180_register_word_read(0xAE);
    p_calibrate->ac4 = bmp180_register_word_read(0xB0);
    p_calibrate->ac5 = bmp180_register_word_read(0xB2);
    p_calibrate->ac6 = bmp180_register_word_read(0xB4);
    p_calibrate->b1 = bmp180_register_word_read(0xB6);
    p_calibrate->b2 = bmp180_register_word_read(0xB8);
    p_calibrate->mb = bmp180_register_word_read(0xBA);
    p_calibrate->mc = bmp180_register_word_read(0xBC);
    p_calibrate->md = bmp180_register_word_read(0xBE);
    return 0;
}

// @brief si7020 去初始化
void drv_bmp180_uninit(bool is_power_off)
{
    if (NULL != m_bmp180_i2c_handle) {
        htd_i2cm_close(m_bmp180_i2c_handle);
        m_bmp180_i2c_handle = NULL;
    }
    if (NULL != m_bmp180_calibrate_param) {
        htd_free(m_bmp180_calibrate_param);
        m_bmp180_calibrate_param = NULL;
    }
    barometer_dev_power_ctrl(false);
}

// @brief 获取真实的压强值 (Pa, 帕)
int32_t drv_bmp180_pressure_get(uint32_t *pressure_pa) 
{
    int32_t ut, up;
    int32_t x1, x2, x3, b3, b5, b6;
    uint32_t b4, b7;
    int32_t p;

    bmp180_calibrate_t *p_calibrate = m_bmp180_calibrate_param;
    if (m_bmp180_i2c_handle == NULL) {
        return -1;
    }
    
    // 1. 读取温度
    ut = bmp180_read_ut();
    
    // 2. 计算真实温度 (单位 0.1 degC)
    // 虽然这里不需要返回温度，但计算压强必须先算温度
    x1 = (((int32_t)ut - (int32_t)p_calibrate->ac6) * (int32_t)p_calibrate->ac5) >> 15;
    x2 = ((int32_t)p_calibrate->mc << 11) / (x1 + (int32_t)p_calibrate->md);
    b5 = x1 + x2;
    
    // 3. 读取压强
    up = bmp180_read_up(BMP180_OSS);
    
    // 4. 计算真实压强
    b6 = b5 - 4000;
    x1 = ((int32_t)p_calibrate->b2 * ((b6 * b6) >> 12)) >> 11;
    x2 = ((int32_t)p_calibrate->ac2 * b6) >> 11;
    x3 = x1 + x2;
    b3 = ((((int32_t)p_calibrate->ac1 * 4 + x3) << oss) + 2) >> 2;
    
    x1 = ((int32_t)p_calibrate->ac3 * b6) >> 13;
    x2 = ((int32_t)p_calibrate->b1 * ((b6 * b6) >> 12)) >> 16;
    x3 = ((x1 + x2) + 2) >> 2;
    b4 = ((uint32_t)p_calibrate->ac4 * (uint32_t)(x3 + 32768)) >> 15;
    b7 = ((uint32_t)up - b3) * (uint32_t)(50000 >> oss);
    if (b7 < 0x80000000) {
        p = (b7 << 1) / b4;
    } else {
        p = (b7 / b4) << 1;
    }
    
    x1 = (p >> 8) * (p >> 8);
    x1 = (x1 * 3038) >> 16;
    x2 = (-7357 * p) >> 16;
    p = p + ((x1 + x2 + 3791) >> 4);

    if (pressure_pa) {
        *pressure_pa = p;
    }
    return 0;
}

// @brief 注册设备
static int32_t drv_barometer_dev_register(void)
{
    static const barometer_dev_t m_barometer_bmp180 = {
        .chip_name = "BMP180",
        .init = drv_bmp180_init,
        .uninit = drv_bmp180_uninit,
        .read_handle = drv_bmp180_pressure_get,
    };
    return htd_barometer_dev_register(&m_barometer_bmp180);
}
DEVICE_INIT_FUNC_REGISTER(drv_barometer_dev_register);
#endif /*defined(HTD_BAROMETER_USING_BMP180) && HTD_BAROMETER_USING_BMP180*/

