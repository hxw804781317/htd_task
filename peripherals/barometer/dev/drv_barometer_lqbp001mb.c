// Copyright (c) 2026 Daven <804781317@qq.com>
// SPDX-License-Identifier: MIT
#if 0
// #define __ENABLE_DEBUG__
#define DEBUG_TAG "sdrv.lqbp001mb"

#include "sdrv_lqbp001mb.h"
#include <rtthread.h>
#include "sdrv_bar_interface.h"
#include "sdrv_power_management_ido.h"
#include "air_pressure_imp.h"
#include "air_pressure_al.h"
#include "app_error.h"

#define T_REF 27
#define A0 5030
#define T_C_VBE -0.001735
#define aa 9.45
#define V_BE_TARGET_T_REF 0.687027
#define k_PTAT_CORNER -0.8
#define k_PTAT_CURVATURE 0.039

LQBP001MB_RRE_TEMP LQBP001MB_CAL_PRE_TEMP; //  数据存放的结构体
LQBP001MB_CAL_COFF LQBP001MB_CAL_COFF_RE;  // 存放COFF 计算的相关数值

//爱都自定义参数
static bool s_read_succeed_f = false;
static bool lqbp001mb_is_open = false;
static float pressure_value;
static float temperature_value;

// 读 指定地址 指定寄存器的 指定长度的数据
int32_t I2C_ReadBuf(uint8_t Addr, uint8_t reg, uint8_t *pBuffer, uint8_t len)
{
    return drv_bar_register_data_read(Addr, reg, pBuffer, len);
}

// 读指定地址 指定寄存器的 值
int32_t I2C_ReadReg(uint8_t Addr, uint8_t reg)
{
    uint8_t data = 0;
    drv_bar_register_data_read(Addr, reg, &data, 1);
    return data;
}

// 写单个寄存器数据
int32_t I2C_WriteReg(uint8_t Addr, uint8_t reg, uint8_t value)
{
    return drv_bar_register_data_write(Addr, reg, &value, 1);
}

/*!
 * @brief implement delay function
 *
 * @param msec  millisecond numbers
 *
 * @return no return value
 */
static void lqbp001mb_delay_ms(uint32_t msec)
{
    rt_thread_mdelay(msec);
}

/**
 * @brief 获取芯片ID
 *
 * 该函数通过I2C读取LQBP001MB芯片的ID寄存器，以确定芯片是否存在。
 * 如果芯片ID匹配预期值0x1A，则认为芯片存在并返回真；否则，认为芯片不存在并返回假。
 *
 * @param id 指向用于存储芯片ID的缓冲区
 * @return uint8_t 返回true表示芯片存在，返回false表示芯片不存在
 */
static uint8_t get_chipid(uint8_t *id)
{
    // 读取LQBP001MB芯片的ID寄存器值
    if (I2C_ReadReg(LQBP001MB, LQBP001MB_ID_addr) == 0x1A)
    {
        // 如果读取的值等于预期的芯片ID，则将ID赋值为0x1A，并返回true
        *id = 0x1A;
        return true;
    }

    // 如果读取的值不等于预期的芯片ID，则将ID赋值为0xff，并返回false
    *id = 0xff;
    return false;
}
/**
 * @brief   	LQBP001MB 初始化，并核对ID
 * @param   	参数
 *		@arg 	无
 * @retval  	器件是否初始化成功 true:成功 false:失败
 * @attention	初始化器件
 */
u8 LQBP001MB_INIT(void)
{
    if (I2C_ReadReg(LQBP001MB, LQBP001MB_ID_addr) != 0x1A)
    {
        lqbp001mb_delay_ms(40);
    }

    if (I2C_ReadReg(LQBP001MB, LQBP001MB_ID_addr) == 0x1A)
    {
        LQBP001MB_COFF_CAL(); // 读取校准系数并保存

        return true;
    }
    else
        return false;
}

/**
 * @brief   	LQBP001MB 设置共存模式
 * @param   	参数
 *		@arg 	无
 * @retval  	设置成功
 * @attention	模式设置
 */
