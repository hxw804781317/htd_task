#if 0


#define __ENABLE_DEBUG__
#define DEBUG_TAG           "bar.lqbp001ta"
#include "sdrv_lqbp001ta.h"


static struct lqbp001ta_t lqbp001ta;
static struct lqbp001ta_t *p_lqbp001ta;

void lqbp001ta_write(uint8_t hwadr, uint8_t regadr, uint8_t val);
uint8_t lqbp001ta_read(uint8_t hwadr, uint8_t regadr);
void lqbp001ta_get_calib_param(void);

/*****************************************************************************
 Function: lqbp001ta_write
 Description: this function will write data to specofic register through software I2C bus
 Input:  uint8_t hwadr   hardware I2C address
         uint8_t regadr  register address
         uint8_t val     write-in value          
 Output: 
 Return: 
 Calls: 
 Called By: 
*****************************************************************************/
void lqbp001ta_write(uint8_t hwadr, uint8_t regadr, uint8_t val)
{
    drv_bar_register_cmd_write(hwadr,regadr,val);
}

/*****************************************************************************
 Function: lqbp001ta_read
 Description: this function will read register data through software I2C bus
 Input: uint8_t hwadr   hardware I2C address
        uint8_t regadr  register address        
 Output: 
 Return: uint8_t readout value
 Calls: 
 Called By: 
*****************************************************************************/
uint8_t lqbp001ta_read(uint8_t hwadr, uint8_t regadr)
{
    uint8_t val = 0;
    drv_bar_register_data_read(hwadr,regadr,1);
}

/*****************************************************************************
 Function: lqbp001ta_init
 Description: initialization
 Input: void             
 Output: 
 Return: void 
 Calls: 
 Called By: 
*****************************************************************************/
void lqbp001ta_init(void)
{
    p_lqbp001ta = &lqbp001ta; /* read Chip Id */
    p_lqbp001ta->i32rawPressure = 0;
    p_lqbp001ta->i32rawTemperature = 0;
    p_lqbp001ta->chip_id = 0x10;
    lqbp001ta_get_calib_param();
    // sampling rate = 1Hz; Pressure oversample = 2;
    lqbp001ta_rateset(PRESSURE_SENSOR,32, 8);   
    // sampling rate = 1Hz; Temperature oversample = 1; 
    lqbp001ta_rateset(TEMPERATURE_SENSOR,32, 8);
    //Start background measurement
    
}

/*****************************************************************************
 Function: lqbp001ta_rateset
 Description: set sample rate and over sample rate per second for specific sensor
 Input:     uint8_t u8OverSmpl  oversample rate         Maximal = 128
            uint8_t u8SmplRate  sample rate(Hz) Maximal = 128
            uint8_t iSensor     0: Pressure; 1: Temperature 
 Output: 
 Return: void
 Calls: 
 Called By: 
*****************************************************************************/
void lqbp001ta_rateset(uint8_t iSensor, uint8_t u8SmplRate, uint8_t u8OverSmpl)
{
    uint8_t reg = 0;
    int32 i32kPkT = 0;
    switch(u8SmplRate)
    {
        case 2:
            reg |= (1<<4);
            break;
        case 4:
            reg |= (2<<4);
            break;
        case 8:
            reg |= (3<<4);
            break;
        case 16:
            reg |= (4<<4);
            break;
        case 32:
            reg |= (5<<4);
            break;
        case 64:
            reg |= (6<<4);
            break;
        case 128:
            reg |= (7<<4);
            break;
        case 1:
        default:
            break;
    }
    switch(u8OverSmpl)
    {
        case 2:
            reg |= 1;
            i32kPkT = 1572864;
            break;
        case 4:
            reg |= 2;
            i32kPkT = 3670016;
            break;
        case 8:
            reg |= 3;
            i32kPkT = 7864320;
            break;
        case 16:
            i32kPkT = 253952;
            reg |= 4;
            break;
        case 32:
            i32kPkT = 516096;
            reg |= 5;
            break;
        case 64:
            i32kPkT = 1040384;
            reg |= 6;
            break;
        case 128:
            i32kPkT = 2088960;
            reg |= 7;
            break;
        case 1:
        default:
            i32kPkT = 524288;
            break;
    }

    if(iSensor == PRESSURE_SENSOR)
    {
        p_lqbp001ta->i32kP = i32kPkT;
        lqbp001ta_write(HW_ADR, 0x06, reg);
        if(u8OverSmpl > 8)
        {
            reg = lqbp001ta_read(HW_ADR, 0x09);
            lqbp001ta_write(HW_ADR, 0x09, reg | 0x04);
        }
        else
        {
            reg = lqbp001ta_read(HW_ADR, 0x09);
            lqbp001ta_write(HW_ADR, 0x09, reg & (~0x04));
        }
    }
    if(iSensor == TEMPERATURE_SENSOR)
    {
        p_lqbp001ta->i32kT = i32kPkT;
        lqbp001ta_write(HW_ADR, 0x07, reg|0x80);  //Using mems temperature
        if(u8OverSmpl > 8)
        {
            reg = lqbp001ta_read(HW_ADR, 0x09);
            lqbp001ta_write(HW_ADR, 0x09, reg | 0x08);
        }
        else
        {
            reg = lqbp001ta_read(HW_ADR, 0x09);
            lqbp001ta_write(HW_ADR, 0x09, reg & (~0x08));
        }
    }

}

