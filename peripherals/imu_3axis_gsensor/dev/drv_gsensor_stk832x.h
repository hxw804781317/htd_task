#ifndef _DRV_GSENSOR_STK832X_REG_H_
#define _DRV_GSENSOR_STK832X_REG_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


//////////////////////////////////////////////////////////////////////
#define STK8321_ODR_3HZ                 0x10
#define STK8321_ODR_6HZ                 0x20



//////////////////////////////////////////////////////////////////////
#define STK8321_TOTAL_FIFO_LENGTH          0x20

/* Common addresses definition for temperature sensor. */
#define STK832x_ADDR                       0x1F   /* SA0 pull-high, 7-bit slave address */

#define STK832x_REG_0x00_CHIPID            0x00    // 
#define STK832x_REG_0x02_ACCD_X_LSB        0x02    // 
#define STK832x_REG_0x03_ACCD_X_MSB        0x03    // 
#define STK832x_REG_0x04_ACCD_Y_LSB        0x04    // 
#define STK832x_REG_0x05_ACCD_Y_MSB        0x05    // 
#define STK832x_REG_0x06_ACCD_Z_LSB        0x06    // 
#define STK832x_REG_0x07_ACCD_Z_MSB        0x07    // 
#define STK832x_REG_0x0C_FIFOSTS           0x0C    // 获取数据长度
#define STK832x_REG_0x0F_G_RANGE           0x0F    // @stk832x_g_range_t
#define STK832x_REG_0x10_BANDWIDTH         0x10    // @stk832x_bandwidth_t
#define STK832x_REG_0x11_POWER_MODE        0x11    // @stk832x_power_mode_t
#define STK832x_REG_0x14_SOFTRESET         0x14    // 
#define STK832x_REG_0x16_INTEN1            0x16    // 
#define STK832x_REG_0x17_INTEN2            0x17    // 
#define STK832x_REG_0x19_INTMAP1           0x19    // 
#define STK832x_REG_0x1A_INTMAP2           0x1A    // 
#define STK832x_REG_0x20_INTCFG1           0x20    // 
#define STK832x_REG_0x21_INTCFG2           0x21    // 
#define STK832x_REG_0x27_SLOPEDLY          0x27    // 
#define STK832x_REG_0x28_SLOPETHD          0x28    // 
#define STK832x_REG_0x34_INTFCFG           0x34    // 
#define STK832x_REG_0x3D_FIFOCFG1          0x3D    // 
#define STK832x_REG_0x3E_FIFOCFG2          0x3E    // @stk832x_mode_t
#define STK832x_REG_0x3F_FIFODATA          0x3F    // 
#define STK832x_REG_0x5E_RES_1             0x5E    // 
#define STK832x_REG_0x33_RES_2             0x33    /*spi first set*/
#define STK832x_REG_0x70_RES_3             0x70    /*spi first set reg enable*/

#define STK832x_SPI_REG_ENABLE          0xE1
#define STK832x_SPI_REG_VALUE           0x5A

//SOFTRESET
#define STK832x_SOFTRESET               0xB6

//FIFOCFG2
typedef enum {
    STK832X_FIFO_INTERVAL1 = 0x00,
    STK832X_FIFO_INTERVAL2,
    STK832X_FIFO_INTERVAL4,
    STK832X_FIFO_INTERVAL8,
} stk832x_fifo_interval_t;
typedef enum {
    STK832X_MODE_BYPASS = 0x00, 
    STK832X_MODE_FIFO,
    STK832X_MODE_RESERVED1,
    STK832X_MODE_STREAM_TO_FIFO,
    STK832X_MODE_BYPASS_TO_STREAM,
    STK832X_MODE_RESERVED2,
    STK832X_MODE_STREAM,
    STK832X_MODE_RESERVED3,
} stk832x_mode_t;
#define STK832x_INIT_MODE           ((STK832X_MODE_STREAM<<5)|(STK832X_FIFO_INTERVAL2<<2))

//INTMAP2 REG
#define STK832x_DATA2INT1_BIT       (0x01UL<<0)
#define STK832x_FWM2INT1_BIT        (0x01UL<<1)
#define STK832x_FFULL2INT1_BIT      (0x01UL<<2)
#define STK832x_FFULL2INT2_BIT      (0x01UL<<5)
#define STK832x_FWM2INT2_BIT        (0x01UL<<6)
#define STK832x_DATA2INT2_BIT       (0x01UL<<7)

//INTEN2 REG
#define STK832x_DATA_EN_BIT         (0x01UL<<4)
#define STK832x_FFULL_EN_BIT        (0x01UL<<5)
#define STK832x_FWM_EN_BIT          (0x01UL<<6)

//INTCFG1 REG 
#define STK832x_INT1_LV_BIT         (0x01UL<<0)
#define STK832x_INT1_OD_BIT         (0x01UL<<1)
#define STK832x_INT2_LV_BIT         (0x01UL<<2)
#define STK832x_INT2_OD_BIT         (0x01UL<<3) 

//G_RANGE REG
#define STK832x_SENSING_RANGE_2G    0x03
#define STK832x_SENSING_RANGE_4G    0x05
#define STK832x_SENSING_RANGE_8G    0x08
typedef uint8_t stk832x_g_range_t;

//BANDWIDTH REG
typedef enum {
    STK832X_BANDWIDTH_7_81HZ = 0x08,    //0.5ms
    STK832X_BANDWIDTH_15_63HZ,          //1ms
    STK832X_BANDWIDTH_31_25HZ,          //2ms
    STK832X_BANDWIDTH_62_5HZ,           //4ms
    STK832X_BANDWIDTH_125HZ,            //6ms
    STK832X_BANDWIDTH_250HZ,            //10ms
    STK832X_BANDWIDTH_500HZ,            //25ms
    STK832X_BANDWIDTH_1000HZ,           //50ms
} stk832x_bandwidth_t;

// 运行模式
#define STK832x_SUSPEND_BIT         (0x01UL<<7)     // 0x80
#define STK832x_LOWPOWER_BIT        (0x01UL<<6)     // 0x40
#define STK832x_SLEEP_TIMER_BIT     (0x01UL<<5)     // 0x20
typedef uint8_t stk832x_power_mode_t;

typedef enum {
    STK832X_FREQ_2KHZ = 0x05,       //0.5ms  
    STK832X_FREQ_1KHZ,              //1ms
    STK832X_FREQ_500HZ,             //2ms
    STK832X_FREQ_250HZ,             //4ms
    STK832X_FREQ_167HZ,             //6ms
    STK832X_FREQ_100HZ,             //10ms
    STK832X_FREQ_40HZ,              //25ms
    STK832X_FREQ_20HZ,              //50ms
    STK832X_FREQ_10HZ,              //100ms
    STK832X_FREQ_2HZ,               //500ms
    STK832X_FREQ_1HZ,               //1000ms
} stk832x_frequency_t;
#define STK832x_INIT_FREQ           (STK832X_FREQ_100HZ<<0)
#define STK832x_CHIP_ID_VAL         0x23    /* STK8323 CHIP ID = 0x23 */


#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*_DRV_GSENSOR_STK832X_REG_H_*/