u8 set_LQBP001MB_mode(u8 mode)
{
    u8 i = 0;
    if (mode == 1)
    {
        I2C_WriteReg(LQBP001MB, 0X08, 0X00); // 停止采集

        while ((I2C_ReadReg(LQBP001MB, 0x08) & 0x40) != 0) // 判断是否已经停止
        {
            i++;
            if (i > 100)
                return 0x00;
        }
        I2C_WriteReg(LQBP001MB, 0x7c, 0x08);        //
        I2C_WriteReg(LQBP001MB, 0X0E, 0x08 | 0x03); // 设置模式
        I2C_WriteReg(LQBP001MB, 0X08, 0x04 | 0x03); // 开启采集

        return 0x01;
    }
    else
    {
        I2C_WriteReg(LQBP001MB, 0X08, 0X00); // 停止采集

        while ((I2C_ReadReg(LQBP001MB, 0x08) & 0x40) != 0) // 判断是否已经停止
        {
            i++;
            if (i > 100)
                return 0x00;
        }
        I2C_WriteReg(LQBP001MB, 0x7c, 0x00);        //
        I2C_WriteReg(LQBP001MB, 0X0E, 0x00);        // 设置模式
        I2C_WriteReg(LQBP001MB, 0X08, 0x04 | 0x03); // 开启采集

        return 0x01;
    }
}

/*=============================================LQBP001MB===============================================*/

/**
 * @brief   	LQBP001MB 根据参数计算相关的系数值
 * @param   	参数
 *		@arg 	无
 * @retval  	无
 * @attention	计算校准参数值
 */