/*****************************************************************************
 Function: lqbp001ta_get_calib_param
 Description: obtain the calibrated coefficient
 Input: void     
 Output: 
 Return: void
 Calls: 
 Called By: 
*****************************************************************************/
void lqbp001ta_get_calib_param(void)
{
    uint32 h;
    uint32 m;
    uint32 l;
    h =  lqbp001ta_read(HW_ADR, 0x10);
    l  =  lqbp001ta_read(HW_ADR, 0x11);
    p_lqbp001ta->calib_param.c0 = (int16)h<<4 | l>>4;
    p_lqbp001ta->calib_param.c0 = (p_lqbp001ta->calib_param.c0&0x0800)?(0xF000|p_lqbp001ta->calib_param.c0):p_lqbp001ta->calib_param.c0;
    h =  lqbp001ta_read(HW_ADR, 0x11);
    l  =  lqbp001ta_read(HW_ADR, 0x12);
    p_lqbp001ta->calib_param.c1 = (int16)(h&0x0F)<<8 | l;
    p_lqbp001ta->calib_param.c1 = (p_lqbp001ta->calib_param.c1&0x0800)?(0xF000|p_lqbp001ta->calib_param.c1):p_lqbp001ta->calib_param.c1;
    h =  lqbp001ta_read(HW_ADR, 0x13);
    m =  lqbp001ta_read(HW_ADR, 0x14);
    l =  lqbp001ta_read(HW_ADR, 0x15);
    p_lqbp001ta->calib_param.c00 = (int32)h<<12 | (int32)m<<4 | (int32)l>>4;
    p_lqbp001ta->calib_param.c00 = (p_lqbp001ta->calib_param.c00&0x080000)?(0xFFF00000|p_lqbp001ta->calib_param.c00):p_lqbp001ta->calib_param.c00;
    h =  lqbp001ta_read(HW_ADR, 0x15);
    m =  lqbp001ta_read(HW_ADR, 0x16);
    l =  lqbp001ta_read(HW_ADR, 0x17);
    p_lqbp001ta->calib_param.c10 = (int32)(h&0x0F)<<16 | (int32)m<<8 | l;
    p_lqbp001ta->calib_param.c10 = (p_lqbp001ta->calib_param.c10&0x080000)?(0xFFF00000|p_lqbp001ta->calib_param.c10):p_lqbp001ta->calib_param.c10;
    h =  lqbp001ta_read(HW_ADR, 0x18);
    l  =  lqbp001ta_read(HW_ADR, 0x19);
    p_lqbp001ta->calib_param.c01 = (int16)h<<8 | l;
    h =  lqbp001ta_read(HW_ADR, 0x1A);
    l  =  lqbp001ta_read(HW_ADR, 0x1B);
    p_lqbp001ta->calib_param.c11 = (int16)h<<8 | l;
    h =  lqbp001ta_read(HW_ADR, 0x1C);
    l  =  lqbp001ta_read(HW_ADR, 0x1D);
    p_lqbp001ta->calib_param.c20 = (int16)h<<8 | l;
    h =  lqbp001ta_read(HW_ADR, 0x1E);
    l  =  lqbp001ta_read(HW_ADR, 0x1F);
    p_lqbp001ta->calib_param.c21 = (int16)h<<8 | l;
    h =  lqbp001ta_read(HW_ADR, 0x20);
    l  =  lqbp001ta_read(HW_ADR, 0x21);
    p_lqbp001ta->calib_param.c30 = (int16)h<<8 | l;
}

/*****************************************************************************
 Function: lqbp001ta_start_temperature
 Description: start one measurement for temperature
 Input: void    
 Output: 
 Return: void
 Calls: 
 Called By: 
*****************************************************************************/
void lqbp001ta_start_temperature(void)
{
    lqbp001ta_write(HW_ADR, 0x08, 0x02);
}

/*****************************************************************************
 Function: lqbp001ta_start_pressure
 Description: start one measurement for pressure
 Input: void       
 Output: 
 Return: void
 Calls: 
 Called By: 
*****************************************************************************/