void LQBP001MB_COFF_CAL(void) // 计算数据,根据结构体中的数据自动进行计算
{
    //------------------读取校准参数，然后进行计算 ------------------------------------------------

    // I2C_ReadBuf(LQBP001MB,0X00,&LQBP001MB_CAL_PRE_TEMP.PSR_B[0],15);      		//读取数值
    I2C_ReadBuf(LQBP001MB, 0X20, &LQBP001MB_CAL_PRE_TEMP.T_GAIN_COEFF[0], 3); // 读取COFF gain 值
    I2C_ReadBuf(LQBP001MB, 0X26, &LQBP001MB_CAL_PRE_TEMP.COEFF_reg[0], 20);   // 读取COFF 值

    //-----------------------  T  COEFF 计算 ----------------------------------------------------
    s8 T_GAIN_COEFF = LQBP001MB_CAL_PRE_TEMP.T_GAIN_COEFF[0];
    s8 T_dVBE_COEFF = LQBP001MB_CAL_PRE_TEMP.T_GAIN_COEFF[1] >> 1;
    s16 T_VBE_COEFF = ((LQBP001MB_CAL_PRE_TEMP.T_GAIN_COEFF[1] & 0x01)) | (LQBP001MB_CAL_PRE_TEMP.T_GAIN_COEFF[2] << 1);

    LQBP001MB_CAL_COFF_RE.T_GAIN_COEFF = T_GAIN_COEFF;
    LQBP001MB_CAL_COFF_RE.T_dVBE_COEFF = (T_dVBE_COEFF & 0x40) ? (0X80 | T_dVBE_COEFF) : T_dVBE_COEFF;
    LQBP001MB_CAL_COFF_RE.T_VBE_COEFF = (T_VBE_COEFF & 0x0100) ? (0XFE00 | T_VBE_COEFF) : T_VBE_COEFF;
    //------------------------------计算温度数值--------------------------------------

    double V_BE = LQBP001MB_CAL_COFF_RE.T_VBE_COEFF * 0.000105031 + 0.463232422;
    double tr_V_BE = LQBP001MB_CAL_COFF_RE.T_dVBE_COEFF * 0.0000125885 + 0.04027621;
    double A_ADC = LQBP001MB_CAL_COFF_RE.T_GAIN_COEFF * 0.000084375 + 0.675;

    double V_BE_CAL = V_BE / A_ADC;
    double tr_V_BE_CAL = tr_V_BE / A_ADC;

    double T_CALIB = (double)A0 * tr_V_BE_CAL - 273.15;

    double V_BE_CAL_T_REF = (double)V_BE_CAL - (T_CALIB - T_REF) * (T_C_VBE);

    double k_PTAT = (double)(V_BE_TARGET_T_REF - V_BE_CAL_T_REF) * k_PTAT_CORNER + k_PTAT_CURVATURE;

    LQBP001MB_CAL_COFF_RE.Ap = (double)A0 * (V_BE_CAL + aa * tr_V_BE_CAL) * (1 + k_PTAT);
    LQBP001MB_CAL_COFF_RE.Bp = (double)-273.15 * (1 + k_PTAT) - k_PTAT * T_CALIB;

    // -------------------P  COEFF ??------------------------------------------------------------------
    LQBP001MB_CAL_COFF_RE.C00 = (LQBP001MB_CAL_PRE_TEMP.COEFF_reg[0] << 12) + (LQBP001MB_CAL_PRE_TEMP.COEFF_reg[1] << 4) + ((LQBP001MB_CAL_PRE_TEMP.COEFF_reg[2] >> 4) & 0x0F);
    if (LQBP001MB_CAL_COFF_RE.C00 > LQBP001MB_POW_2_19_MINUS_1)
        LQBP001MB_CAL_COFF_RE.C00 = LQBP001MB_CAL_COFF_RE.C00 - LQBP001MB_POW_2_20;

    LQBP001MB_CAL_COFF_RE.C10 = ((LQBP001MB_CAL_PRE_TEMP.COEFF_reg[2] & 0x0F) << 16) + (LQBP001MB_CAL_PRE_TEMP.COEFF_reg[3] << 8) + LQBP001MB_CAL_PRE_TEMP.COEFF_reg[4];
    if (LQBP001MB_CAL_COFF_RE.C10 > LQBP001MB_POW_2_19_MINUS_1)
        LQBP001MB_CAL_COFF_RE.C10 = LQBP001MB_CAL_COFF_RE.C10 - LQBP001MB_POW_2_20;

    LQBP001MB_CAL_COFF_RE.C01 = (LQBP001MB_CAL_PRE_TEMP.COEFF_reg[5] << 12) + (LQBP001MB_CAL_PRE_TEMP.COEFF_reg[6] << 4) + ((LQBP001MB_CAL_PRE_TEMP.COEFF_reg[7] >> 4) & 0x0F);
    if (LQBP001MB_CAL_COFF_RE.C01 > LQBP001MB_POW_2_19_MINUS_1)
        LQBP001MB_CAL_COFF_RE.C01 = LQBP001MB_CAL_COFF_RE.C01 - LQBP001MB_POW_2_20;

    LQBP001MB_CAL_COFF_RE.C02 = ((LQBP001MB_CAL_PRE_TEMP.COEFF_reg[7] & 0x0F) << 16) + (LQBP001MB_CAL_PRE_TEMP.COEFF_reg[8] << 8) + LQBP001MB_CAL_PRE_TEMP.COEFF_reg[9];
    if (LQBP001MB_CAL_COFF_RE.C02 > LQBP001MB_POW_2_19_MINUS_1)
        LQBP001MB_CAL_COFF_RE.C02 = LQBP001MB_CAL_COFF_RE.C02 - LQBP001MB_POW_2_20;

    LQBP001MB_CAL_COFF_RE.C20 = ((LQBP001MB_CAL_PRE_TEMP.COEFF_reg[10] & 0x7F) << 8) + LQBP001MB_CAL_PRE_TEMP.COEFF_reg[11];
    if (LQBP001MB_CAL_COFF_RE.C20 > LQBP001MB_POW_2_14_MINUS_1)
        LQBP001MB_CAL_COFF_RE.C20 = LQBP001MB_CAL_COFF_RE.C20 - LQBP001MB_POW_2_15;

    LQBP001MB_CAL_COFF_RE.C30 = ((LQBP001MB_CAL_PRE_TEMP.COEFF_reg[12] & 0x0F) << 8) + LQBP001MB_CAL_PRE_TEMP.COEFF_reg[13];
    if (LQBP001MB_CAL_COFF_RE.C30 > LQBP001MB_POW_2_11_MINUS_1)
        LQBP001MB_CAL_COFF_RE.C30 = LQBP001MB_CAL_COFF_RE.C30 - LQBP001MB_POW_2_12;

    LQBP001MB_CAL_COFF_RE.C11 = (LQBP001MB_CAL_PRE_TEMP.COEFF_reg[14] << 9) + (LQBP001MB_CAL_PRE_TEMP.COEFF_reg[15] << 1) + ((LQBP001MB_CAL_PRE_TEMP.COEFF_reg[16] & 0x80) >> 7);
    if (LQBP001MB_CAL_COFF_RE.C11 > LQBP001MB_POW_2_16_MINUS_1)
        LQBP001MB_CAL_COFF_RE.C11 = LQBP001MB_CAL_COFF_RE.C11 - LQBP001MB_POW_2_17;

    LQBP001MB_CAL_COFF_RE.C12 = ((LQBP001MB_CAL_PRE_TEMP.COEFF_reg[16] & 0x7F) << 10) + (LQBP001MB_CAL_PRE_TEMP.COEFF_reg[17] << 2) + ((LQBP001MB_CAL_PRE_TEMP.COEFF_reg[18] & 0xC0) >> 6);
    if (LQBP001MB_CAL_COFF_RE.C12 > LQBP001MB_POW_2_16_MINUS_1)
        LQBP001MB_CAL_COFF_RE.C12 = LQBP001MB_CAL_COFF_RE.C12 - LQBP001MB_POW_2_17;

    LQBP001MB_CAL_COFF_RE.C21 = ((LQBP001MB_CAL_PRE_TEMP.COEFF_reg[18] & 0x3F) << 8) + LQBP001MB_CAL_PRE_TEMP.COEFF_reg[19];
    if (LQBP001MB_CAL_COFF_RE.C21 > LQBP001MB_POW_2_13_MINUS_1)
        LQBP001MB_CAL_COFF_RE.C21 = LQBP001MB_CAL_COFF_RE.C21 - LQBP001MB_POW_2_14;

#ifdef __ENABLE_DEBUG__

    print_debug("C00 %d\r\n", LQBP001MB_CAL_COFF_RE.C00);
    print_debug("C10 %d\r\n", LQBP001MB_CAL_COFF_RE.C10);
    print_debug("C01 %d\r\n", LQBP001MB_CAL_COFF_RE.C01);
    print_debug("C02 %d\r\n", LQBP001MB_CAL_COFF_RE.C02);

    print_debug("C20 %d\r\n", LQBP001MB_CAL_COFF_RE.C20);
    print_debug("C30 %d\r\n", LQBP001MB_CAL_COFF_RE.C30);
    print_debug("C11 %d\r\n", LQBP001MB_CAL_COFF_RE.C11);
    print_debug("C12 %d\r\n", LQBP001MB_CAL_COFF_RE.C12);
    print_debug("C21 %d\r\n", LQBP001MB_CAL_COFF_RE.C21);

    print_debug("T_GAIN_COEFF %d\r\n", LQBP001MB_CAL_COFF_RE.T_GAIN_COEFF);
    print_debug("T_dVBE_COEFF %d\r\n", LQBP001MB_CAL_COFF_RE.T_dVBE_COEFF);
    print_debug("T_VBE_COEFF %d\r\n", LQBP001MB_CAL_COFF_RE.T_VBE_COEFF);

    print_debug("Ap %f\r\n", LQBP001MB_CAL_COFF_RE.Ap);
    print_debug("Bp %f\r\n", LQBP001MB_CAL_COFF_RE.Bp);

#endif
}

/**
 * @brief   	LQBP001MB 根据参数计算气压值和 温度值
 * @param   	参数
 *		@arg 	无
 * @retval  	无
 * @attention	计算结果
 */
void LQBP001MB_calculate(void)
{
#define MAX_READ_ATTEMPTS 5 // 定义最大尝试次数

    bool readSuccessful = false;
    for (int attempt = 0; attempt < MAX_READ_ATTEMPTS; ++attempt)
    {
        if (I2C_ReadReg(LQBP001MB, 0x08) == 0xf7)
        {
            I2C_ReadBuf(LQBP001MB, 0x00, &LQBP001MB_CAL_PRE_TEMP.PSR_B[0], 6); // 读取气压温度数值
            readSuccessful = true;
            break; // 成功读取后退出循环
        }
        else
        {
            lqbp001mb_delay_ms(10); // 如果读取不成功，等待并重试
        }
    }

    if (!readSuccessful)
    {
        // 这里可以添加错误处理逻辑，比如记录错误日志或者触发错误处理函数
        print_debug("Failed to read data after %d attempts.\r\n", MAX_READ_ATTEMPTS);
        app_error_set(RT_ERROR,AIR_PRESSURE);
    }
    //----------------------------------------计算压力数值-----------------------------------------

    s32 P_raw = (LQBP001MB_CAL_PRE_TEMP.PSR_B[0] << 16) | (LQBP001MB_CAL_PRE_TEMP.PSR_B[1] << 8) | (LQBP001MB_CAL_PRE_TEMP.PSR_B[2]);
    P_raw = (P_raw & 0x800000) ? (0xFF000000 | P_raw) : P_raw;

    // s32 T_raw	=    (LQBP001MB_CAL_PRE_TEMP.TMP_B[0]<<16)|(LQBP001MB_CAL_PRE_TEMP.TMP_B[1]<<8)|(LQBP001MB_CAL_PRE_TEMP.TMP_B[2]);
    s32 T_raw = (LQBP001MB_CAL_PRE_TEMP.PSR_B[3] << 16) | (LQBP001MB_CAL_PRE_TEMP.PSR_B[4] << 8) | (LQBP001MB_CAL_PRE_TEMP.PSR_B[5]);

    T_raw = (T_raw & 0x800000) ? (0xFF000000 | T_raw) : T_raw;

#ifdef __ENABLE_DEBUG__

    print_debug(" 00h:%x\n 01h:%x\n 02h:%x\r\n", LQBP001MB_CAL_PRE_TEMP.PSR_B[0], LQBP001MB_CAL_PRE_TEMP.PSR_B[1], LQBP001MB_CAL_PRE_TEMP.PSR_B[2]);

    print_debug("P_raw%d\r\n", P_raw);
    print_debug("T_raw %d\r\n", T_raw);
#endif

    //-----------------------计算气压最终结果------------------------
    double Pres_SCALED = (double)P_raw / LQBP001MB_CAL_COFF_RE.KP;
    double Temp_X = (double)T_raw / 1048576;
    double Temp_SCALED = (8.5 * Temp_X) / (1 + 8.8 * Temp_X);

    float PA_CAL = LQBP001MB_CAL_COFF_RE.C00 + (LQBP001MB_CAL_COFF_RE.C10 * Pres_SCALED) + (LQBP001MB_CAL_COFF_RE.C01 * Temp_SCALED) + (LQBP001MB_CAL_COFF_RE.C20 * Pres_SCALED * Pres_SCALED) + (LQBP001MB_CAL_COFF_RE.C02 * Temp_SCALED * Temp_SCALED) + (LQBP001MB_CAL_COFF_RE.C30 * Pres_SCALED * Pres_SCALED * Pres_SCALED) + (LQBP001MB_CAL_COFF_RE.C11 * Pres_SCALED * Temp_SCALED) + (LQBP001MB_CAL_COFF_RE.C12 * Pres_SCALED * Temp_SCALED * Temp_SCALED) + (LQBP001MB_CAL_COFF_RE.C21 * Pres_SCALED * Pres_SCALED * Temp_SCALED);

    //-----------------------计算温度最终结果------------------------
    double T_CAL = (double)T_raw / 1048576;

    double uu = (double)T_CAL / (1 + aa * T_CAL);

    double T_OUT = (double)(LQBP001MB_CAL_COFF_RE.Ap * uu) + LQBP001MB_CAL_COFF_RE.Bp;

    LQBP001MB_CAL_PRE_TEMP.temp_result = T_OUT;
    LQBP001MB_CAL_PRE_TEMP.pre_result = PA_CAL;
    print_debug("temp_result:%f, pre_result:%f\r\n", LQBP001MB_CAL_PRE_TEMP.temp_result, LQBP001MB_CAL_PRE_TEMP.pre_result);
}