void lqbp001ta_start_pressure(void)
{
    lqbp001ta_write(HW_ADR, 0x08, 0x01);
}
/*****************************************************************************
 Function: lqbp001ta_start_continuous
 Description: Select mode for the continuously measurement
 Input: uint8_t mode  1: pressure; 2: temperature; 3: pressure and temperature        
 Output: 
 Return: void
 Calls: 
 Called By: 
*****************************************************************************/
void lqbp001ta_start_continuous(uint8_t mode)
{
    lqbp001ta_write(HW_ADR, 0x08, mode+4);
}

void lqbp001ta_stop(void)
{
    lqbp001ta_write(HW_ADR, 0x08, 0);
}

/*****************************************************************************
 Function: lqbp001ta_get_raw_temp
 Description:obtain the original temperature value and turn them into 32bits-integer 
 Input: void          
 Output: 
 Return: void
 Calls: 
 Called By: 
*****************************************************************************/
void lqbp001ta_get_raw_temp(void)
{
    uint8_t h,m,l;
    SoftI2C_start();
    SoftI2C_write_byte(HW_ADR << 1);
    SoftI2C_write_byte(0x03);
    SoftI2C_start();
    SoftI2C_write_byte((HW_ADR << 1)|0x01);
    h = SoftI2C_read_byte(0);
    m = SoftI2C_read_byte(0);
    l = SoftI2C_read_byte(1);
    SoftI2C_stop();
    p_lqbp001ta->i32rawTemperature = (int32)h<<16 | (int32)m<<8 | (int32)l;
    p_lqbp001ta->i32rawTemperature= (p_lqbp001ta->i32rawTemperature&0x800000) ? (0xFF000000|p_lqbp001ta->i32rawTemperature) : p_lqbp001ta->i32rawTemperature;
}

/*****************************************************************************
 Function: lqbp001ta_get_raw_pressure
 Description: obtain the original pressure value and turn them into 32bits-integer
 Input: void       
 Output: 
 Return: void
 Calls: 
 Called By: 
*****************************************************************************/
void lqbp001ta_get_raw_pressure(void)
{
    uint8_t h,m,l;
    SoftI2C_start();
    SoftI2C_write_byte(HW_ADR << 1);
    SoftI2C_write_byte(0x00);
    SoftI2C_start();
    SoftI2C_write_byte((HW_ADR << 1)|0x01);
    h = SoftI2C_read_byte(0);
    m = SoftI2C_read_byte(0);
    l = SoftI2C_read_byte(1);
    SoftI2C_stop();
    
    p_lqbp001ta->i32rawPressure = (int32)h<<16 | (int32)m<<8 | (int32)l;
    p_lqbp001ta->i32rawPressure= (p_lqbp001ta->i32rawPressure&0x800000) ? (0xFF000000|p_lqbp001ta->i32rawPressure) : p_lqbp001ta->i32rawPressure;
}

/*****************************************************************************
 Function: lqbp001ta_get_temperature
 Description:  return calibrated temperature value base on original value.
 Input: void          
 Output: 
 Return: void
 Calls: 
 Called By: 
*****************************************************************************/
float lqbp001ta_get_temperature(void)
{
    float fTCompensate;
    float fTsc;

    fTsc = p_lqbp001ta->i32rawTemperature / (float)p_lqbp001ta->i32kT;
    fTCompensate =  p_lqbp001ta->calib_param.c0 * 0.5 + p_lqbp001ta->calib_param.c1 * fTsc;
    return fTCompensate;
}

/*****************************************************************************
 Function: lqbp001ta_get_pressure
 Description: return calibrated pressure value base on original value.
 Input: void            
 Output: 
 Return: void
 Calls: 
 Called By: 
*****************************************************************************/

float lqbp001ta_get_pressure(void)
{
    float fTsc, fPsc;
    float qua2, qua3;
    float fPCompensate;

    fTsc = p_lqbp001ta->i32rawTemperature / (float)p_lqbp001ta->i32kT;
    fPsc = p_lqbp001ta->i32rawPressure / (float)p_lqbp001ta->i32kP;
    qua2 = p_lqbp001ta->calib_param.c10 + fPsc * (p_lqbp001ta->calib_param.c20 + fPsc* p_lqbp001ta->calib_param.c30);
    qua3 = fTsc * fPsc * (p_lqbp001ta->calib_param.c11 + fPsc * p_lqbp001ta->calib_param.c21);

    fPCompensate = p_lqbp001ta->calib_param.c00 + fPsc * qua2 + fTsc * p_lqbp001ta->calib_param.c01 + qua3;
    return fPCompensate;
}


#endif