/*****************************************************************************
 函数名 :  LQBP001MB_rateset
 功能描述: 设置气压传感器的每秒采样率和过采样率
 输入参数: uint8_t u8OverSmpl  过采样率         Maximal = 128
           uint8_t u8SmplRate 每秒采样次数(Hz) Maximal = 128
           uint8_t iSensor     0: Pressure; 1: Temperature
*****************************************************************************/
void LQBP001MB_rateset(uint8_t iSensor, uint8_t u8SmplRate, uint8_t u8OverSmpl)
{
    uint8_t reg = 0;
    s32 KP;
    switch (u8SmplRate)
    {
    case 2:
        reg |= (1 << 4);
        break;
    case 4:
        reg |= (2 << 4);
        break;
    case 8:
        reg |= (3 << 4);
        break;
    case 16:
        reg |= (4 << 4);
        break;
    case 32:
        reg |= (5 << 4);
        break;
    case 64:
        reg |= (6 << 4);
        break;
    case 128:
        reg |= (7 << 4);
        break;
    case 1:
    default:
        break;
    }
    switch (u8OverSmpl)
    {
    case 2:
        reg |= 1;
        KP = 1572864;
        break;
    case 4:
        reg |= 2;
        KP = 3670016;
        break;
    case 8:
        reg |= 3;
        KP = 7864320;
        break;
    case 16:
        KP = 253952;
        reg |= 4;
        break;
    case 32:
        KP = 516096;
        reg |= 5;
        break;
    case 64:
        KP = 1040384;
        reg |= 6;
        break;
    case 128:
        KP = 2088960;
        reg |= 7;
        break;
    case 1:
    default:
        KP = 524288;
        break;
    }

    if (iSensor == 0)
    {
        LQBP001MB_CAL_PRE_TEMP.PSR_CFG = reg;
        I2C_WriteReg(LQBP001MB, 0x06, reg);
        LQBP001MB_CAL_COFF_RE.KP = KP;
    }

    if (iSensor == 1)
    {
        LQBP001MB_CAL_PRE_TEMP.TMP_CFG = reg;
        I2C_WriteReg(LQBP001MB, 0x07, reg | 0x80); // Using mems temperature
    }
}
// 连续测量模式
void LQBP001MB_start_continuous(uint8_t mode)
{
    I2C_WriteReg(LQBP001MB, 0x08, mode + 4);
}

int32_t drv_bar_lqbp001mb_init(void)
{
    print_debug("=============lqbp001mb_init===========\n");
    open_device_power(SENSOR_PRESSURE_AIR);
    drv_bar_interface_init(400000);
	delay_ms(1);
    uint8_t chip_id = 0xff;
    if(get_chipid(&chip_id) == false)
    {
        print_debug("LQBP001MB write Failed");
        close_device_power(SENSOR_PRESSURE_AIR);
        return 1;
    }
    s_read_succeed_f = false;

    LQBP001MB_INIT(); //读取校准参数，然后进行计算，保存
	
	
	//-----32hz 30ms一次采样的配置，气压每秒钟更新32次数据	
	
	LQBP001MB_rateset(PRESSURE_SENSOR,8, 4); 
	LQBP001MB_rateset(TEMPERATURE_SENSOR,8, 4);
	
	//-----64hz 20ms一次采样的配置，气压每秒钟更新64次数据
	//LQBP001MB_rateset(PRESSURE_SENSOR,64, 8); 
	//LQBP001MB_rateset(TEMPERATURE_SENSOR,4, 4);	
	
	LQBP001MB_start_continuous(CONTINUOUS_P_AND_T);	//background mode 111 - Continous pressure and temperature measurement
	delay_ms(1);		


#ifdef __ENABLE_DEBUG__	

	uint8_t a,b,c;

	a = I2C_ReadReg(LQBP001MB,0X06);
	b = I2C_ReadReg(LQBP001MB,0X07);
	c = I2C_ReadReg(LQBP001MB,0X08);

	print_debug(" 06h:%x\n 07h:%x\n 08h:%x\r\n",a,b,c);
	print_debug(" KP:%d\n 07h:%x\n 08h:%x\r\n",LQBP001MB_CAL_COFF_RE.KP,b,c);

#endif
    // close_device_power(SENSOR_PRESSURE_AIR);
    print_debug("LQBP001MB Init Ok!\r\n");	
    return 0;
}

void drv_bar_lqbp001mb_uninit(void)
{
    close_device_power(SENSOR_PRESSURE_AIR);
}

// @brief 关闭气压计 lqbp001mb
void drv_bar_lqbp001mb_stop(void)
{
    if (lqbp001mb_is_open == false) {
        return;
    }
    s_read_succeed_f = false;
    //先关闭连续测量模式
    // I2C_WriteReg(LQBP001MB, 0x08, 0);
    delay_ms(1);
    drv_bar_lqbp001mb_uninit();
    lqbp001mb_is_open = false;
}
// @brief 使能气压计 lqbp001mb
void drv_bar_lqbp001mb_start(void)
{
    print_debug("enter drv_bar_lqbp001mb_start\r\n");
    open_device_power(SENSOR_PRESSURE_AIR); 
    if (lqbp001mb_is_open == true) {
        lqbp001mb_read_data(&temperature_value,&pressure_value);
        return;
    }
    drv_bar_lqbp001mb_init();
    open_device_power(SENSOR_PRESSURE_AIR);
    lqbp001mb_is_open = true;
    lqbp001mb_read_data(&temperature_value,&pressure_value);
}

int32_t lqbp001mb_drv_init(uint8_t *id)
{
    int32_t error_code = ERROR_CODE_SUCCESS;
    air_pressure_imp_init();

    error_code = air_pressure_al_init();

    print_debug("aip2520r_drv_init %d\r\n", error_code);

    return error_code;
}



int32_t lqbp001mb_read_data(float *temperature_value, float *pressure_value)
{
    LQBP001MB_calculate();
    *temperature_value = LQBP001MB_CAL_PRE_TEMP.temp_result;
    *pressure_value = LQBP001MB_CAL_PRE_TEMP.pre_result;
    printf("Temperature: %.1f, Pressure: %.2fPa\r\n", *temperature_value, *pressure_value);
    return 0;
}



#endif
